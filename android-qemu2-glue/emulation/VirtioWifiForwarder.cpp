/* Copyright 2020 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "android-qemu2-glue/emulation/VirtioWifiForwarder.h"

#include "aemu/base/Log.h"
#include "aemu/base/StringFormat.h"
#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/sockets/SocketUtils.h"
#include "android/network/Endian.h"
#include "android/network/WifiForwardClient.h"
#include "android/network/WifiForwardPeer.h"
#include "android/network/WifiForwardPipe.h"
#include "android/network/WifiForwardServer.h"
#include "android/network/mac80211_hwsim.h"

using android::base::AutoLock;
using android::base::IOVector;
using android::base::Looper;
using android::base::RecurrentTask;
using android::base::ScopedSocket;
using android::base::socketCreatePair;
using android::base::socketRecv;
using android::base::socketSend;
using android::base::StringFormat;
using android::base::ThreadLooper;
using android::emulation::HostapdController;
using android::network::CipherScheme;
using android::network::FrameInfo;
using android::network::FrameType;
using android::network::GenericNetlinkMessage;
using android::network::hwsim_tx_rate;
using android::network::Ieee80211Frame;
using android::network::MacAddress;
using android::network::WifiForwardClient;
using android::network::WifiForwardMode;
using android::network::WifiForwardPeer;
using android::network::WifiForwardServer;

extern "C" {
#include "common/ieee802_11_defs.h"
#include "drivers/driver_virtio_wifi.h"
#ifdef LIBSLIRP
#include "libslirp.h"
#include "utils/eloop.h"
#else
#include "qemu/osdep.h"
#include "net/net.h"
#include "net/slirp.h"
#endif
}  // extern "C"

// Conflicts with Log.h
#ifdef ERROR
#undef ERROR
#endif

namespace android {
namespace qemu2 {

struct WifiForwardHeader {
    WifiForwardHeader(FrameType type,
                      MacAddress transmitter,
                      uint32_t fullLength,
                      uint64_t cookie,
                      uint32_t flags,
                      uint32_t channel,
                      uint32_t numRates,
                      const hwsim_tx_rate* txRates)
        : magic((VirtioWifiForwarder::kWifiForwardMagic)),
          version(VirtioWifiForwarder::kWifiForwardVersion),
          type(static_cast<uint16_t>(type)),
          transmitter(transmitter),
          dataOffset(sizeof(WifiForwardHeader)),
          fullLength(fullLength),
          cookie(cookie),
          flags(flags),
          channel(channel),
          numRates(numRates) {
        memcpy(rates, txRates,
               std::min(numRates, Ieee80211Frame::TX_MAX_RATES));
    }

    uint32_t magic;
    uint8_t version;
    uint8_t type;
    MacAddress transmitter;
    uint16_t dataOffset;
    uint32_t fullLength;
    uint64_t cookie;
    uint32_t flags;
    uint32_t channel;
    uint32_t numRates;
    hwsim_tx_rate rates[Ieee80211Frame::TX_MAX_RATES] = {};
} __attribute__((__packed__));

struct ieee8023_hdr {
    uint8_t dest[ETH_ALEN]; /* destination eth addr */
    uint8_t src[ETH_ALEN];  /* source ether addr    */
    uint16_t ethertype;     /* packet type ID field */
} __attribute__((__packed__));

const char* const VirtioWifiForwarder::kNicModel = "virtio-wifi-device";
const char* const VirtioWifiForwarder::kNicName = "virtio-wifi-device";
static constexpr float kTUtoMS = 1.024f;

VirtioWifiForwarder::VirtioWifiForwarder(
        const uint8_t* bssid,
        WifiService::OnReceiveCallback cb,
        WifiService::OnLinkStatusChangedCallback onLinkStatusChanged,
        WifiService::CanReceiveCallback canReceive,
        WifiService::OnSentCallback onFrameSentCallback,
        ::NICConf* conf,
        Slirp* slirp,
        uint16_t serverPort,
        uint16_t clientPort)
    : mBssID(MACARG(bssid)),
      mOnFrameAvailableCallback(std::move(cb)),
      mOnLinkStatusChanged(std::move(onLinkStatusChanged)),
      mCanReceive(std::move(canReceive)),
      mOnFrameSentCallback(std::move(onFrameSentCallback)),
      mLooper(ThreadLooper::get()),
      mNicConf(conf),
      mSlirp(slirp),
      mServerPort(serverPort),
      mClientPort(clientPort) {}

VirtioWifiForwarder::~VirtioWifiForwarder() {
    stop();
}

bool VirtioWifiForwarder::init() {
    mHostapd = HostapdController::getInstance();
    if (!mHostapd) {
        LOG(ERROR)
                << "Failed to initialize Wi-Fi:  hostapd controller is null.";
        return false;
    }
    // init socket pair.
    int fds[2];
    if (socketCreatePair(&fds[0], &fds[1])) {
        LOG(ERROR)
                << "Failed to initialize Wi-Fi: Unable to create socket pair.";
        return false;
    }
    mHostapdSock = ScopedSocket(fds[0]);
    mVirtIOSock = ScopedSocket(fds[1]);
    mHostapdSockInitSuccess = mHostapd->setDriverSocket(mHostapdSock);

#ifdef LIBSLIRP
    eloop_register_read_sock(mVirtIOSock.get(),
                             VirtioWifiForwarder::eloopSocketHandler, nullptr,
                             this);
#else
    //  Looper FdWatch and set callback functions
    mFdWatch = mLooper->createFdWatch(mVirtIOSock.get(),
                                      &VirtioWifiForwarder::onHostApd, this);
    mFdWatch->wantRead();
    mFdWatch->dontWantWrite();

    if (mNicConf) {
        // init Qemu Nic
        static NetClientInfo info = {
                .type = NET_CLIENT_DRIVER_NIC,
                .size = sizeof(NICState),
                .receive = &VirtioWifiForwarder::onNICFrameAvailable,
                .can_receive = &VirtioWifiForwarder::canReceive,
                .link_status_changed =
                        &VirtioWifiForwarder::onLinkStatusChanged,
        };

        mNic = qemu_new_nic(&info, mNicConf, kNicModel, kNicName, this);
    }
#endif
    // init WifiFordPeer for P2P network.
    auto onData = [this](const uint8_t* data, size_t size) {
        return this->onRemoteData(data, size);
    };

    if (mClientPort > 0) {
        mRemotePeer.reset(new WifiForwardClient(mClientPort, onData));
    } else if (mServerPort > 0) {
        mRemotePeer.reset(new WifiForwardServer(mServerPort, onData));
    }
    if (mRemotePeer != nullptr) {
        mRemotePeer->init();
        mRemotePeer->run();
    }
    resetBeaconTask();
    mBeaconTask->start();
    LOG(DEBUG) << "Successfully initialized Wi-Fi";
    return true;
}

void VirtioWifiForwarder::ackLocalFrame(const Ieee80211Frame* frame) {
    GenericNetlinkMessage txInfo(GenericNetlinkMessage::NL_AUTO_PORT,
                                 GenericNetlinkMessage::NL_AUTO_SEQ,
                                 GenericNetlinkMessage::NLMSG_MIN_TYPE, 0, 0,
                                 HWSIM_CMD_TX_INFO_FRAME, 0);
    const FrameInfo& info = frame->info();
    txInfo.putAttribute(HWSIM_ATTR_ADDR_TRANSMITTER, info.mTransmitter.mAddr,
                        ETH_ALEN);
    uint32_t flags = info.mFlags | HWSIM_TX_STAT_ACK;
    txInfo.putAttribute(HWSIM_ATTR_FLAGS, &flags, sizeof(uint32_t));
    txInfo.putAttribute(HWSIM_ATTR_COOKIE, &info.mCookie, sizeof(uintptr_t));
    uint32_t signal = -50;
    txInfo.putAttribute(HWSIM_ATTR_SIGNAL, &signal, sizeof(signal));
    txInfo.putAttribute(HWSIM_ATTR_TX_INFO, info.mTxRates.data(),
                        Ieee80211Frame::TX_MAX_RATES * sizeof(hwsim_tx_rate));
    AutoLock lock(mLock);
    mOnFrameAvailableCallback(txInfo.data(), txInfo.dataLen());
}

int VirtioWifiForwarder::send(const IOVector& iov) {
    if (!mHostapd->isRunning()) {
        return 0;
    }
    if (!mHostapdSockInitSuccess.load()) {
        mHostapdSockInitSuccess = mHostapd->setDriverSocket(mHostapdSock);
        if (!mHostapdSockInitSuccess.load()) {
            LOG(DEBUG) << "Hostapd event loop has not been initialized yet.";
            return 0;
        }
    }
    if (iov.summedLength() < IEEE80211_HDRLEN) {
        return 0;
    }
    const GenericNetlinkMessage msg(iov);
    if (msg.genericNetlinkHeader()->cmd != HWSIM_CMD_FRAME) {
        return 0;
    }

    std::unique_ptr<Ieee80211Frame> frame = parseHwsimCmdFrame(msg);
    if (frame == nullptr) {
        LOG(DEBUG) << "Not a HWSIM_CMD_FRAME netlink message";
        return 0;
    }
    const MacAddress addr1 = frame->addr1();

    if (frame->isProtected()) {
        if (!frame->decrypt(mHostapd->getCipherScheme())) {
            LOG(ERROR) << "Unable to decrypt WPA-protected frame.";
            return 0;
        }
    }
    if (frame->isValid()) {
        ackLocalFrame(frame.get());
    }
    // Data frames
    if (frame->isData()) {
        // EAPoL is used in Wi-Fi 4-way handshake.
        if (frame->isEAPoL()) {
            if (socketSend(mVirtIOSock.get(), frame->data(), frame->size()) <
                0) {
                LOG(DEBUG) << "Failed to send frame to hostapd.";
            }
        } else if (addr1 != mBssID) {
            sendToRemoteVM(std::move(frame), FrameType::Data);
        } else if (frame->isToDS() && !frame->isFromDS()) {
            return sendToNIC(std::move(frame));
        }
    } else {  // Mgmt or Ctrl frames.
        if (addr1.isBroadcast() || addr1.isMulticast() || addr1 == mBssID) {
            // TODO (wdu@) Experiement with shared memory approach
            if (socketSend(mVirtIOSock.get(), frame->data(), frame->size()) <
                0) {
                LOG(DEBUG) << "Failed to send frame to hostapd.";
            }
        }
        if (addr1.isMulticast() || addr1 != mBssID) {
            sendToRemoteVM(std::move(frame), FrameType::Data);
        }
    }
    return 0;
}

// Because virtio-wifi driver uses recv callback, this function is not used.
int VirtioWifiForwarder::recv(android::base::IOVector& iov) {
    return 0;
}

void VirtioWifiForwarder::stop() {
    mBeaconTask.clear();
    mNic = nullptr;
    mSlirp = nullptr;
#ifdef LIBSLIRP
    eloop_unregister_read_sock(mVirtIOSock.get());
#endif
    if (mHostapd) {
        mHostapd->terminate();
    }
    if (mRemotePeer) {
        mRemotePeer->stop();
        mRemotePeer.reset(nullptr);
    }
}

#ifdef LIBSLIRP
void VirtioWifiForwarder::eloopSocketHandler(int sock,
                                             void* eloop_ctx,
                                             void* sock_ctx) {
    VirtioWifiForwarder::onHostApd(sock_ctx, sock,
                                   android::base::Looper::FdWatch::kEventRead);
}
#else

MacAddress VirtioWifiForwarder::getStaMacAddr(const char* ssid) {
    if (!mHostapd->getSsid().compare(ssid))
        return mFrameInfo.mTransmitter;
    else
        return MacAddress();
}

VirtioWifiForwarder* VirtioWifiForwarder::getInstance(NetClientState* nc) {
    return static_cast<VirtioWifiForwarder*>(qemu_get_nic_opaque(nc));
}

int VirtioWifiForwarder::canReceive(NetClientState* nc) {
    auto forwarder = getInstance(nc);
    return forwarder->mCanReceive(nc->queue_index);
}

void VirtioWifiForwarder::onLinkStatusChanged(NetClientState* nc) {
    auto forwarder = getInstance(nc);
    forwarder->mOnLinkStatusChanged(nc);
}

void VirtioWifiForwarder::onFrameSentCallback(NetClientState* nc,
                                              ssize_t size) {
    auto forwarder = getInstance(nc);
    if (forwarder->mOnFrameSentCallback) {
        forwarder->mOnFrameSentCallback(nc->queue_index);
    }
}

// QEMU NIC client will receive one ethernet packet at a time.
ssize_t VirtioWifiForwarder::onNICFrameAvailable(NetClientState* nc,
                                                 const uint8_t* buf,
                                                 size_t size) {
    auto forwarder = static_cast<VirtioWifiForwarder*>(getInstance(nc));
    if (!forwarder || size < ETH_HLEN || !forwarder->mHostapd->isRunning()) {
        return -1;
    }
    return forwarder->onRxPacketAvailable(buf, size);
}
#endif

ssize_t VirtioWifiForwarder::sendToGuest(
        std::unique_ptr<Ieee80211Frame> frame) {
    GenericNetlinkMessage msg(GenericNetlinkMessage::NL_AUTO_PORT,
                              GenericNetlinkMessage::NL_AUTO_SEQ,
                              GenericNetlinkMessage::NLMSG_MIN_TYPE, 0, 0,
                              HWSIM_CMD_FRAME, 0);
    const auto& info = frame->info();
    msg.putAttribute(HWSIM_ATTR_ADDR_RECEIVER, mFrameInfo.mTransmitter.mAddr,
                     ETH_ALEN);
    msg.putAttribute(HWSIM_ATTR_FRAME, frame->data(), frame->size());
    uint32_t rateIdx = 1;
    msg.putAttribute(HWSIM_ATTR_RX_RATE, &rateIdx, sizeof(uint32_t));
    uint32_t signal = -50;
    msg.putAttribute(HWSIM_ATTR_SIGNAL, &signal, sizeof(signal));
    msg.putAttribute(HWSIM_ATTR_FREQ, &info.mChannel, sizeof(uint32_t));
    AutoLock lock(mLock);
    return mOnFrameAvailableCallback(msg.data(), msg.dataLen());
}

ssize_t VirtioWifiForwarder::onRxPacketAvailable(const uint8_t* buf,
                                                 size_t size) {
    std::unique_ptr<Ieee80211Frame> frame =
            Ieee80211Frame::buildFromEthernet(buf, size, mBssID);
    if (frame == nullptr) {
        LOG(DEBUG) << "Unable to convert from Ethernet to Ieee80211.";
        return -1;
    }
    // encrypt will be no-op if cipher scheme is none.
    if (!frame->encrypt(mHostapd->getCipherScheme())) {
        LOG(ERROR) << "Unable to encrypt the IEEE80211 frame with CCMP.";
        return 0;
    }
    auto& info = frame->info();
    info = mFrameInfo;
    return sendToGuest(std::move(frame));
}

void VirtioWifiForwarder::onHostApd(void* opaque, int fd, unsigned events) {
    if ((events & android::base::Looper::FdWatch::kEventRead) == 0) {
        // Not interested in this event
        return;
    }
    auto forwarder = static_cast<VirtioWifiForwarder*>(opaque);

    static uint8_t buf[Ieee80211Frame::MAX_FRAME_LEN];
    ssize_t size = socketRecv(fd, buf, sizeof(buf));
    if (size < 0) {
        return;
    }
    auto frame =
            std::make_unique<Ieee80211Frame>(buf, size, forwarder->mFrameInfo);
    if (frame->isBeacon()) {
        forwarder->mBeaconFrame = std::move(frame);
        struct ieee80211_mgmt* hdr = (struct ieee80211_mgmt*)buf;
        Looper::Duration newBeaconInt = hdr->u.beacon.beacon_int * kTUtoMS;
        if (newBeaconInt != forwarder->mBeaconIntMs) {
            forwarder->mBeaconIntMs = newBeaconInt;
            forwarder->resetBeaconTask();
            forwarder->mBeaconTask->start();
        }
    } else {
        forwarder->sendToGuest(std::move(frame));
    }
}

void VirtioWifiForwarder::resetBeaconTask() {
    // set up periodic job of sending beacons.
    mBeaconTask.emplace(
            mLooper,
            [this]() {
                if (mBeaconFrame != nullptr && mBeaconFrame->isBeacon()) {
                    sendToGuest(std::make_unique<Ieee80211Frame>(
                            mBeaconFrame->data(), mBeaconFrame->size(),
                            mBeaconFrame->info()));
                }
                return true;
            },
            mBeaconIntMs);
}
size_t VirtioWifiForwarder::onRemoteData(const uint8_t* data, size_t size) {
    size_t offset = 0;
    while (offset < size) {
        const WifiForwardHeader* header =
                reinterpret_cast<const WifiForwardHeader*>(data + offset);
        // Magic or version is wrong.
        if (header->magic != kWifiForwardMagic ||
            header->version != kWifiForwardVersion) {
            std::string errStr = StringFormat(
                    "onRemoteData skip data at offset %zu size %zu due to "
                    "incorrect magic or version number\n",
                    offset, size);
            LOG(DEBUG) << errStr;
            offset += sizeof(WifiForwardHeader);
            continue;
        }
        // Either data offset or full lenth is wrong or type is wrong.
        if (header->dataOffset < sizeof(struct WifiForwardHeader) ||
            header->dataOffset > header->fullLength) {
            offset += sizeof(WifiForwardHeader);
            std::string errStr = StringFormat(
                    "onRemoteData skip data at offset %zu size %zu due to "
                    "incorrect header offset or length\n",
                    offset, size);
            LOG(DEBUG) << errStr;
            continue;
        }
        if (offset + header->fullLength <= size) {
            const uint8_t* payload = data + offset + header->dataOffset;
            size_t payloadSize = header->fullLength - header->dataOffset;
            if (payloadSize > 0) {  // Data frame type
                // The total amount of data is bigger than payload size due to
                // generic netlink message.
                FrameInfo info(header->transmitter, header->cookie,
                               header->flags, header->channel, header->rates,
                               header->numRates);
                auto frame = std::make_unique<Ieee80211Frame>(
                        payload, payloadSize, info);
                if (sendToGuest(std::move(frame)) >= payloadSize) {
                    offset += header->fullLength;
                } else {
                    break;
                }
            } else {  // Ack frame type
                offset += header->dataOffset;
            }

        } else {
            // In case of partial frame is received, do not consume the data.
            // Ideally, retranmission should fix it by mRemotePeer.
            break;
        }
    }
    if (offset != size) {
        LOG(DEBUG) << "onRemoteData sends partial data. Size: " << size
                     << " actually sent: " << offset;
        return offset;
    } else {
        return size;
    }
}

void VirtioWifiForwarder::sendToRemoteVM(
        std::unique_ptr<android::network::Ieee80211Frame> frame,
        FrameType type) {
    if (mRemotePeer == nullptr) {
        return;
    }
    auto& info = frame->info();
    WifiForwardHeader hdr(type, info.mTransmitter,
                          frame->size() + sizeof(WifiForwardHeader),
                          info.mCookie, info.mFlags, info.mChannel,
                          Ieee80211Frame::TX_MAX_RATES, info.mTxRates.data());
    mRemotePeer->queue((uint8_t*)&hdr, hdr.dataOffset);
    mRemotePeer->queue(frame->data(), frame->size());
}

// Before the packet is sent to QEMU NIC, it needs to be converted
// from IEEE80211 frame to Ethernet frame.
int VirtioWifiForwarder::sendToNIC(
        std::unique_ptr<android::network::Ieee80211Frame> frame) {
    if (frame->isDataNull()) {
        return 0;
    }

#ifdef LIBSLIRP
    assert(mSlirp);
    auto packet = frame->toEthernet();
    slirp_input(mSlirp, packet.data(), packet.size());
    return packet.size();
#else
    if (!mNic) {
        return 0;
    }
    int res;
    auto outSg = frame->toEthernetIOVector();
    if (outSg.size() == 0) {
        return 0;
    }
    if (mOnFrameSentCallback) {
        res = qemu_sendv_packet_async(
                qemu_get_subqueue(mNic, 0), &outSg[0], outSg.size(),
                &VirtioWifiForwarder::onFrameSentCallback);
    } else {
        res = qemu_sendv_packet(qemu_get_subqueue(mNic, 0), &outSg[0],
                                outSg.size());
    }
    return res ? 0 : -EBUSY;
#endif
}

std::unique_ptr<Ieee80211Frame> VirtioWifiForwarder::parseHwsimCmdFrame(
        const GenericNetlinkMessage& msg) {
    MacAddress transmitter;
    if (!msg.getAttribute(HWSIM_ATTR_ADDR_TRANSMITTER, transmitter.mAddr,
                          ETH_ALEN)) {
        return nullptr;
    }
    struct iovec iov = msg.getAttribute(HWSIM_ATTR_FRAME);
    if (iov.iov_len < IEEE80211_HDRLEN) {
        return nullptr;
    }
    uint32_t flags;
    if (!msg.getAttribute(HWSIM_ATTR_FLAGS, &flags, sizeof(uint32_t))) {
        return nullptr;
    }
    uint32_t channel;
    if (!msg.getAttribute(HWSIM_ATTR_FREQ, &channel, sizeof(uint32_t))) {
        return nullptr;
    }
    hwsim_tx_rate rates[Ieee80211Frame::TX_MAX_RATES];
    if (!msg.getAttribute(
                HWSIM_ATTR_TX_INFO, rates,
                sizeof(struct hwsim_tx_rate) * Ieee80211Frame::TX_MAX_RATES)) {
        return nullptr;
    }
    hwsim_tx_rate_flag rate_flags[Ieee80211Frame::TX_MAX_RATES];
    if (!msg.getAttribute(HWSIM_ATTR_TX_INFO_FLAGS, rate_flags,
                          sizeof(struct hwsim_tx_rate_flag) *
                                  Ieee80211Frame::TX_MAX_RATES)) {
        return nullptr;
    }
    uint64_t cookie;
    if (!msg.getAttribute(HWSIM_ATTR_COOKIE, &cookie, sizeof(uint64_t))) {
        return nullptr;
    }
    mFrameInfo = FrameInfo(transmitter, cookie, flags, channel, rates,
                           Ieee80211Frame::TX_MAX_RATES);
    return std::make_unique<Ieee80211Frame>(
            static_cast<const uint8_t*>(iov.iov_base), iov.iov_len, mFrameInfo);
}

}  // namespace qemu2
}  // namespace android

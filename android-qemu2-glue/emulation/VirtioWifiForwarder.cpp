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

#include "android-qemu2-glue/emulation/VirtioWifiForwarder.h"

#include "android-qemu2-glue/emulation/HostapdController.h"
#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/network/Endian.h"
#include "android/network/WifiForwardClient.h"
#include "android/network/WifiForwardPeer.h"
#include "android/network/WifiForwardPipe.h"
#include "android/network/WifiForwardServer.h"

using android::base::IOVector;
using android::base::ScopedSocket;
using android::base::socketCreatePair;
using android::base::socketRecv;
using android::base::socketSend;
using android::base::StringFormat;
using android::base::ThreadLooper;
using android::network::Ieee80211Frame;
using android::network::MacAddress;
using android::network::WifiForwardClient;
using android::network::WifiForwardMode;
using android::network::WifiForwardPeer;
using android::network::WifiForwardServer;

namespace fc = android::featurecontrol;

extern "C" {
#include "qemu/osdep.h"
#include "net/net.h"
#include "common/ieee802_11_defs.h"

/* The port to use for WiFi forwarding as a server */
extern uint16_t android_wifi_server_port;

/* The port to use for WiFi forwarding as a client */
extern uint16_t android_wifi_client_port;

}  // extern "C"

namespace android {
namespace qemu2 {

enum class FrameHeaderType : std::uint8_t {
    DATA = 0,
    ACK,
};

struct FrameHeader {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t dataOffset;
    uint32_t fullLength;
} __attribute__((__packed__));

struct ieee8023_hdr {
    uint8_t dest[ETH_ALEN]; /* destination eth addr */
    uint8_t src[ETH_ALEN];  /* source ether addr    */
    uint16_t ethertype;     /* packet type ID field */
} __attribute__((__packed__));

const char* const VirtioWifiForwarder::kNicModel = "virtio-wifi-device";
const char* const VirtioWifiForwarder::kNicName = "virtio-wifi-device";

VirtioWifiForwarder::VirtioWifiForwarder(
        const uint8_t* bssid,
        OnFrameAvailableCallback cb,
        OnLinkStatusChanged onLinkStatusChanged,
        CanReceive canReceive,
        ::NICConf* conf,
        OnFrameSentCallback onFrameSentCallback)
    : mBssID(MACARG(bssid)),
      mOnFrameAvailableCallback(std::move(cb)),
      mOnLinkStatusChanged(std::move(onLinkStatusChanged)),
      mCanReceive(std::move(canReceive)),
      mOnFrameSentCallback(std::move(onFrameSentCallback)),
      mLooper(ThreadLooper::get()),
      mNicConf(conf),
      mRemotePeer(nullptr) {}

VirtioWifiForwarder::~VirtioWifiForwarder() {
    stop();
}

bool VirtioWifiForwarder::init() {
    // init socket pair.
    int fds[2];
    if (socketCreatePair(&fds[0], &fds[1])) {
        return false;
    }
    mVirtIOSock = ScopedSocket(fds[1]);
    HostapdController::getInstance()->setDriverSocket(ScopedSocket(fds[0]));
    //  Looper FdWatch and set callback functions
    mFdWatch = mLooper->createFdWatch(mVirtIOSock.get(),
                                      &VirtioWifiForwarder::onHostApd, this);
    mFdWatch->wantRead();
    mFdWatch->dontWantWrite();

    // init Qemu Nic
    static NetClientInfo info = {
            .type = NET_CLIENT_DRIVER_NIC,
            .size = sizeof(NICState),
            .receive = &VirtioWifiForwarder::onNICFrameAvailable,
            .can_receive = &VirtioWifiForwarder::canReceive,
            .link_status_changed = &VirtioWifiForwarder::onLinkStatusChanged,
    };

    mNic = qemu_new_nic(&info, mNicConf, kNicModel, kNicName, this);

    // init WifiFordPeer for P2P network.
    auto onData = [this](const uint8_t* data, size_t size) {
        return this->onRemoteData(data, size);
    };

    if (android_wifi_client_port > 0) {
        mRemotePeer.reset(
                new WifiForwardClient(android_wifi_client_port, onData));
    } else if (android_wifi_server_port > 0) {
        mRemotePeer.reset(
                new WifiForwardServer(android_wifi_server_port, onData));
    }
    if (mRemotePeer != nullptr) {
        mRemotePeer->init();
        mRemotePeer->run();
    }
    return true;
}

int VirtioWifiForwarder::forwardFrame(IOVector sg, bool toRemoteVM) {
    size_t size = sg.summedLength();

    if (size < IEEE80211_HDRLEN) {
        return 0;
    }
    Ieee80211Frame frame(size);
    sg.copyTo(frame.data(), 0, size);
    const MacAddress addr1 = frame.addr1();
    if (toRemoteVM) {
        sendToRemoteVM(frame);
        return 0;
    }

    int res = 0;
    // Data frames
    if (frame.isData()) {
        if (addr1 != mBssID) {
            sendToRemoteVM(frame);
        } else if (frame.isToDS() && !frame.isFromDS()) {
            res = sendToNIC(frame);
        }
    } else {  // Management frames.
        // If the frame is broadcast or multicast, forward it to both hostapd
        // and remote VM
        if (addr1.isBroadcast() || addr1.isMulticast() || addr1 == mBssID) {
            // TODO (wdu@) Experiement with shared memory approach
            if (socketSend(mVirtIOSock.get(), frame.data(), size) < 0) {
                LOG(VERBOSE) << "Failed to send frame to hostapd.";
            }
        }
        if (addr1.isBroadcast() || addr1.isMulticast() || addr1 != mBssID) {
            sendToRemoteVM(frame);
        }
    }
    return res;
}

void VirtioWifiForwarder::stop() {
    HostapdController::getInstance()->terminate(false);
    mNic = nullptr;
    if (mRemotePeer) {
        mRemotePeer->stop();
        mRemotePeer.reset(nullptr);
    }
}

VirtioWifiForwarder* VirtioWifiForwarder::getInstance(NetClientState* nc) {
    return static_cast<VirtioWifiForwarder*>(qemu_get_nic_opaque(nc));
}

int VirtioWifiForwarder::canReceive(NetClientState* nc) {
    auto forwarder = getInstance(nc);
    return forwarder->mCanReceive(nc);
}

void VirtioWifiForwarder::onLinkStatusChanged(NetClientState* nc) {
    auto forwarder = getInstance(nc);
    forwarder->mOnLinkStatusChanged(nc);
}

void VirtioWifiForwarder::onFrameSentCallback(NetClientState* nc,
                                              ssize_t size) {
    auto forwarder = getInstance(nc);
    if (forwarder->mOnFrameSentCallback) {
        forwarder->mOnFrameSentCallback(nc, size);
    }
}

// QEMU NIC client will receive one ethernet packet at a time.
ssize_t VirtioWifiForwarder::onNICFrameAvailable(NetClientState* nc,
                                                 const uint8_t* buf,
                                                 size_t size) {
    if (size < ETH_HLEN) {
        return -1;
    }
    auto forwarder = getInstance(nc);
    if (!forwarder->mCanReceive(nc)) {
        return -1;
    }
    Ieee80211Frame frame(std::vector<uint8_t>(buf, buf + size),
                         forwarder->mBssID);
    return forwarder->mOnFrameAvailableCallback(frame.data(), frame.size(),
                                                false);
}

void VirtioWifiForwarder::onHostApd(void* opaque, int fd, unsigned events) {
    if ((events & android::base::Looper::FdWatch::kEventRead) == 0) {
        // Not interested in this event
        return;
    }
    auto forwarder = static_cast<VirtioWifiForwarder*>(opaque);

    uint8_t buf[Ieee80211Frame::MAX_FRAME_LEN];
    ssize_t size = socketRecv(fd, buf, sizeof(buf));
    if (size < 0) {
        return;
    }
    forwarder->mOnFrameAvailableCallback(buf, size, false);
}

size_t VirtioWifiForwarder::onRemoteData(const uint8_t* data, size_t size) {
    size_t offset = 0;
    while (offset < size) {
        struct FrameHeader* header = (struct FrameHeader*)(data + offset);
        // Magic or version is wrong.
        if (header->magic != kWifiForwardMagic ||
            header->version != kWifiForwardVersion) {
            std::string errStr = StringFormat(
                    "onRemoteData skip data at offset %zu size %zu due to "
                    "incorrect magic or version number\n",
                    offset, size);
            LOG(VERBOSE) << errStr;
            offset += sizeof(struct FrameHeader);
            continue;
        }
        // Either data offset or full lenth is wrong or type is wrong.
        if (header->dataOffset < sizeof(struct FrameHeader) ||
            header->dataOffset > header->fullLength ||
            header->type != (uint8_t)FrameHeaderType::DATA) {
            offset += sizeof(struct FrameHeader);
            std::string errStr = StringFormat(
                    "onRemoteData skip data at offset %zu size %zu due to "
                    "incorrect header offset or length\n",
                    offset, size);
            LOG(VERBOSE) << errStr;
            continue;
        }
        if (offset + header->fullLength <= size) {
            const uint8_t* payload = data + offset + header->dataOffset;
            size_t payloadSize = header->fullLength - header->dataOffset;
            if (payloadSize > 0) {
                if (mOnFrameAvailableCallback(payload, payloadSize, true) ==
                    payloadSize) {
                    offset += header->fullLength;
                } else {
                    break;
                }
            } else {  // In case there is an empty frame.
                offset += header->dataOffset;
            }

        } else {
            // In case of partial frame is received, do not consume the data.
            // Ideally, retranmission should fix it by mRemotePeer.
            break;
        }
    }
    if (offset != size) {
        LOG(VERBOSE) << "onRemoteData sends partial data. Size: " << size
                     << " actually sent: " << offset;
        return offset;
    } else {
        return size;
    }
}

void VirtioWifiForwarder::sendToRemoteVM(Ieee80211Frame& frame) {
    if (mRemotePeer == nullptr) {
        return;
    }
    FrameHeader fhdr;
    fhdr.magic = kWifiForwardMagic;
    fhdr.version = kWifiForwardVersion;
    fhdr.type = (uint8_t)FrameHeaderType::DATA;
    fhdr.dataOffset = sizeof(struct FrameHeader);
    fhdr.fullLength = frame.size() + sizeof(struct FrameHeader);
    mRemotePeer->queue((uint8_t*)&fhdr, fhdr.dataOffset);
    mRemotePeer->queue(frame.data(), frame.size());
}

// Before the packet is sent to QEMU NIC, it needs to be converted
// from IEEE80211 frame to Ethernet frame.
int VirtioWifiForwarder::sendToNIC(Ieee80211Frame& frame) {
    if (mNic == nullptr) {
        return 0;
    }
    if (frame.isDataNull()) {
        return 0;
    }
    auto outSg = frame.toEthernet();
    if (outSg.size() == 0) {
        return 0;
    }
    // Send to QEMU NIC.
    int res;
    if (mOnFrameSentCallback) {
        res = qemu_sendv_packet_async(
                qemu_get_subqueue(mNic, 0), &outSg[0], outSg.size(),
                &VirtioWifiForwarder::onFrameSentCallback);
    } else {
        res = qemu_sendv_packet(qemu_get_subqueue(mNic, 0), &outSg[0],
                                outSg.size());
    }
    return res ? 0 : -EBUSY;
}

}  // namespace qemu2
}  // namespace android

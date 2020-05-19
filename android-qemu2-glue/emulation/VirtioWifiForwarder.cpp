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

#include "android-qemu2-glue/utils/Endian.h"
#include "android/base/Log.h"
#include "android/base/StringFormat.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/network/WifiForwardClient.h"
#include "android/network/WifiForwardPeer.h"
#include "android/network/WifiForwardPipe.h"
#include "android/network/WifiForwardServer.h"

using android::base::ScopedSocket;
using android::base::socketCreatePair;
using android::base::socketRecv;
using android::base::socketSend;
using android::base::StringFormat;
using android::base::ThreadLooper;
using android::network::MacAddress;
using android::network::WifiForwardClient;
using android::network::WifiForwardMode;
using android::network::WifiForwardPeer;
using android::network::WifiForwardServer;
using android::qemu2::Ieee80211Frame;

namespace fc = android::featurecontrol;

/* Maximum packet size we can transmit/receive */
#define VIRTQUEUE_MAX_SIZE 10
#define ETH_P_ARP (0x0806) /* Address Resolution packet */
#define ETH_P_IPV6 (0x86dd)
#define ETH_P_NCSI (0x88f8)

extern "C" {

// Undefine MACRO ARRAY_SIZE to avoid name clash and function
// redefinitions.
#undef ARRAY_SIZE
#include "common/ieee802_11_defs.h"
#include "drivers/driver_virtio_wifi.h"

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

/* See IEEE 802.1H for LLC/SNAP encapsulation/decapsulation */
/* Ethernet-II snap header (RFC1042 for most EtherTypes) */
static constexpr uint8_t kRfc1042Header[] = {0xaa, 0xaa, 0x03,
                                             0x00, 0x00, 0x00};

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

bool VirtioWifiForwarder::init() {
    // init socket pair.
    int fds[2];
    if (socketCreatePair(&fds[0], &fds[1])) {
        return false;
    }
    mHostApdSock = ScopedSocket(fds[0]);
    mVirtIOSock = ScopedSocket(fds[1]);
    ::set_virtio_sock(mHostApdSock.get());
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

    mNic.reset(::qemu_new_nic(&info, mNicConf, kNicModel, kNicName, this));

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

int VirtioWifiForwarder::forwardFrame(struct iovec* sg,
                                      size_t num,
                                      bool toRemoteVM) {
    size_t size = ::iov_size(sg, num);
    if (size < IEEE80211_HDRLEN) {
        return 0;
    }
    uint8_t buf[Ieee80211Frame::MAX_FRAME_LEN];
    ::iov_to_buf(sg, num, 0, buf, Ieee80211Frame::MAX_FRAME_LEN);
    Ieee80211Frame frame(buf, size);
    const MacAddress addr1 = frame.addr1();
    if (toRemoteVM) {
        sendToRemoteVM(frame);
        return 0;
    }

    int res = 0;
    if (frame.isData()) {
        if (addr1 != mBssID) {
            sendToRemoteVM(frame);
        } else if (frame.isToDS() && !frame.isFromDS()) {
            res = sendToNIC(sg, num, frame);
        }
    } else {  // Management frames
        if (addr1.isBroadcast() || addr1.isMulticast() || addr1 == mBssID) {
            // TODO (wdu@) Experiement with shared memory approach
            if (socketSend(mVirtIOSock.get(), buf, size) < 0) {
                LOG(ERROR) << "Failed to send frame to hostapd.";
            }
        }
        if (addr1.isBroadcast() || addr1.isMulticast() || addr1 != mBssID) {
            sendToRemoteVM(frame);
        }
    }
    return res;
}

void VirtioWifiForwarder::stop() {
    ::qemu_del_nic(mNic.get());
    mRemotePeer->stop();
}

/*static ssize_t ieee80211_data_from_8023(const uint8_t* buf,
                                        VirtQueueElement* elem,
                                        size_t orig_size) {
    struct ieee80211_hdr hdr;
    le16 ethertype, fc;
    const u8* encaps_data;
    struct iovec* sg;
    size_t encaps_len, skip_header_bytes, total;
    size_t hdrlen = IEEE80211_HDRLEN;
    if (orig_size < ETH_HLEN) {
        return 0;
    }
    ethertype = (buf[12] << 8) | buf[13];
    skip_header_bytes = ETH_HLEN;
    if (ethertype >= 0x0600) {
        encaps_data = rfc1042_header;
        encaps_len = sizeof(rfc1042_header);
        skip_header_bytes -= 2;
    } else {
        encaps_data = NULL;
        encaps_len = 0;
    }
    fc = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
    fc |= WLAN_FC_FROMDS;
    // Order of addresses: DA BSSID SA
    memcpy(hdr.addr1, buf, ETH_ALEN);
    memcpy(hdr.addr2, s_bssid, ETH_ALEN);
    memcpy(hdr.addr3, buf + ETH_ALEN, ETH_ALEN);
    hdr.frame_control = fc;
    hdr.duration_id = 0;
    hdr.seq_ctrl = 0;
    sg = elem->in_sg;
    total = iov_from_buf(sg, elem->in_num, 0, (const void*)&hdr,
                         IEEE80211_HDRLEN);
    if (encaps_len) {
        total += iov_from_buf(sg, elem->in_num, hdrlen, encaps_data,
encaps_len);
    }
    total += iov_from_buf(sg, elem->in_num, hdrlen + encaps_len,
                          buf + skip_header_bytes,
                          orig_size - skip_header_bytes);
    return total;
}*/

int VirtioWifiForwarder::canReceive(::NetClientState* nc) {
    auto forwarder = static_cast<VirtioWifiForwarder*>(qemu_get_nic_opaque(nc));
    return forwarder->mCanReceive(nc);
}

void VirtioWifiForwarder::onLinkStatusChanged(::NetClientState* nc) {
    auto forwarder = static_cast<VirtioWifiForwarder*>(qemu_get_nic_opaque(nc));
    forwarder->mOnLinkStatusChanged(nc);
}

void VirtioWifiForwarder::onFrameSentCallback(NetClientState* nc,
                                              ssize_t size) {
    auto forwarder = static_cast<VirtioWifiForwarder*>(qemu_get_nic_opaque(nc));
    if (forwarder->mOnFrameSentCallback) {
        forwarder->mOnFrameSentCallback(nc, size);
    }
}
ssize_t VirtioWifiForwarder::onNICFrameAvailable(::NetClientState* nc,
                                                 const uint8_t* buf,
                                                 size_t size) {
    if (size < ETH_HLEN) {
        return -1;
    }
    auto forwarder = static_cast<VirtioWifiForwarder*>(qemu_get_nic_opaque(nc));

    if (!forwarder->mCanReceive(nc)) {
        return -1;
    }

    ssize_t offset = 0;
    while (offset < size) {
        uint16_t ethertype = (buf[12] << 8) | buf[13];
        size_t skip_header_bytes = ETH_HLEN;
        size_t encaps_len = 0;
        if (ethertype >= 0x0600) {
            encaps_len = ETH_ALEN;
            skip_header_bytes -= 2;
        }
        uint16_t fc = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
        fc |= WLAN_FC_FROMDS;
        struct ieee80211_hdr hdr;
        // Order of addresses: DA BSSID SA
        ::memcpy(hdr.addr1, buf, ETH_ALEN);
        ::memcpy(hdr.addr2, &(forwarder->mBssID[0]), ETH_ALEN);
        ::memcpy(hdr.addr3, buf + ETH_ALEN, ETH_ALEN);
        hdr.frame_control = fc;
        hdr.duration_id = 0;
        hdr.seq_ctrl = 0;
        struct iovec outSg[VIRTQUEUE_MAX_SIZE + 1];
        outSg[0].iov_base = &hdr;
        outSg[0].iov_len = IEEE80211_HDRLEN;
        size_t outNum = 1;
        if (encaps_len) {
            outSg[outNum].iov_base = (uint8_t*)kRfc1042Header;
            outSg[outNum].iov_len = encaps_len;
            outNum += 1;
        }
        outSg[outNum].iov_base = (uint8_t*)(buf + skip_header_bytes);
        outSg[outNum].iov_len = size - skip_header_bytes;
        outNum += 1;
        uint8_t buf[Ieee80211Frame::MAX_FRAME_LEN];
        size_t s = iov_to_buf(outSg, outNum, 0, buf,
                              Ieee80211Frame::MAX_FRAME_LEN);
        forwarder->mOnFrameAvailableCallback(buf, s, false);
        offset += s;
    }
    return offset;
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
                    "onRemoteData skip data at line %d offset %zu size %zu\n",
                    __LINE__, offset, size);
            LOG(ERROR) << errStr;
            offset += sizeof(struct FrameHeader);
            continue;
        }
        // Either data offset or full lenth is wrong or type is wrong.
        if (header->dataOffset < sizeof(struct FrameHeader) ||
            header->dataOffset > header->fullLength ||
            header->type != (uint8_t)FrameHeaderType::DATA) {
            offset += sizeof(struct FrameHeader);
            std::string errStr = StringFormat(
                    "onRemoteData skip data at line %d offset %zu size %zu\n",
                    __LINE__, offset, size);
            LOG(ERROR) << errStr;
            continue;
        }
        if (offset + header->fullLength <= size) {
            const uint8_t* payload = data + offset + header->dataOffset;
            size_t payloadSize = header->fullLength - header->dataOffset;
            if (payloadSize > 0) {
                mOnFrameAvailableCallback(payload, payloadSize, true);
            }
            offset += header->fullLength;
        } else {
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
    std::unique_ptr<struct FrameHeader> fhdr(new FrameHeader);
    fhdr->magic = kWifiForwardMagic;
    fhdr->version = kWifiForwardVersion;
    fhdr->type = (uint8_t)FrameHeaderType::DATA;
    fhdr->dataOffset = sizeof(struct FrameHeader);
    fhdr->fullLength = frame.size() + sizeof(struct FrameHeader);
    mRemotePeer->queue((uint8_t*)fhdr.get(), fhdr->dataOffset);
    mRemotePeer->queue(frame.data(), frame.size());
}

// Before the packet is sent to QEMU NIC, it needs to be converted
// from IEEE80211 frame to Ethernet frame.
int VirtioWifiForwarder::sendToNIC(struct iovec* sg,
                                   size_t num,
                                   Ieee80211Frame& frame) {
    if (frame.isDataNull()) {
        return 0;
    }
    uint8_t* frameBody = frame.frameBody();
    uint16_t ethertype = (frameBody[0] << 8) | frameBody[1];
    if (ethertype != ETH_P_IPV6 && ethertype != ETH_P_IP &&
        ethertype != ETH_P_ARP && ethertype != ETH_P_NCSI) {
        LOG(ERROR) << "Unexpected ether type. Dump frame: " << frame.toStr();
        return 0;
    }
    std::unique_ptr<struct ieee8023_hdr> eth(new struct ieee8023_hdr);
    eth->ethertype = htons(ethertype);
    // Order of addresses: RA SA DA
    MacAddress addr2 = frame.addr2();
    MacAddress addr3 = frame.addr3();
    memcpy(eth->dest, &(addr3[0]), ETH_ALEN);
    memcpy(eth->src, &(addr2[0]), ETH_ALEN);
    struct iovec outSg[VIRTQUEUE_MAX_SIZE + 1];
    outSg[0].iov_base = eth.get();
    outSg[0].iov_len = sizeof(struct ieee8023_hdr);
    size_t outNum = ::iov_copy(&outSg[1], VIRTQUEUE_MAX_SIZE, sg, num,
                               frame.hdrLength() + sizeof(kRfc1042Header) +
                                       sizeof(eth->ethertype),
                               iov_size(sg, num)) +
                    1;
    // Send to QEMU NIC.
    int res;
    if (mOnFrameSentCallback) {
        res = ::qemu_sendv_packet_async(
                qemu_get_subqueue(mNic.get(), 0), outSg, outNum,
                &VirtioWifiForwarder::onFrameSentCallback);
    } else {
        res = ::qemu_sendv_packet(qemu_get_subqueue(mNic.get(), 0), outSg,
                                  outNum);
    }
    return res ? 0 : -EBUSY;
}

}  // namespace qemu2
}  // namespace android

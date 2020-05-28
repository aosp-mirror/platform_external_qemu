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

#include "android/network/Ieee80211Frame.h"
#include "android/base/Log.h"
#include "android/network/Endian.h"

#include <iomanip>
#include <sstream>

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#elif defined(__linux__)
#include <arpa/inet.h>
#endif

extern "C" {
#include "common/ieee802_11_defs.h"
}

#define ETH_P_ARP (0x0806) /* Address Resolution packet */
#define ETH_P_IPV6 (0x86dd)
#define ETH_P_NCSI (0x88f8)

using namespace android::base;
struct ieee8023_hdr {
    uint8_t dest[ETH_ALEN]; /* destination eth addr */
    uint8_t src[ETH_ALEN];  /* source ether addr    */
    uint16_t ethertype;     /* packet type ID field */
} __attribute__((__packed__));

/* See IEEE 802.1H for LLC/SNAP encapsulation/decapsulation */
/* Ethernet-II snap header (RFC1042 for most EtherTypes) */
static constexpr uint8_t kRfc1042Header[] = {0xaa, 0xaa, 0x03,
                                             0x00, 0x00, 0x00};

namespace android {
namespace network {

// Frame Header layout as below
//       2         2           6        6        6       2       6
// Frame-Ctl | Duration ID | Addr1 | Addr2 | Addr3 | Seq-Ctl | Addr4 | Frame
// Body
Ieee80211Frame::Ieee80211Frame(const uint8_t* data, size_t size)
    : mData(data, data + size) {}

Ieee80211Frame::Ieee80211Frame(size_t size) : mData(size) {}

Ieee80211Frame::Ieee80211Frame(const std::vector<uint8_t>& ethernet,
                               MacAddress bssid) {
    uint16_t ethertype = (ethernet[12] << 8) | ethernet[13];
    size_t skippedBytes = ETH_HLEN;
    size_t encapLen = 0;

    if (validEtherType(ethertype)) {
        encapLen = ETH_ALEN;
        skippedBytes -= sizeof(ethertype);
    }

    uint16_t fc = IEEE80211_FC(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
    fc |= WLAN_FC_FROMDS;
    struct ieee80211_hdr hdr;
    memcpy(hdr.addr1, ethernet.data(), ETH_ALEN);
    memcpy(hdr.addr2, &bssid[0], ETH_ALEN);
    memcpy(hdr.addr3, ethernet.data() + ETH_ALEN, ETH_ALEN);
    hdr.frame_control = fc;
    hdr.duration_id = 0;
    hdr.seq_ctrl = 0;

    mData.insert(mData.end(), (uint8_t*)&hdr,
                 (uint8_t*)&hdr + IEEE80211_HDRLEN);

    if (encapLen) {
        mData.insert(mData.end(), kRfc1042Header, kRfc1042Header + encapLen);
    }
    mData.insert(mData.end(), ethernet.begin() + skippedBytes,
                 ethernet.end());
}

MacAddress Ieee80211Frame::addr1() const {
    return MacAddress(MACARG(hdr().addr1));
}

MacAddress Ieee80211Frame::addr2() const {
    return MacAddress(MACARG(hdr().addr2));
}

MacAddress Ieee80211Frame::addr3() const {
    return MacAddress(MACARG(hdr().addr3));
}

MacAddress Ieee80211Frame::addr4() const {
    if (uses4Addresses()) {
        return MacAddress(MACARG(mData.data() + IEEE80211_HDRLEN));
    } else {
        return MacAddress();
    }
}

const ieee80211_hdr& Ieee80211Frame::hdr() const {
    const ieee80211_hdr* hdr = (ieee80211_hdr*)mData.data();
    return *hdr;
}

size_t Ieee80211Frame::hdrLength() const {
    return uses4Addresses() ? (IEEE80211_HDRLEN + ETH_ALEN) : IEEE80211_HDRLEN;
}

uint8_t* Ieee80211Frame::frameBody() {
    return mData.data() +
           (IEEE80211_HDRLEN + (uses4Addresses() ? ETH_ALEN : 0));
}

const uint8_t* Ieee80211Frame::frameBody() const {
    return mData.data() +
           (IEEE80211_HDRLEN + (uses4Addresses() ? ETH_ALEN : 0));
}

std::string Ieee80211Frame::toStr() {
    std::stringstream ss;
    uint8_t type = WLAN_FC_GET_TYPE(hdr().frame_control);
    uint8_t subType = WLAN_FC_GET_STYPE(hdr().frame_control);
    switch (type) {
        case WLAN_FC_TYPE_MGMT:
            // Management
            ss << "Management (";
            switch (subType) {
                case WLAN_FC_STYPE_ASSOC_REQ:
                    ss << "Association Request";
                    break;
                case WLAN_FC_STYPE_ASSOC_RESP:
                    ss << "Association Response";
                    break;
                case WLAN_FC_STYPE_REASSOC_REQ:
                    ss << "Reassociation Request";
                    break;
                case WLAN_FC_STYPE_REASSOC_RESP:
                    ss << "Reassociation Response";
                    break;
                case WLAN_FC_STYPE_PROBE_REQ:
                    ss << "Probe Request";
                    break;
                case WLAN_FC_STYPE_PROBE_RESP:
                    ss << "Probe Response";
                    break;
                case 6:
                    ss << "Timing Advertisement";
                    break;
                case WLAN_FC_STYPE_BEACON:
                    ss << "Beacon";
                    break;
                case WLAN_FC_STYPE_ATIM:
                    ss << "ATIM";
                    break;
                case WLAN_FC_STYPE_DISASSOC:
                    ss << "Disassociation";
                    break;
                case WLAN_FC_STYPE_AUTH:
                    ss << "Authentication";
                    break;
                case WLAN_FC_STYPE_DEAUTH:
                    ss << "Deauthentication";
                    break;
                case WLAN_FC_STYPE_ACTION:
                    ss << "Action";
                    break;
                case 14:
                    ss << "Action No Ack";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        case WLAN_FC_TYPE_CTRL:
            // Control
            ss << "Control (";
            switch (subType) {
                case 7:
                    ss << "Control Wrapper";
                    break;
                case 8:
                    ss << "Block Ack Request";
                    break;
                case 9:
                    ss << "Block Ack";
                    break;
                case WLAN_FC_STYPE_PSPOLL:
                    ss << "PS-Poll";
                    break;
                case WLAN_FC_STYPE_RTS:
                    ss << "RTS";
                    break;
                case WLAN_FC_STYPE_CTS:
                    ss << "CTS";
                    break;
                case WLAN_FC_STYPE_ACK:
                    ss << "Ack";
                    break;
                case WLAN_FC_STYPE_CFEND:
                    ss << "CF-End";
                    break;
                case WLAN_FC_STYPE_CFENDACK:
                    ss << "CF-End+CF-Ack";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        case WLAN_FC_TYPE_DATA:
            // Data
            ss << "Data (";
            switch (subType) {
                case WLAN_FC_STYPE_DATA:
                    ss << "Data";
                    break;
                case WLAN_FC_STYPE_DATA_CFACK:
                    ss << "Data+CF-Ack";
                    break;
                case WLAN_FC_STYPE_DATA_CFPOLL:
                    ss << "Data+CF-Poll";
                    break;
                case WLAN_FC_STYPE_DATA_CFACKPOLL:
                    ss << "Data+CF-Ack+CF-Poll";
                    break;
                case WLAN_FC_STYPE_NULLFUNC:
                    ss << "Null";
                    break;
                case WLAN_FC_STYPE_CFACK:
                    ss << "CF-Ack";
                    break;
                case WLAN_FC_STYPE_CFPOLL:
                    ss << "CF-Poll";
                    break;
                case WLAN_FC_STYPE_CFACKPOLL:
                    ss << "CF-Ack+CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_DATA:
                    ss << "QoS Data";
                    break;
                case WLAN_FC_STYPE_QOS_DATA_CFACK:
                    ss << "QoS Data+CF-Ack";
                    break;
                case WLAN_FC_STYPE_QOS_DATA_CFPOLL:
                    ss << "QoS Data+CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_DATA_CFACKPOLL:
                    ss << "QoS Data+CF-Ack+CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_NULL:
                    ss << "QoS Null";
                    break;
                case WLAN_FC_STYPE_QOS_CFPOLL:
                    ss << "QoS CF-Poll";
                    break;
                case WLAN_FC_STYPE_QOS_CFACKPOLL:
                    ss << "QoS CF-Poll+CF-Ack";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        case 3:
            // Extension
            ss << "Extension (";
            switch (subType) {
                case 0:
                    ss << "DMG Beacon";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        default:
            ss << type << " (" << subType << ')';
            break;
    }
    ss << std::endl;
    if (LOG_IS_ON(VERBOSE)) {
        for (int i = 0; i < mData.size(); i++)
            ss << std::hex << mData[i] << std::setw(2) << ' ';
    }
    return ss.str();
}

bool Ieee80211Frame::isData() const {
    return WLAN_FC_GET_TYPE(hdr().frame_control) == WLAN_FC_TYPE_DATA;
}

bool Ieee80211Frame::isMgmt() const {
    return WLAN_FC_GET_TYPE(hdr().frame_control) == WLAN_FC_TYPE_MGMT;
}

bool Ieee80211Frame::isDataQoS() const {
    return isData() &&
           WLAN_FC_GET_STYPE(hdr().frame_control) == WLAN_FC_STYPE_QOS_DATA;
}

bool Ieee80211Frame::isDataNull() const {
    return isData() &&
           (WLAN_FC_GET_STYPE(hdr().frame_control) & WLAN_FC_STYPE_NULLFUNC);
}
bool Ieee80211Frame::isBeacon() const {
    return isMgmt() &&
           WLAN_FC_GET_STYPE(hdr().frame_control) == WLAN_FC_STYPE_BEACON;
}

bool Ieee80211Frame::isToDS() const {
    return isData() && (hdr().frame_control & WLAN_FC_TODS);
}

bool Ieee80211Frame::isFromDS() const {
    return isData() && (hdr().frame_control & WLAN_FC_FROMDS);
}

bool Ieee80211Frame::uses4Addresses() const {
    return (mData[1] & WLAN_FC_TODS) && (mData[1] & WLAN_FC_FROMDS);
}

uint16_t Ieee80211Frame::getQoSControl() const {
    const uint8_t* addr = mData.data() + (IEEE80211_HDRLEN +
                                          (uses4Addresses() ? ETH_ALEN : 0));
    return *reinterpret_cast<const uint16_t*>(addr);
}

const IOVector Ieee80211Frame::toEthernet() {
    IOVector ret;
    uint16_t ethertype;
    if (!memcmp(frameBody(), kRfc1042Header, ETH_ALEN)) {
        ethertype = (frameBody()[ETH_ALEN] << 8) | frameBody()[ETH_ALEN + 1];
    } else {
        ethertype = (frameBody()[0] << 8) | frameBody()[1];
    }
    if (!validEtherType(ethertype)) {
        LOG(VERBOSE) << "Unexpected ether type. Dump frame: "
                     << toStr().c_str();
        return ret;
    }
    struct ieee8023_hdr* eth = new struct ieee8023_hdr;
    eth->ethertype = htons(ethertype);
    // Order of addresses: RA SA DA
    memcpy(eth->dest, &(addr3()[0]), ETH_ALEN);
    memcpy(eth->src, &(addr2()[0]), ETH_ALEN);

    ret.push_back({.iov_base = eth, .iov_len = ETH_HLEN});
    size_t offset = hdrLength() + ETH_ALEN + sizeof(eth->ethertype);
    IOVector inSg;
    inSg.push_back({.iov_base = data(), .iov_len = size()});
    inSg.appendEntriesTo(&ret, offset, size() - offset);
    return ret;
}

bool Ieee80211Frame::validEtherType(uint16_t ethertype) {
    return ethertype == ETH_P_IPV6 || ethertype == ETH_P_IP ||
           ethertype == ETH_P_ARP || ethertype == ETH_P_NCSI;
}

}  // namespace network
}  // namespace android

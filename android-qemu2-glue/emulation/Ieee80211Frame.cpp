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

#include "android-qemu2-glue/emulation/Ieee80211Frame.h"
#include "android-qemu2-glue/utils/Endian.h"

#include <sstream>

extern "C" {
#include "qemu/osdep.h"
// Undefine MACRO ARRAY_SIZE to avoid name clash and function
// redefinitions.
#undef ARRAY_SIZE
#include "common/ieee802_11_defs.h"
}

using android::network::MacAddress;
namespace android {
namespace qemu2 {

// Frame Header layout as below
//       2         2           6        6        6       2       6
// Frame-Ctl | Duration ID | Addr1 | Addr2 | Addr3 | Seq-Ctl | Addr4 | Frame
// Body
Ieee80211Frame::Ieee80211Frame(const uint8_t* data, size_t size)
    : mData(data, data + size) {
    mAddr1 = MacAddress(MACARG(hdr().addr1));
    mAddr2 = MacAddress(MACARG(hdr().addr2));
    mAddr3 = MacAddress(MACARG(hdr().addr3));
    if (uses4Addresses()) {
        mAddr4 = MacAddress(MACARG(data + IEEE80211_HDRLEN));
    }
}

const ieee80211_hdr& Ieee80211Frame::hdr() const {
    const ieee80211_hdr* hdr = (ieee80211_hdr*)mData.data();
    return *hdr;
}

size_t Ieee80211Frame::hdrLength() const {
    return uses4Addresses() ? IEEE80211_HDRLEN : (IEEE80211_HDRLEN + ETH_ALEN);
}
uint8_t* Ieee80211Frame::frameBody() {
    return mData.data() +
           (IEEE80211_HDRLEN + (uses4Addresses() ? ETH_ALEN : 0));
}

std::string Ieee80211Frame::toStr() {
    std::stringstream ss;
    uint8_t type = WLAN_FC_GET_TYPE(hdr().frame_control);
    uint8_t subType = WLAN_FC_GET_STYPE(hdr().frame_control);
    switch (type) {
        case 0:
            // Management
            ss << "Management (";
            switch (subType) {
                case 0:
                    ss << "Association Request";
                    break;
                case 1:
                    ss << "Association Response";
                    break;
                case 2:
                    ss << "Reassociation Request";
                    break;
                case 3:
                    ss << "Reassociation Response";
                    break;
                case 4:
                    ss << "Probe Request";
                    break;
                case 5:
                    ss << "Probe Response";
                    break;
                case 6:
                    ss << "Timing Advertisement";
                    break;
                case 8:
                    ss << "Beacon";
                    break;
                case 9:
                    ss << "ATIM";
                    break;
                case 10:
                    ss << "Disassociation";
                    break;
                case 11:
                    ss << "Authentication";
                    break;
                case 12:
                    ss << "Deauthentication";
                    break;
                case 13:
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
        case 1:
            // Control
            ss << "Control (";
            switch (subType) {
                case 4:
                    ss << "Beamforming Report Poll";
                    break;
                case 5:
                    ss << "VHT NDP Announcement";
                    break;
                case 6:
                    ss << "Control Frame Extension";
                    break;
                case 7:
                    ss << "Control Wrapper";
                    break;
                case 8:
                    ss << "Block Ack Request";
                    break;
                case 9:
                    ss << "Block Ack";
                    break;
                case 10:
                    ss << "PS-Poll";
                    break;
                case 11:
                    ss << "RTS";
                    break;
                case 12:
                    ss << "CTS";
                    break;
                case 13:
                    ss << "Ack";
                    break;
                case 14:
                    ss << "CF-End";
                    break;
                case 15:
                    ss << "CF-End+CF-Ack";
                    break;
                default:
                    ss << subType;
                    break;
            }
            ss << ')';
            break;
        case 2:
            // Data
            ss << "Data (";
            switch (subType) {
                case 0:
                    ss << "Data";
                    break;
                case 1:
                    ss << "Data+CF-Ack";
                    break;
                case 2:
                    ss << "Data+CF-Poll";
                    break;
                case 3:
                    ss << "Data+CF-Ack+CF-Poll";
                    break;
                case 4:
                    ss << "Null";
                    break;
                case 5:
                    ss << "CF-Ack";
                    break;
                case 6:
                    ss << "CF-Poll";
                    break;
                case 7:
                    ss << "CF-Ack+CF-Poll";
                    break;
                case 8:
                    ss << "QoS Data";
                    break;
                case 9:
                    ss << "QoS Data+CF-Ack";
                    break;
                case 10:
                    ss << "QoS Data+CF-Poll";
                    break;
                case 11:
                    ss << "QoS Data+CF-Ack+CF-Poll";
                    break;
                case 12:
                    ss << "QoS Null";
                    break;
                case 14:
                    ss << "QoS CF-Poll";
                    break;
                case 15:
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

    // for (int i = 0; i < mData.size(); i++) {
    //    ss << std::hex << mData[i] << ' ';
    //}
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
    return !!(mData[1] & 0x03);
}

uint16_t Ieee80211Frame::getQoSControl() const {
    const uint8_t* addr = mData.data() + (IEEE80211_HDRLEN +
                                          (uses4Addresses() ? ETH_ALEN : 0));
    return *reinterpret_cast<const uint16_t*>(addr);
}

}  // namespace qemu2
}  // namespace android

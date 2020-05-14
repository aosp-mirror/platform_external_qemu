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

extern "C" {
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
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    mAddr1 = MacAddress(hdr->addr1);
    mAddr2 = MacAddress(hdr->addr2);
    mAddr3 = MacAddress(hdr->addr1);
    if (uses4Addresses()) {
        mAddr4 = MacAddress(data + IEEE80211_HDRLEN);
    }
}

uint8_t* Ieee80211Frame::frameBody() {
    return mData.data() +
           (IEEE80211_HDRLEN + (uses4Addresses() ? ETH_ALEN : 0));
}

bool Ieee80211Frame::isData() const {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    return WLAN_FC_GET_TYPE(hdr->frame_control) == WLAN_FC_TYPE_DATA;
}

bool Ieee80211Frame::isMgmt() const {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    return WLAN_FC_GET_TYPE(hdr->frame_control) == WLAN_FC_TYPE_MGMT;
}

bool Ieee80211Frame::isDataQoS() const {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    return isData() &&
           WLAN_FC_GET_STYPE(hdr->frame_control) == WLAN_FC_STYPE_QOS_DATA;
}

bool Ieee80211Frame::isDataNull() const {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    return isData() &&
           (WLAN_FC_GET_STYPE(hdr->frame_control) & WLAN_FC_STYPE_NULLFUNC);
}
bool Ieee80211Frame::isBeacon() const {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    return isMgmt() &&
           WLAN_FC_GET_STYPE(hdr->frame_control) == WLAN_FC_STYPE_BEACON;
}

bool Ieee80211Frame::isToDS() const {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    return isData() && (hdr->frame_control & WLAN_FC_TODS);
}

bool Ieee80211Frame::isFromDS() const {
    struct ieee80211_hdr* hdr = (struct ieee80211_hdr*)mData.data();
    return isData() && (hdr->frame_control & WLAN_FC_FROMDS);
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

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

#include "android-qemu2-glue/emulation/Frame.h"

extern "C" {
#include "common/ieee802_11_defs.h"
}
using android::network::MacAddress;
namespace android {
namespace qemu2 {

// Frame Header layout as below
//       2              2          6       6       6       2         6
// Frame Control | Duration ID | Addr1 | Addr2 | Addr3 | Seq-Ctl | Addr4 | Frame
// Body
Frame::Frame(const uint8_t* data, size_t size)
    : mData(data, data + size),
      mHdr((struct ieee80211_hdr*)mData.data()),
      mAddr1(mHdr->addr1),
      mAddr2(mHdr->addr2),
      mAddr3(mHdr->addr3) {
    if (uses4Addresses()) {
        mAddr4 = MacAddress(data + IEEE80211_HDRLEN);
    }
}

uint8_t* Frame::frameBody() {
    return mData.data() + (IEEE80211_HDRLEN + (uses4Addresses() ? 6 : 0));
}

bool Frame::isData() const {
    return WLAN_FC_GET_TYPE(mHdr->frame_control) == WLAN_FC_TYPE_DATA;
}

bool Frame::isMgmt() const {
    return WLAN_FC_GET_TYPE(mHdr->frame_control) == WLAN_FC_TYPE_MGMT;
}

bool Frame::isDataQoS() const {
    return isData() &&
           WLAN_FC_GET_STYPE(mHdr->frame_control) == WLAN_FC_STYPE_QOS_DATA;
}

bool Frame::isDataNull() const {
    return isData() &&
           (WLAN_FC_GET_STYPE(mHdr->frame_control) & WLAN_FC_STYPE_NULLFUNC);
}
bool Frame::isBeacon() const {
    return isMgmt() &&
           WLAN_FC_GET_STYPE(mHdr->frame_control) == WLAN_FC_STYPE_BEACON;
}

bool Frame::isToDS() const {
    return isData() && (mHdr->frame_control & WLAN_FC_TODS);
}

bool Frame::isFromDS() const {
    return isData() && (mHdr->frame_control & WLAN_FC_FROMDS);
}

bool Frame::uses4Addresses() const {
    return !!(mData[1] & 0x03);
}

uint16_t Frame::getQoSControl() const {
    const uint8_t* addr =
            mData.data() + (IEEE80211_HDRLEN + (uses4Addresses() ? 6 : 0));
    return *reinterpret_cast<const uint16_t*>(addr);
}

}  // namespace qemu2
}  // namespace android

/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/base/Compiler.h"
#include "android/network/MacAddress.h"

typedef struct ieee80211_hdr ieee80211_hdr;

namespace android {
namespace qemu2 {

// Provide a simple wrapper on top of the IEEE80211 Ieee80211Frame header
// and its underlying data.

class Ieee80211Frame {
public:
    Ieee80211Frame(const uint8_t* data, size_t size);

    size_t size() const { return mData.size(); }
    const uint8_t* data() const { return mData.data(); }
    uint8_t* data() { return mData.data(); }
    uint8_t* frameBody();
    const ieee80211_hdr& hdr() const;
    const android::network::MacAddress& addr1() const { return mAddr1; }
    const android::network::MacAddress& addr2() const { return mAddr2; }
    const android::network::MacAddress& addr3() const { return mAddr3; }
    const android::network::MacAddress& addr4() const { return mAddr4; }

    bool isData() const;
    bool isMgmt() const;
    bool isDataQoS() const;
    bool isDataNull() const;
    bool isBeacon() const;
    bool isToDS() const;
    bool isFromDS() const;
    bool uses4Addresses() const;
    uint16_t getQoSControl() const;

private:
    std::vector<uint8_t> mData;
    android::network::MacAddress mAddr1;
    android::network::MacAddress mAddr2;
    android::network::MacAddress mAddr3;
    android::network::MacAddress mAddr4;
};

}  // namespace qemu2
}  // namespace android

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
#include "android/base/IOVector.h"
#include "android/network/MacAddress.h"

#include <string>
typedef struct ieee80211_hdr ieee80211_hdr;

namespace android {
namespace network {

// Provide a simple wrapper on top of the IEEE80211 Ieee80211Frame header
// and its underlying data.

class Ieee80211Frame {
public:
    Ieee80211Frame(const uint8_t* data, size_t size);
    Ieee80211Frame(size_t size);
    Ieee80211Frame(const std::vector<uint8_t>& ethernet, MacAddress bssid);
    size_t size() const { return mData.size(); }
    size_t hdrLength() const;
    const uint8_t* data() const { return mData.data(); }
    uint8_t* data() { return mData.data(); }
    uint8_t* frameBody();
    const uint8_t* frameBody() const;
    const ieee80211_hdr& hdr() const;
    MacAddress addr1() const;
    MacAddress addr2() const;
    MacAddress addr3() const;
    MacAddress addr4() const;
    std::string toStr();
    bool isData() const;
    bool isMgmt() const;
    bool isDataQoS() const;
    bool isDataNull() const;
    bool isBeacon() const;
    bool isToDS() const;
    bool isFromDS() const;
    bool uses4Addresses() const;
    uint16_t getQoSControl() const;
    const android::base::IOVector toEthernet();
    static constexpr size_t MAX_FRAME_LEN = 2352;
    static bool validEtherType(uint16_t ethertype);

private:
    std::vector<uint8_t> mData;
};

}  // namespace network
}  // namespace android

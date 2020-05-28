/*
 * Copyright 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "android/base/Compiler.h"

#include <stdint.h>
#include <string.h>
#include <vector>

// Format macros for printf, e.g. printf("MAC address: " PRIMAC, MACARG(mac))
#define PRIMAC "%02x:%02x:%02x:%02x:%02x:%02x"
#define MACARG(x) (x)[0], (x)[1], (x)[2], (x)[3], (x)[4], (x)[5]
#define ETH_ALEN 6

namespace android {
namespace network {

struct MacAddress {
    MacAddress() { memset(mAddr, 0, sizeof(mAddr)); }
    MacAddress(uint8_t b1,
               uint8_t b2,
               uint8_t b3,
               uint8_t b4,
               uint8_t b5,
               uint8_t b6) {
        mAddr[0] = b1;
        mAddr[1] = b2;
        mAddr[2] = b3;
        mAddr[3] = b4;
        mAddr[4] = b5;
        mAddr[5] = b6;
    }
    bool isBroadcast() const {
        return memcmp(mAddr, "\xFF\xFF\xFF\xFF\xFF\xFF", ETH_ALEN) == 0;
    }
    bool isMulticast() const { return mAddr[0] & 0x01; }
    bool empty() const {
        return memcmp(mAddr, "\x00\x00\x00\x00\x00\x00", ETH_ALEN) == 0;
    }
    uint8_t& operator[](size_t index) { return mAddr[index]; }
    const uint8_t operator[](size_t index) const { return mAddr[index]; }
    bool operator==(const MacAddress& right) const {
        return memcmp(mAddr, right.mAddr, ETH_ALEN) == 0;
    }
    bool operator!=(const MacAddress& right) const { return !(*this == right); }
    uint8_t mAddr[ETH_ALEN];
};

}  // namespace network
}  // namespace android
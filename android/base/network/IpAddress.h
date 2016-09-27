// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "android/base/StringView.h"

#include <stdint.h>

namespace android {
namespace base {

// A simple structure used to encapsulate an IPv4 or IPv6 address
struct IpAddress {
    // Type of supported IP addresses.
    enum Kind {
        kInvalid = -1,
        kIpv4 = 4,
        kIpv6 = 6,
    };

    Kind kind = kInvalid;
    union {
        uint32_t ipv4;
        struct {
            uint8_t addr[16];
            uint32_t scopeId;
        } ipv6;
    };

    // Default constructor.
    IpAddress() {}

    // Construct an IPv4 address instance.
    explicit IpAddress(uint32_t ip) : kind(kIpv4), ipv4(ip) {}

    // Construct an IPv6 address instance. |bytes| contains the address
    // and |scope_id| the optional scope id, which is a system-specific
    // network interface index (see if_nametoindex()).
    IpAddress(const uint8_t bytes[16], uint32_t scope_id = 0);

    // Construct an instance from |string| which must be either an IPv4
    // dotted-decimal address, of a column-separated IPv6 address, possibly
    // followed by a '%' and a scope id. Caller should later call valid()
    // to ensure the input |string| was valid.
    IpAddress(StringView string);

    // Returns true iff this instance is valid.
    bool valid() const { return kind == kIpv4 || kind == kIpv6; }

    // Returns true iff this instance is IPv4.
    bool isIpv4() const { return kind == kIpv4; }

    // Returns true iff this instance is IPv6.
    bool isIpv6() const { return kind == kIpv6; }

    // Copy and Move operations.
    IpAddress(const IpAddress& other) { copyFrom(this, &other); }

    IpAddress& operator=(const IpAddress& other) {
        copyFrom(this, &other);
        return *this;
    }

    IpAddress(IpAddress&& other) { moveFrom(this, &other); }

    IpAddress& operator=(IpAddress&& other) {
        moveFrom(this, &other);
        return *this;
    }

    // Equality comparator.
    bool operator==(const IpAddress& other) const;

    bool operator!=(const IpAddress& other) const {
        return !(*this == other);
    }

    std::string toString() const;

    static IpAddress createFromString(StringView str);

private:
    static void copyFrom(IpAddress* dst, const IpAddress* src);
    static void moveFrom(IpAddress* dst, IpAddress* src);
};

}  // namespace base
}  // namespace android

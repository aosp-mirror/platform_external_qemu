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

#include "android/base/network/IpAddress.h"

#include "android/base/Log.h"

#include <string>

#include <string.h>

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#include <iphlpapi.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

namespace android {
namespace base {

#ifdef _WIN32
#define inet_pton InetPtonA
#define inet_ntop InetNtopA
#endif

IpAddress::IpAddress(const uint8_t bytes[16], uint32_t scopeId) : kind(kIpv6) {
    ::memcpy(ipv6.addr, bytes, 16);
    ipv6.scopeId = scopeId;
}

IpAddress::IpAddress(StringView string) {
    std::string str(string.c_str(), string.size());

    // Try to parse it as an IPv4 address first.
    struct in_addr ip4;
    if (inet_pton(AF_INET, str.c_str(), &ip4) > 0) {
        this->kind = kIpv4;
        this->ipv4 = ntohl(ip4.s_addr);
        return;
    }

    // Try to parse it as an IPv6 address second.
    struct in6_addr ip6;
    uint32_t scopeId = 0;
    char* p = const_cast<char*>(::strchr(str.c_str(), '%'));
    if (p != nullptr) {
        *p = '\0';
        scopeId = static_cast<uint32_t>(if_nametoindex(p + 1));
    }
    if (inet_pton(AF_INET6, str.c_str(), &ip6) > 0) {
        this->kind = kIpv6;
        this->ipv6.scopeId = scopeId;
        memcpy(this->ipv6.addr, ip6.s6_addr, 16);
        return;
    }

    // Invalid.
    this->kind = kInvalid;
}

std::string IpAddress::toString() const {
    std::string result;
    switch (this->kind) {
        case kIpv4: {
            struct in_addr ip4;
            ip4.s_addr = htonl(this->ipv4);
            result.resize(INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &ip4, &result[0],
                      static_cast<socklen_t>(result.size()));
            result.resize(::strlen(result.c_str()));
            break;
        }
        case kIpv6: {
            struct in6_addr ip6;
            memcpy(ip6.s6_addr, this->ipv6.addr, 16);
            result.resize(INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, &ip6, &result[0],
                      static_cast<socklen_t>(result.size()));
            result.resize(::strlen(result.c_str()));
            break;
        }
        default:
            result = "<invalid>";
    }
    return result;
}

void IpAddress::copyFrom(IpAddress* dst, const IpAddress* src) {
    dst->kind = src->kind;
    switch (dst->kind) {
        case kInvalid:
            break;
        case kIpv4:
            dst->ipv4 = src->ipv4;
            break;

        case kIpv6:
            dst->ipv6 = src->ipv6;
            break;

        default:
            // Useful to detect corrupted instances.
            DCHECK(false) << "Invalid IpAddress kind " << dst->kind;
    }
}

void IpAddress::moveFrom(IpAddress* dst, IpAddress* src) {
    copyFrom(dst, src);
    src->kind = kInvalid;
}

bool IpAddress::operator==(const IpAddress& other) const {
    if (kind != other.kind) {
        return false;
    }
    switch (kind) {
        case kInvalid:
            return true;

        case kIpv4:
            return ipv4 == other.ipv4;

        case kIpv6:
            return ipv6.scopeId == other.ipv6.scopeId &&
                   !memcmp(ipv6.addr, other.ipv6.addr, 16);

        default:
            DCHECK(false) << "Invalid IpAddress kind " << kind;
            return false;
    }
}

}  // namespace base
}  // namespace android

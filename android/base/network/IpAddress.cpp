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

#include "android/base/network/NetworkUtils.h"
#include "android/base/Log.h"

#include <algorithm>
#include <string>

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

// Windows has InetPtonA() and InetNtopA() but these are not implemented
// by Wine in 32-bit mode, which will actually abort when running our
// unit-test suite, so implement the functions here directly:
static int my_inet_pton(int af, const char* src, void* dst) {
    if (af == AF_INET) {
        auto in = static_cast<struct in_addr*>(dst);
        return netStringToIpv4(src, reinterpret_cast<uint8_t*>(&in->s_addr));
    }
    if (af == AF_INET6) {
        auto in6 = static_cast<struct in6_addr*>(dst);
        return netStringToIpv6(src, in6->s6_addr);
    }
    return 0;
}

#define inet_pton my_inet_pton

// InetNtopA works on Wine though.
#define inet_ntop InetNtopA
#endif

IpAddress::IpAddress(const Ipv6Address bytes, uint32_t scopeId) : mKind(kIpv6) {
    memcpy(mIpv6.mAddr, bytes, sizeof(mIpv6.mAddr));
    mIpv6.mScopeId = scopeId;
}

IpAddress::IpAddress(StringView string) {
    std::string str(string.c_str(), string.size());

    // Try to parse it as an IPv4 address first.
    struct in_addr ip4;
    if (inet_pton(AF_INET, str.c_str(), &ip4) > 0) {
        mKind = kIpv4;
        mIpv4 = ntohl(ip4.s_addr);
        return;
    }

    // Try to parse it as an IPv6 address second.
    struct in6_addr ip6;
    uint32_t scopeId = 0;
    auto it = std::find(str.begin(), str.end(), '%');
    if (it != str.end()) {
        *it = '\0';
        int idx = netStringToInterfaceIndex(&(*++it));
        if (idx < 0) {  // invalid interface name.
            return;
        }
        scopeId = static_cast<uint32_t>(idx);
    }
    if (inet_pton(AF_INET6, str.c_str(), &ip6) > 0) {
        mKind = kIpv6;
        mIpv6.mScopeId = scopeId;
        static_assert(sizeof(mIpv6.mAddr) == sizeof(ip6.s6_addr),
                      "invalid size of Ipv6Address");
        memcpy(mIpv6.mAddr, ip6.s6_addr, sizeof(mIpv6.mAddr));
        return;
    }

    // Invalid.
    mKind = kInvalid;
}

std::string IpAddress::toString() const {
    std::string result;
    switch (mKind) {
        case kIpv4: {
            struct in_addr ip4;
            ip4.s_addr = htonl(mIpv4);
            result.resize(INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &ip4, &result[0],
                      static_cast<socklen_t>(result.size()));
            result.resize(::strlen(result.c_str()));
            break;
        }
        case kIpv6: {
            struct in6_addr ip6;
            static_assert(sizeof(ip6.s6_addr) == sizeof(mIpv6.mAddr),
                          "Invalid Ipv6Address size");
            memcpy(ip6.s6_addr, mIpv6.mAddr, sizeof(ip6.s6_addr));
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

size_t IpAddress::hash() const {
    size_t hash = 0U;
    switch (mKind) {
        case kInvalid:
            // Don't want 0 here, since it's the hash of 0.0.0.0 as well
            // which can be quite common.
            hash = 0x12345678;
            break;
        case kIpv4:
            hash = static_cast<size_t>(mIpv4);
            break;
        case kIpv6:
            // TODO(digit): Find better hash function?
            for (size_t n = 0; n < sizeof(mIpv6.mAddr); ++n) {
                hash = (hash * 33) ^ mIpv6.mAddr[n];
            }
            hash = hash ^ mIpv6.mScopeId;
            break;
    }
    return hash;
}

void IpAddress::copyFrom(IpAddress* dst, const IpAddress* src) {
    dst->mKind = src->mKind;
    switch (dst->mKind) {
        case kInvalid:
            break;
        case kIpv4:
            dst->mIpv4 = src->mIpv4;
            break;

        case kIpv6:
            dst->mIpv6 = src->mIpv6;
            break;

        default:
            // Useful to detect corrupted instances.
            DCHECK(false) << "Invalid IpAddress mKind " << dst->mKind;
    }
}

bool IpAddress::operator==(const IpAddress& other) const {
    if (mKind != other.mKind) {
        return false;
    }
    switch (mKind) {
        case kInvalid:
            return true;

        case kIpv4:
            return mIpv4 == other.mIpv4;

        case kIpv6:
            return mIpv6.mScopeId == other.mIpv6.mScopeId &&
                   !memcmp(mIpv6.mAddr, other.mIpv6.mAddr, sizeof(mIpv6.mAddr));

        default:
            DCHECK(false) << "Invalid IpAddress mKind " << mKind;
            return false;
    }
}

bool IpAddress::operator<(const IpAddress& other) const {
    bool result = false;
    if (mKind == other.mKind) {
        switch (mKind) {
            case kInvalid:
                result = false;
                break;
            case kIpv4:
                result = (mIpv4 < other.mIpv4);
                break;
            case kIpv6:
                int ret = ::memcmp(mIpv6.mAddr, other.mIpv6.mAddr,
                                   sizeof(mIpv6.mAddr));
                if (ret != 0) {
                    result = (ret < 0);
                } else {
                    result = mIpv6.mScopeId < other.mIpv6.mScopeId;
                }
        }
    } else {
        // Invalid addresses are always smaller than valid ones.
        // Ipv4 addresses are smaller than IPv6 ones.
        switch (mKind) {
            case kInvalid:
                result = true;
                break;
            case kIpv4:
                result = (other.mKind == kIpv6);
                break;
            case kIpv6:
                result = false;
                break;
        }
    }
    return result;
}

}  // namespace base
}  // namespace android

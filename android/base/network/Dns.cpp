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

#include "android/base/network/Dns.h"

#include "android/base/Log.h"

#include <string>

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#include "android/base/sockets/SocketErrors.h"
#include "iphlpapi.h"
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <fstream>

#include <errno.h>
#include <string.h>

namespace android {
namespace base {

namespace {

using AddressList = android::base::Dns::AddressList;

// Implementation of Dns::Resolver interface based on ::getaddrinfo().
class SystemResolver : public Dns::Resolver {
public:
    virtual int resolveName(StringView dns_server_name,
                            AddressList* out) override {
        std::string hostname = dns_server_name;
        struct addrinfo* res;
        int ret = ::getaddrinfo(hostname.c_str(), nullptr, nullptr, &res);
        if (ret != 0) {
            switch (ret) {
                case EAI_AGAIN:  // server is down.
                case EAI_FAIL:   // server is sick.
                    return -EHOSTDOWN;

#if defined(EAI_NODATA) && (EAI_NODATA != EAHO_NONAME)
                case EAI_NODATA:
#endif
                case EAI_NONAME:
                    return -ENOENT;

                case EAI_MEMORY:
                    return -ENOMEM;

                default:
                    return -EINVAL;
            }
        }

        // Parse the returned list of addresses.
        for (struct addrinfo* r = res; r != nullptr; r = r->ai_next) {
            // On Linux, getaddrinfo() will return the same address with up to
            // *three* struct addrinfo that only differ by their
            // |ai_socktype| field, which will be SOCK_STREAM, SOCK_DGRAM,
            // and SOCK_RAW. Ignore the ones that are not SOCK_STREAM
            // since we don't expect a machine to have different IP
            // addresses for TCP and UDP.
            if (r->ai_socktype != SOCK_STREAM) {
                continue;
            }
            switch (r->ai_family) {
                case AF_INET: {
                    auto sin =
                            reinterpret_cast<struct sockaddr_in*>(r->ai_addr);
                    out->push_back(IpAddress(ntohl(sin->sin_addr.s_addr)));
                    break;
                }

                case AF_INET6: {
                    auto sin6 =
                            reinterpret_cast<struct sockaddr_in6*>(r->ai_addr);
                    out->push_back(IpAddress(sin6->sin6_addr.s6_addr,
                                             sin6->sin6_scope_id));
                    break;
                }
            }
        }

        ::freeaddrinfo(res);

        return 0;
    }

    virtual int getSystemServerList(AddressList* out) override {
#ifdef _WIN32
        std::vector<char> fixedInfoBuffer(sizeof(FIXED_INFO));
        auto fixedInfo = reinterpret_cast<FIXED_INFO*>(&fixedInfoBuffer[0]);
        ULONG bufLen = fixedInfoBuffer.size();

        DWORD ret = GetNetworkParams(fixedInfo, &bufLen);
        if (ret == ERROR_BUFFER_OVERFLOW) {
            fixedInfoBuffer.resize(bufLen);
            fixedInfo = reinterpret_cast<FIXED_INFO*>(&fixedInfoBuffer[0]);
            ret = GetNetworkParams(fixedInfo, &bufLen);
        }
        if (ret != ERROR_SUCCESS) {
            // No DNS entries in system table?
            return -ENOENT;
        }

        // TODO(digit): Use GetAdaptersAddresses() to get a list of IPv6
        // DNS servers. This is not trivial because this API only gives
        // per-adapter DNS server lists, not a global one like
        // GetNetworkParams() which is limited to IPv4.
        size_t count = 0;
        for (IP_ADDR_STRING* ipAddr = &fixedInfo->DnsServerList; ipAddr;
             ipAddr = ipAddr->Next) {
            IpAddress addr(ipAddr->IpAddress.String);
            if (addr.valid() && addr.isIpv4()) {
                out->emplace_back(std::move(addr));
                count++;
            }
        }

        if (count == 0) {
            return -ENOENT;
        }
        return 0;
#else  // !_WIN32
        std::ifstream fin;
#ifdef __APPLE__
        // on Darwin /etc/resolv.conf is a symlink to
        // /private/var/run/resolv.conf
        // in some situations, the symlink can be destroyed and the system will
        // not
        // re-create it. Darwin-aware applications will continue to run, but
        // "legacy" Unix ones will not.
        fin.open("/private/var/run/resolv.conf");
        if (!fin) {
            fin.open("/etc/resolv.conf");
        }
#else
        fin.open("/etc/resolv.conf");
#endif
        if (!fin.good()) {
            return -ENOENT;
        }

        std::string line;
        size_t count = 0;
        while (std::getline(fin, line)) {
            char nameserver[257];
            if (sscanf(line.c_str(), "nameserver%*[ \t]%256s", nameserver) ==
                1) {
                IpAddress ip(nameserver);
                if (ip.valid()) {
                    out->emplace_back(std::move(ip));
                    count++;
                }
            }
        }

        if (count == 0) {
            return -ENOENT;
        }

        return 0;
#endif  // !_WIN32
    }
};

// Current process-global Resolver instance.
Dns::Resolver* sResolver = nullptr;

}  // namespace

// static
AddressList Dns::resolveName(StringView serverName) {
    AddressList result;
    int err = Dns::Resolver::get()->resolveName(serverName, &result);
    if (err != 0) {
        result.clear();
    }
    return result;
}

// static
AddressList Dns::getSystemServerList() {
    AddressList result;
    int err = Dns::Resolver::get()->getSystemServerList(&result);
    if (err != 0) {
        result.clear();
    }
    return result;
}

// static
Dns::Resolver* Dns::Resolver::get() {
    if (!sResolver) {
        sResolver = new SystemResolver();
    }
    return sResolver;
}

// static
Dns::Resolver* Dns::Resolver::setForTesting(Dns::Resolver* resolver) {
    Dns::Resolver* result = sResolver;
    sResolver = resolver;
    return result;
}

}  // namespace base
}  // namespace android

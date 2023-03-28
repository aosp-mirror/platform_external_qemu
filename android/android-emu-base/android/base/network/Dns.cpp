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

#include "android/base/logging/CLog.h"

#include <string>
#include <unordered_set>

#ifdef _WIN32
#include "android/base/ArraySize.h"
#include "android/base/sockets/Winsock.h"
#include "android/base/sockets/SocketErrors.h"
#include "android/base/system/Win32UnicodeString.h"
#include "android/base/system/Win32Utils.h"
#include "wincrypt.h"
#include "iphlpapi.h"
# include <windows.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# include <sys/timeb.h>
# include <wincrypt.h>
# include <iphlpapi.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "android/utils/system.h"

#include <fstream>

#include <errno.h>
#include <string.h>

// Set to 1 to increase verbosity of debug messages.
#define DEBUG 0

namespace android {
namespace base {

namespace {

using AddressList = android::base::Dns::AddressList;

#ifdef _WIN32
#define INITIAL_DNS_ADDR_BUF_SIZE 32 * 1024
#define REALLOC_RETRIES 5

// Broadcast site local DNS resolvers. We do not use these because they are
// highly unlikely to be valid.
// https://www.ietf.org/proceedings/52/I-D/draft-ietf-ipngwg-dns-discovery-03.txt
static const struct in6_addr SITE_LOCAL_DNS_BROADCAST_ADDRS[] = {
    {
        {{
            0xfe, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
        }}
    },
    {
        {{
            0xfe, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02
        }}
    },
    {
        {{
            0xfe, 0xc0, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03,
        }}
    },
};

static inline bool in6_equal(const struct in6_addr* a,
                             const struct in6_addr* b) {
    return memcmp(a, b, sizeof(*a)) == 0;
}

static int is_site_local_dns_broadcast(struct in6_addr* address) {
    int i;
    for (i = 0; i < ARRAY_SIZE(SITE_LOCAL_DNS_BROADCAST_ADDRS); i++) {
        if (in6_equal(address, &SITE_LOCAL_DNS_BROADCAST_ADDRS[i])) {
            return 1;
        }
    }
    return 0;
}
#endif

#if DEBUG
#ifdef _WIN32
// Convenience function to save typing.
static std::string toUtf8(const wchar_t* str) {
    return android::base::Win32UnicodeString::convertToUtf8(str);
}
#endif  // !_WIN32
#endif  // _DEBUG

// Implementation of Dns::Resolver interface based on ::getaddrinfo().
class SystemResolver : public Dns::Resolver {
public:
    virtual int resolveName(StringView dns_server_name,
                            AddressList* out) override {
        std::string hostname = dns_server_name;
        struct addrinfo* res;
        struct addrinfo* pHints = nullptr;
#ifdef _WIN32
        struct addrinfo hints = {};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        pHints = &hints;
#endif
        int ret = ::getaddrinfo(hostname.c_str(), nullptr, pHints, &res);
        if (ret != 0) {
            derror("getaddrinfo('%s') returned %d", dns_server_name.data(), ret);
            switch (ret) {
                case EAI_AGAIN:  // server is down.
                case EAI_FAIL:   // server is sick.
                    return -EHOSTDOWN;

#if defined(EAI_NODATA) && (EAI_NODATA != EAHO_NONAME) && (EAI_NODATA != EAI_NONAME)
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
        FIXED_INFO* FixedInfo = NULL;
        ULONG BufLen;
        DWORD ret;
        IP_ADDR_STRING* pIPAddr;
        struct in_addr tmp_addr;
        // The set of DNS server addresses already in the output list,
        // used to remove duplicates.
        std::unordered_set<IpAddress> present;

        // Retrieve IPv4 address.
        FixedInfo = (FIXED_INFO*)GlobalAlloc(GPTR, sizeof(FIXED_INFO));
        BufLen = sizeof(FIXED_INFO);

        if (ERROR_BUFFER_OVERFLOW == GetNetworkParams(FixedInfo, &BufLen)) {
            if (FixedInfo) {
                GlobalFree(FixedInfo);
                FixedInfo = NULL;
            }
            FixedInfo = (FIXED_INFO*)GlobalAlloc(GPTR, BufLen);
        }

        if ((ret = GetNetworkParams(FixedInfo, &BufLen)) != ERROR_SUCCESS) {
            LOG(ERROR) << "GetNetworkParams failed. ret = " << ((u_int)ret);
            if (FixedInfo) {
                GlobalFree(FixedInfo);
                FixedInfo = NULL;
            }
        } else {
            pIPAddr = &(FixedInfo->DnsServerList);
            IpAddress ip(pIPAddr->IpAddress.String);
            if (ip.valid()) {
                auto ret = present.insert(ip);
                if (ret.second) {
                    LOG(INFO) << "IPv4 server found: " << ip.toString();
                    out->emplace_back(std::move(ip));
                }
            }
            if (FixedInfo) {
                GlobalFree(FixedInfo);
                FixedInfo = NULL;
            }
        }

        // Gets the first valid DNS resolver with an IPv6 address.
        // Ignores any site local broadcast DNS servers, as these
        // are on deprecated addresses and not generally expected
        // to work. Further details at:
        // https://www.ietf.org/proceedings/52/I-D/draft-ietf-ipngwg-dns-discovery-03.txt

        PIP_ADAPTER_ADDRESSES addresses = NULL;
        PIP_ADAPTER_ADDRESSES address = NULL;
        IP_ADAPTER_DNS_SERVER_ADDRESS* dns_server = NULL;
        struct sockaddr_in6* dns_v6_addr = NULL;

        ULONG buf_size = INITIAL_DNS_ADDR_BUF_SIZE;
        DWORD res = ERROR_BUFFER_OVERFLOW;
        int i;

        for (i = 0; i < REALLOC_RETRIES; i++) {
            // If non null, we hit buffer overflow, free it so we can try again.
            if (addresses != NULL) {
                android_free(addresses);
            }

            addresses = static_cast<PIP_ADAPTER_ADDRESSES>(android_alloc0(buf_size));
            res = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL,
                                       addresses, &buf_size);

            if (res != ERROR_BUFFER_OVERFLOW) {
                break;
            }
        }

        if (res != NO_ERROR) {
            LOG(ERROR) << "Failed to get IPv6 DNS addresses due to error "
                       << res;
        } else {
            address = addresses;
            for (address = addresses; address != NULL;
                 address = address->Next) {
                for (dns_server = address->FirstDnsServerAddress;
                     dns_server != NULL; dns_server = dns_server->Next) {
                    if (dns_server->Address.lpSockaddr->sa_family != AF_INET6) {
                        continue;
                    }

                    dns_v6_addr = (struct sockaddr_in6*)
                                          dns_server->Address.lpSockaddr;
                    char address_str[INET6_ADDRSTRLEN] = "";
                    if (inet_ntop(AF_INET6, &address, address_str,
                                  INET6_ADDRSTRLEN) == NULL) {
                        LOG(ERROR) << "Failed to stringify IPv6 address for "
                                      "logging.";
                    }
                    // b/267647323
                    // Ignore IPv6 DNS address which is site local DNS broadcast
                    // or link local.
                    if (is_site_local_dns_broadcast(&dns_v6_addr->sin6_addr) ||
                        IN6_IS_ADDR_LINKLOCAL(&dns_v6_addr->sin6_addr)) {
                        dprint("Ignore IPv6 address: %s\n", address_str);
                        continue;
                    }

                    dprint("IPv6 DNS server found: %s\n", address_str);
                    IpAddress ip = IpAddress(dns_v6_addr->sin6_addr.s6_addr);
                    if (ip.valid()) {
                        auto ret = present.insert(ip);
                        if (ret.second) {
                            out->emplace_back(std::move(ip));
                        }
                    }
                }
            }
        }
        if (addresses != NULL) {
            android_free(addresses);
        }
        if (present.empty()) {
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
                    // b/267647323, discard IPv6 DNS address which is also link
                    // local
                    bool skip = false;
                    if (ip.isIpv6()) {
                        struct in6_addr sin6_addr;
                        if (!inet_pton(AF_INET6, nameserver, &sin6_addr)) {
                            dprint("Failed to parse IPv6 DNS address %s\n", nameserver);
                        } else {
                            skip = IN6_IS_ADDR_LINKLOCAL(&sin6_addr);
                        }
                    }
                    if (!skip) {
                        out->emplace_back(std::move(ip));
                        count++;
                    } else {
                        dprint("Ignore IPv6 link local address: %s\n", ip.toString().c_str());
                    }
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

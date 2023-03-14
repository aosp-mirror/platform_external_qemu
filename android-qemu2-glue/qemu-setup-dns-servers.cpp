// Copyright 2015 The Android Open Source Project
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
#ifdef _MSC_VER
extern "C" {
#include "sysemu/os-win32-msvc.h"
}
#endif
#include "android-qemu2-glue/qemu-setup.h"

#include "android/base/Log.h"
#include "android/base/network/IpAddress.h"
#include "android/base/network/Dns.h"
#include "android/utils/debug.h"

extern "C" {
#include "qemu/osdep.h"
#include "net/slirp.h"
#include "slirp/libslirp.h"
}  // extern "C"

#include <vector>

#include <errno.h>

#define MAX_DNS_SERVERS 4

using android::base::IpAddress;
using android::base::Dns;

static int s_num_dns_server_addresses = 0;
static sockaddr_storage s_dns_server_addresses[MAX_DNS_SERVERS] = {};

// Convert IpAddress instance |src| into a sockaddr_storage |dst|.
// Return true on success, false on failure (invalid IpAddress type).
static bool sockaddr_storage_from_ipaddress(sockaddr_storage* dst,
                                            const IpAddress& src) {
    if (src.isIpv4()) {
        auto sin = reinterpret_cast<sockaddr_in *>(dst);
        sin->sin_family = AF_INET;
        sin->sin_addr.s_addr = htonl(src.ipv4());
        return true;
    }
    if (src.isIpv6()) {
        auto sin6 = reinterpret_cast<sockaddr_in6 *>(dst);
        sin6->sin6_family = AF_INET6;
        memcpy(sin6->sin6_addr.s6_addr, src.ipv6Addr(), 16);
        sin6->sin6_scope_id = htonl(src.ipv6ScopeId());
        sin6->sin6_port = 0;
        return true;
    }
    return false;
}

// Resolve host name |hostName| into a list of sockaddr_storage.
// On success, return true and append the names to |*out|. On failure
// return false, leave |*out| untouched, and sets errno.
static bool resolveHostNameToList(
        const char* hostName, std::vector<sockaddr_storage>* out) {
    int count = 0;
    Dns::AddressList list = Dns::resolveName(hostName);
    for (const auto& ip : list) {
        sockaddr_storage addr = {};
        if (sockaddr_storage_from_ipaddress(&addr, ip)) {
            out->emplace_back(std::move(addr));
            count++;
        }
    }
    return (count > 0);
}

bool qemu_android_emulation_setup_dns_servers(const char* dns_servers,
                                              int* pcount) {
    if (net_slirp_state() == nullptr) {
        // When running with a TAP interface slirp will not be active and it
        // also does not make any sense to set up the DNS server translation.
        // The guest will pick up whatever DNS server the host network provides
        // through DHCP and RDNSS.
        return true;
    }

    if (!dns_servers || !dns_servers[0]) {
        // Empty list, use the default behaviour.
        return 0;
    }

    std::vector<sockaddr_storage> server_addresses;

    // Separate individual DNS server names, then resolve each one of them
    // into one or more IP addresses. Support both IPv4 and IPv6 at the same
    // time.
    const char* p = dns_servers;
    while (*p) {
        const char* next_p;
        const char* comma = strchr(p, ',');
        if (!comma) {
            comma = p + strlen(p);
            next_p = comma;
        } else {
            next_p = comma + 1;
        }
        while (p < comma && *p == ' ') p++;
        while (p < comma && comma[-1] == ' ') comma--;

        if (comma > p) {
            // Extract single server name.
            std::string server(p, comma - p);
            if (!resolveHostNameToList(server.c_str(), &server_addresses)) {
                dwarning("Ignoring ivalid DNS address: [%s]\n", server.c_str());
            }
        }
        p = next_p;
    }

    int count = static_cast<int>(server_addresses.size());
    if (!count) {
        return 0;
    }

    if (count > MAX_DNS_SERVERS) {
        dwarning("Too many DNS servers, only keeping the first %d ones\n",
                 MAX_DNS_SERVERS);
        count = MAX_DNS_SERVERS;
    }

    // Save it for qemu_android_emulator_init_slirp().
    s_num_dns_server_addresses = count;
    memcpy(s_dns_server_addresses, &server_addresses[0],
           count * sizeof(server_addresses[0]));

    *pcount = count;
    return true;
}

void qemu_android_emulation_init_slirp(void) {
    net_slirp_init_custom_dns_servers(s_dns_server_addresses,
                                  s_num_dns_server_addresses);
}


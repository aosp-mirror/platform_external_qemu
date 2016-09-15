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

#include "android-qemu2-glue/qemu-setup.h"

#include "android/base/Log.h"
#include "android/utils/debug.h"

#ifdef _WIN32
// This includes must happen before qemu/osdep.h to avoid compiler
// errors regarding FD_SETSIZE being redefined.
#include "android/base/sockets/Winsock.h"
#include "android/base/sockets/SocketErrors.h"
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

extern "C" {
#include "qemu/osdep.h"
#include "net/slirp.h"
#include "slirp/libslirp.h"
}  // extern "C"

#include <vector>

#include <errno.h>

#define MAX_DNS_SERVERS 8

static int s_num_dns_server_addresses = 0;
static sockaddr_storage s_dns_server_addresses[MAX_DNS_SERVERS] = {};

// Resolve host name |hostName| into a list of sockaddr_storage.
// On success, return true and append the names to |*out|. On failure
// return false, leave |*out| untouched, and sets errno.
static bool resolveHostNameToList(
        const char* hostName, std::vector<sockaddr_storage>* out) {
    addrinfo* res = nullptr;
    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    int ret = ::getaddrinfo(hostName, nullptr, &hints, &res);
    if (ret != 0) {
        // Handle errors.
        int err = 0;
        switch (ret) {
            case EAI_AGAIN:  // server is down
            case EAI_FAIL:   // server is sick
                err = EHOSTDOWN;
                break;
/* NOTE that in x86_64-w64-mingw32 both EAI_NODATA and EAI_NONAME are the same */
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
            case EAI_NODATA:
#endif
            case EAI_NONAME:
                err = ENOENT;
                break;

            case EAI_MEMORY:
                err = ENOMEM;
                break;

            default:
                err = EINVAL;
        }
        errno = err;
        return false;
    }

    int count = 0;

    for (auto r = res; r != nullptr; r = r->ai_next) {
        sockaddr_storage addr = {};
        switch (r->ai_family) {
            case AF_INET:
                *(struct sockaddr_in *)&addr =
                        *(const struct sockaddr_in *)r->ai_addr;
                break;

            case AF_INET6:
                *(struct sockaddr_in6 *)&addr =
                        *(const struct sockaddr_in6 *)r->ai_addr;
                break;
            default:
                continue;
        }
        out->emplace_back(std::move(addr));
        count++;
    }
    ::freeaddrinfo(res);

    return (count > 0);
}

bool qemu_android_emulation_setup_dns_servers(const char* dns_servers,
                                              int* pcount4, int* pcount6) {
    CHECK(net_slirp_state() != nullptr) << "slirp stack should be inited!";

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

    // Save it for qemu_android_emulator_init_slirp().
    s_num_dns_server_addresses = count;
    memcpy(s_dns_server_addresses, &server_addresses[0],
           count * sizeof(server_addresses[0]));

    // Count number of IPv4 and IPv6 DNS servers.
    int count4 = 0;
    int count6 = 0;
    for (const auto& item : server_addresses) {
        if (item.ss_family == AF_INET) {
            count4 += 1;
        } else if (item.ss_family == AF_INET6) {
            count6 += 1;
        }
    }

    *pcount4 = count4;
    *pcount6 = count6;

    return true;
}

void qemu_android_emulation_init_slirp(void) {
    slirp_init_custom_dns_servers(static_cast<Slirp*>(net_slirp_state()),
                                  s_dns_server_addresses,
                                  s_num_dns_server_addresses);
}


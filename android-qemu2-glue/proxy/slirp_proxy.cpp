/* Copyright 2016 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android-qemu2-glue/proxy/slirp_proxy.h"

#include "aemu/base/Log.h"
#include "aemu/base/memory/LazyInstance.h"
#include "android/proxy/proxy_common.h"
#include "android/utils/sockets.h"

extern "C" {
#include "slirp/proxy.h"
}

#ifdef _WIN32
#include "aemu/base/sockets/Winsock.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include <unordered_map>
#include <utility>

extern "C" const char* op_http_proxy;

namespace {

// NOTE: The SLIRP stack only runs in the main thread, so don't
//       use a mutex to protect global variables from multiple threads.

struct Globals {
    // Implement a global map |connect_opaque| -> { |af|, |connect_func| }
    // to be used in the android_tcp_proxy_event() function to map opaque
    // handles to the appropriate callback and address family.
    struct Info {
        Info() = default;
        Info(int af_, SlirpProxyConnectFunc* connect_func_) : af(af_), connect_func(connect_func_) {}
        int af = 0;
        SlirpProxyConnectFunc* connect_func = nullptr;
    };
    using MapType = std::unordered_map<void*, Info>;
    MapType mMap;
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

}  // namespace

using MapType = Globals::MapType;

static void android_tcp_proxy_event(void* opaque, int fd, ProxyEvent event) {
    MapType* map = &sGlobals->mMap;
    auto it = map->find(opaque);
    DCHECK(it != map->end());
    if (event != PROXY_EVENT_CONNECTED) {
        fd = -1;
    }
    it->second.connect_func(opaque, fd, it->second.af);
}

static bool android_proxy_try_connect(const struct sockaddr_storage *addr,
                                      SlirpProxyConnectFunc *connect_func,
                                      void *connect_opaque) {
    SockAddress sockaddr = {};
    int af = addr->ss_family;
    switch (af) {
        case AF_INET: {
            auto sin = reinterpret_cast<const sockaddr_in *>(addr);
            sock_address_init_inet(&sockaddr, ntohl(sin->sin_addr.s_addr),
                                   ntohs(sin->sin_port));
            break;
        }
        case AF_INET6: {
            auto sin6 = reinterpret_cast<const sockaddr_in6 *>(addr);
            sock_address_init_in6(&sockaddr, sin6->sin6_addr.s6_addr,
                                  ntohs(sin6->sin6_port));
            break;
        }
        default:
            return false;
    }

    MapType* map = &sGlobals->mMap;
    (*map)[connect_opaque] = Globals::Info({af, connect_func});

    if (!proxy_manager_add(&sockaddr, SOCKET_STREAM, android_tcp_proxy_event,
                           connect_opaque)) {
        return true;
    }

    // No proxy possible here, to remove from map.
    map->erase(connect_opaque);
    return false;
}

static void android_proxy_remove(void* connect_opaque) {
    sGlobals->mMap.erase(connect_opaque);
    proxy_manager_del(connect_opaque);
}

static const struct SlirpProxyOps android_proxy_ops = {
    .try_connect = android_proxy_try_connect,
    .remove = android_proxy_remove,
};
// Inject TCP proxy implementation into SLIRP stack.
// Initialization of the proxy will happen later.
void qemu_android_init_http_proxy_ops() {
    slirp_proxy = &android_proxy_ops;
}

bool qemu_android_setup_http_proxy(const char* http_proxy) {
    if (!http_proxy || http_proxy[0] == '\0') {
        return true;
    }
    op_http_proxy = http_proxy;
    http_proxy_on = true;

   return true;
}

void qemu_android_remove_http_proxy() {
    op_http_proxy = NULL;
    http_proxy_on = false;
}

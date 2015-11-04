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

#include "android/emulation/control/net_agent.h"

#include "android/utils/sockets.h"
#include "qemu/sockets.h"
#include "net/slirp.h"

#ifdef CONFIG_SLIRP
#include "libslirp.h"

static bool isSlirpInited() {
    return net_slirp_is_inited() != 0;
}

static bool slirpRedir(bool isUdp, int hostPort,
                      uint32_t guestAddr, int guestPort) {
    struct in_addr host = { .s_addr = htonl(SOCK_ADDRESS_INET_LOOPBACK) };
    struct in_addr guest = { .s_addr = guestAddr };
    return slirp_add_hostfwd(net_slirp_lookup(NULL, NULL, NULL), isUdp,
                             host, hostPort, guest, guestPort) == 0;
}

bool slirpUnredir(bool isUdp, int hostPort) {
    struct in_addr host = { .s_addr = htonl(SOCK_ADDRESS_INET_LOOPBACK) };
    return slirp_remove_hostfwd(net_slirp_lookup(NULL, NULL, NULL), isUdp,
                                host, hostPort) == 0;
}


static const QAndroidNetAgent netAgent = {
        .isSlirpInited = &isSlirpInited,
        .slirpRedir = &slirpRedir,
        .slirpUnredir = &slirpUnredir
};
const QAndroidNetAgent* const gQAndroidNetAgent = &netAgent;

#else
const QAndroidNetAgent* const gQAndroidNetAgent = NULL;

#endif

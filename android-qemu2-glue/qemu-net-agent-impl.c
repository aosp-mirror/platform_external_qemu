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

#include "android-qemu2-glue/emulation/virtio-wifi.h"
#include "android/utils/sockets.h"
#include "net/slirp.h"
#include "qemu/osdep.h"
#include "qemu/sockets.h"

#ifdef CONFIG_SLIRP
#include "libslirp.h"

static bool isSlirpInited() {
    return net_slirp_state() != NULL;
}

static bool slirpRedir(bool isUdp, int hostPort, int guestPort) {
    struct in_addr host = { .s_addr = htonl(SOCK_ADDRESS_INET_LOOPBACK) };
    struct in_addr guest = { .s_addr = 0 };
    return slirp_add_hostfwd(net_slirp_state(), isUdp, host, hostPort, guest,
                             guestPort) == 0;
}

bool slirpUnredir(bool isUdp, int hostPort) {
    struct in_addr host = { .s_addr = htonl(SOCK_ADDRESS_INET_LOOPBACK) };
    return slirp_remove_hostfwd(net_slirp_state(), isUdp, host, hostPort) == 0;
}

static bool slirpRedirIpv6(bool isUdp, int hostPort, int guestPort) {
    struct in6_addr host = in6addr_loopback;
    struct in6_addr guest = { .s6_addr = 0 };
    return slirp_add_ipv6_hostfwd(net_slirp_state(), isUdp, host, hostPort,
                                  guest, guestPort) == 0;
}

static bool slirpUnredirIpv6(bool isUdp, int hostPort) {
    struct in6_addr host = in6addr_loopback;
    return slirp_remove_ipv6_hostfwd(net_slirp_state(), isUdp, host,
                                     hostPort) == 0;
}

static bool slirpBlockSsid(const char* ssid, bool enable) {
    const uint8_t* ethaddr = virtio_wifi_ssid_to_ethaddr(ssid);
    if (ethaddr) {
        slirp_block_src_ethaddr(ethaddr, enable);
        return true;
    }
    else {
        return false;
    }
}

static const QAndroidNetAgent netAgent = {
        .isSlirpInited = &isSlirpInited,
        .slirpRedir = &slirpRedir,
        .slirpUnredir = &slirpUnredir,
        .slirpRedirIpv6 = &slirpRedirIpv6,
        .slirpUnredirIpv6 = &slirpUnredirIpv6,
        .slirpBlockSsid = &slirpBlockSsid,
};
const QAndroidNetAgent* const gQAndroidNetAgent = &netAgent;

#else
const QAndroidNetAgent* const gQAndroidNetAgent = NULL;

#endif

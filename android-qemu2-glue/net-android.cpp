// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "net-android.h"
#include "android/android.h"
#include "android/network/constants.h"
#include "android/network/globals.h"
#include "android/telephony/modem_driver.h"
#include "android/shaper.h"

extern "C" {
#include "net/net.h"
}

#if defined(CONFIG_SLIRP)
static void* s_slirp_state = nullptr;
static void* s_net_client_state = nullptr;
static Slirp* s_slirp = nullptr;

static void
android_net_delay_in_cb(void* data, size_t size, void* opaque)
{
    slirp_input(s_slirp, static_cast<const uint8_t*>(data), size);
}

static void
android_net_shaper_in_cb(void* data, size_t size, void* opaque)
{
    netdelay_send_aux(android_net_delay_in, data, size, opaque);
}

static void
android_net_shaper_out_cb(void* data, size_t size, void* opaque)
{
    qemu_send_packet(static_cast<NetClientState*>(s_net_client_state),
                     static_cast<const uint8_t*>(data),
                     size);
}

void
slirp_init_shapers(void* slirp_state, void* net_client_state, Slirp* slirp)
{
    s_slirp_state = slirp_state;
    s_net_client_state = net_client_state;
    s_slirp = slirp;
    android_net_delay_in = netdelay_create(android_net_delay_in_cb);
    android_net_shaper_in = netshaper_create(1, android_net_shaper_in_cb);
    android_net_shaper_out = netshaper_create(1, android_net_shaper_out_cb);

    netdelay_set_latency(android_net_delay_in, android_net_min_latency,
                         android_net_max_latency);
    netshaper_set_rate(android_net_shaper_out, android_net_download_speed);
    netshaper_set_rate(android_net_shaper_in, android_net_upload_speed);
}
#endif  // CONFIG_SLIRP


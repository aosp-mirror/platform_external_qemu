// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android-qemu2-glue/net-android.h"

#include "android/android.h"
#include "android/network/constants.h"
#include "android/network/globals.h"
#include "android/telephony/modem_driver.h"
#include "android/shaper.h"
#include "android/tcpdump.h"

extern "C" {
#include "qemu/osdep.h"
#include "net/slirp.h"
}

void android_qemu_init_slirp_shapers(void)
{
    if (net_slirp_state() == nullptr) {
        // When running with a TAP interface slirp will not be active and in
        // that case it doesn't make sense to set up the shapers.
        return;
    }
    android_net_delay_in = netdelay_create(
            [](void* data, size_t size, void* opaque) {
                net_slirp_receive_raw(opaque,
                                      static_cast<const uint8_t*>(data),
                                      static_cast<int>(size));
            });

    android_net_shaper_in = netshaper_create(1,
            [](void* data, size_t size, void* opaque) {
                netdelay_send_aux(android_net_delay_in, data, size, opaque);
            });

    android_net_shaper_out = netshaper_create(1,
            [](void* data, size_t size, void* opaque) {
                net_slirp_output_raw(opaque,
                                     static_cast<const uint8_t*>(data),
                                     static_cast<int>(size));
            });

    netdelay_set_latency(android_net_delay_in, android_net_min_latency,
                         android_net_max_latency);

    netshaper_set_rate(android_net_shaper_out, android_net_download_speed);
    netshaper_set_rate(android_net_shaper_in, android_net_upload_speed);

    net_slirp_set_shapers(
            android_net_shaper_out,
            [](void* opaque, const void* data, int len, void* slirp_state) {
                if (qemu_tcpdump_active) {
                    qemu_tcpdump_packet(data, len);
                }
                netshaper_send_aux(static_cast<NetShaper>(opaque),
                                   (char*)data, len, slirp_state);
            },
            android_net_shaper_in,
            [](void* opaque, const void* data, int len, void* slirp_state) {
                if (qemu_tcpdump_active) {
                    qemu_tcpdump_packet(data, len);
                }

                netshaper_send_aux(static_cast<NetShaper>(opaque),
                                   (void*)data, len, slirp_state);
            });
}

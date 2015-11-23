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
#include "android/telephony/modem_driver.h"
#include "android/shaper.h"

double   qemu_net_upload_speed   = 0.;
double   qemu_net_download_speed = 0.;
int      qemu_net_min_latency = 0;
int      qemu_net_max_latency = 0;

NetShaper  slirp_shaper_in;
NetShaper  slirp_shaper_out;
NetDelay   slirp_delay_in;

#if defined(CONFIG_SLIRP)
static void* s_slirp_state = nullptr;
static Slirp* s_slirp = nullptr;

static void
slirp_delay_in_cb(void* data, size_t size, void* opaque)
{
    slirp_input(s_slirp, static_cast<const uint8_t*>(data), size);
}

static void
slirp_shaper_in_cb(void* data, size_t size, void* opaque)
{
    netdelay_send_aux(slirp_delay_in, data, size, opaque);
}

static void
slirp_shaper_out_cb(void* data, size_t size, void* opaque)
{
    slirp_output(s_slirp_state, static_cast<const uint8_t*>(data), size);
}

void
slirp_init_shapers(void* slirp_state, Slirp* slirp)
{
    s_slirp_state = slirp_state;
    s_slirp = slirp;
    slirp_delay_in = netdelay_create(slirp_delay_in_cb);
    slirp_shaper_in = netshaper_create(1, slirp_shaper_in_cb);
    slirp_shaper_out = netshaper_create(1, slirp_shaper_out_cb);

    netdelay_set_latency(slirp_delay_in, qemu_net_min_latency,
                         qemu_net_max_latency);
    netshaper_set_rate(slirp_shaper_out, qemu_net_download_speed);
    netshaper_set_rate(slirp_shaper_in, qemu_net_upload_speed);
}
#endif  // CONFIG_SLIRP

int
android_parse_network_speed(const char*  speed)
{
    int          n;
    char*  end;
    double       sp;

    if (speed == NULL || speed[0] == 0) {
        speed = DEFAULT_NETSPEED;
    }

    for (n = 0; android_netspeeds[n].name != NULL; n++) {
        if (!strcmp(android_netspeeds[n].name, speed)) {
            qemu_net_download_speed = android_netspeeds[n].download;
            qemu_net_upload_speed   = android_netspeeds[n].upload;
            return 0;
        }
    }

    /* is this a number ? */
    sp = strtod(speed, &end);
    if (end == speed) {
        return -1;
    }

    qemu_net_download_speed = qemu_net_upload_speed = sp*1000.;
    if (*end == ':') {
        speed = end+1;
        sp = strtod(speed, &end);
        if (end > speed) {
            qemu_net_download_speed = sp*1000.;
        }
    }

    if (android_modem)
        amodem_set_data_network_type( android_modem,
                                      android_parse_network_type(speed) );
    return 0;
}


int
android_parse_network_latency(const char*  delay)
{
    int  n;
    char*  end;
    double  sp;

    if (delay == NULL || delay[0] == 0)
        delay = DEFAULT_NETDELAY;

    for (n = 0; android_netdelays[n].name != NULL; n++) {
        if ( !strcmp( android_netdelays[n].name, delay ) ) {
            qemu_net_min_latency = android_netdelays[n].min_ms;
            qemu_net_max_latency = android_netdelays[n].max_ms;
            return 0;
        }
    }

    /* is this a number ? */
    sp = strtod(delay, &end);
    if (end == delay) {
        return -1;
    }

    qemu_net_min_latency = qemu_net_max_latency = (int)sp;
    if (*end == ':') {
        delay = (const char*)end+1;
        sp = strtod(delay, &end);
        if (end > delay) {
            qemu_net_max_latency = (int)sp;
        }
    }
    return 0;
}

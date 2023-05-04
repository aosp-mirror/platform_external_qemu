// Copyright (C) 2023 The Android Open Source Project
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

#include "android-qemu2-glue/emulation/libslirp_driver.h"

#include "android/base/system/System.h"
#include "android/utils/debug.h"

extern "C" {
#include "libslirp.h"
#include "util.h"
#include "ip6.h"

#ifdef __linux__
#include <poll.h>
#endif
}

namespace android {
namespace qemu2 {

static slirp_rx_callback s_rx_callback = nullptr;
static Slirp* s_slirp = nullptr;
static GArray* s_gpollfds;

static int get_str_sep(char* buf, int buf_size, const char** pp, int sep) {
    const char *p, *p1;
    int len;
    p = *pp;
    p1 = strchr(p, sep);
    if (!p1)
        return -1;
    len = p1 - p;
    p1++;
    if (buf_size > 0) {
        if (len > buf_size - 1)
            len = buf_size - 1;
        memcpy(buf, p, len);
        buf[len] = '\0';
    }
    *pp = p1;
    return 0;
}

static slirp_ssize_t libslirp_send_packet(const void* pkt,
                                          size_t pkt_len,
                                          void* opaque) {
    if (s_rx_callback) {
        return s_rx_callback((const uint8_t*)pkt, pkt_len);
    } else {
        dwarning("libslirp: receive callback is not registered.\n");
        return 0;
    }
}

static void libslirp_guest_error(const char* msg, void* opaque) {
    dinfo("libslirp: %s\n", msg);
}

static int64_t libslirp_clock_get_ns(void* opaque) {
    return android::base::System::getSystemTimeUs() * 1000;
}

static void libslirp_init_completed(Slirp* slirp, void* opaque) {
    dinfo("libslirp: initialization completed.\n");
}

static void libslirp_timer_cb(void* opaque) {
    /*TODO(wdu@): Un-implemented because the function callback is only used
     * by icmp. */
}

static void* libslirp_timer_new_opaque(SlirpTimerId id,
                                       void* cb_opaque,
                                       void* opaque) {
    /*TODO(wdu@): Un-implemented because the function callback is only used
     * by icmp. */
    return nullptr;
}

static void libslirp_timer_free(void* timer, void* opaque) {
    /*TODO(wdu@): Un-implemented because the function callback is only used
     * by icmp. */
}

static void libslirp_timer_mod(void* timer,
                               int64_t expire_timer,
                               void* opaque) {
    /*TODO(wdu@): Un-implemented because the function callback is only used
     * by icmp. */
}

static void libslirp_register_poll_fd(int fd, void* opaque) {
    /*TODO(wdu@): Need implementation for Windows*/
}

static void libslirp_unregister_poll_fd(int fd, void* opaque) {
    /*TODO(wdu@): Need implementation for Windows*/
}

static void libslirp_notify(void* opaque) {
    /*TODO(wdu@): Un-implemented.*/
}

static const SlirpCb slirp_cb = {
        .send_packet = libslirp_send_packet,
        .guest_error = libslirp_guest_error,
        .clock_get_ns = libslirp_clock_get_ns,
        .timer_free = libslirp_timer_free,
        .timer_mod = libslirp_timer_mod,
        .register_poll_fd = libslirp_register_poll_fd,
        .unregister_poll_fd = libslirp_unregister_poll_fd,
        .notify = libslirp_notify,
        .init_completed = libslirp_init_completed,
        .timer_new_opaque = libslirp_timer_new_opaque,
};

Slirp* libslirp_init(slirp_rx_callback rx_callback,
                     int restricted,
                     bool ipv4,
                     const char* vnetwork,
                     const char* vhost,
                     bool ipv6,
                     const char* vprefix6,
                     int vprefix6_len,
                     const char* vhost6,
                     const char* vhostname,
                     const char* tftp_export,
                     const char* bootfile,
                     const char* vdhcp_start,
                     const char* vnameserver,
                     const char* vnameserver6,
                     const char** dnssearch,
                     const char* vdomainname,
                     const char* tftp_server_name) {
    /* default settings according to historic slirp */
    struct in_addr net = {.s_addr = htonl(0x0a000200)};  /* 10.0.2.0 */
    struct in_addr mask = {.s_addr = htonl(0xffffff00)}; /* 255.255.255.0 */
    struct in_addr host = {.s_addr = htonl(0x0a000202)}; /* 10.0.2.2 */
    struct in_addr dhcp = {.s_addr = htonl(0x0a000210)}; /* 10.0.2.16 */
    struct in_addr dns = {.s_addr = htonl(0x0a000203)};  /* 10.0.2.3 */
    struct in6_addr ip6_prefix;
    struct in6_addr ip6_host;
    struct in6_addr ip6_dns;
    SlirpConfig cfg = {0};
    Slirp* slirp = nullptr;
    char buf[20];
    uint32_t addr;
    int shift;
    char* end;
    s_rx_callback = std::move(rx_callback);
#ifndef _WIN32
    signal(SIGPIPE, SIG_IGN);
#endif
    s_gpollfds = g_array_new(FALSE, FALSE, sizeof(GPollFD));

    if (!ipv4 && (vnetwork || vhost || vnameserver)) {
        derror("IPv4 disabled but netmask/host/dns provided");
        return nullptr;
    }

    if (!ipv6 && (vprefix6 || vhost6 || vnameserver6)) {
        derror("IPv6 disabled but prefix/host6/dns6 provided");
        return nullptr;
    }

    if (!ipv4 && !ipv6) {
        /* It doesn't make sense to disable both */
        derror("IPv4 and IPv6 disabled");
        return nullptr;
    }

    if (vnetwork) {
        if (get_str_sep(buf, sizeof(buf), &vnetwork, '/') < 0) {
            if (!inet_aton(vnetwork, &net)) {
                derror("Failed to parse netmask");
                return nullptr;
            }
            addr = ntohl(net.s_addr);
            if (!(addr & 0x80000000)) {
                mask.s_addr = htonl(0xff000000); /* class A */
            } else if ((addr & 0xfff00000) == 0xac100000) {
                mask.s_addr = htonl(0xfff00000); /* priv. 172.16.0.0/12 */
            } else if ((addr & 0xc0000000) == 0x80000000) {
                mask.s_addr = htonl(0xffff0000); /* class B */
            } else if ((addr & 0xffff0000) == 0xc0a80000) {
                mask.s_addr = htonl(0xffff0000); /* priv. 192.168.0.0/16 */
            } else if ((addr & 0xffff0000) == 0xc6120000) {
                mask.s_addr = htonl(0xfffe0000); /* tests 198.18.0.0/15 */
            } else if ((addr & 0xe0000000) == 0xe0000000) {
                mask.s_addr = htonl(0xffffff00); /* class C */
            } else {
                mask.s_addr = htonl(0xfffffff0); /* multicast/reserved */
            }
        } else {
            if (!inet_aton(buf, &net)) {
                derror("Failed to parse netmask");
                return nullptr;
            }
            shift = strtol(vnetwork, &end, 10);
            if (*end != '\0') {
                if (!inet_aton(vnetwork, &mask)) {
                    derror("Failed to parse netmask (trailing chars)");
                    return nullptr;
                }
            } else if (shift < 4 || shift > 32) {
                derror("Invalid netmask provided (must be in range 4-32)");
                return nullptr;
            } else {
                mask.s_addr = htonl(0xffffffff << (32 - shift));
            }
        }
        net.s_addr &= mask.s_addr;
        host.s_addr = net.s_addr | (htonl(0x0202) & ~mask.s_addr);
        dhcp.s_addr = net.s_addr | (htonl(0x020f) & ~mask.s_addr);
        dns.s_addr = net.s_addr | (htonl(0x0203) & ~mask.s_addr);
    }

    if (vhost && !inet_aton(vhost, &host)) {
        derror("Failed to parse host");
        return nullptr;
    }
    if ((host.s_addr & mask.s_addr) != net.s_addr) {
        derror("Host doesn't belong to network");
        return nullptr;
    }

    if (vnameserver && !inet_aton(vnameserver, &dns)) {
        derror("Failed to parse DNS");
        return nullptr;
    }
    if (restricted && (dns.s_addr & mask.s_addr) != net.s_addr) {
        derror("DNS doesn't belong to network");
        return nullptr;
    }
    if (dns.s_addr == host.s_addr) {
        derror("DNS must be different from host");
        return nullptr;
    }

    if (vdhcp_start && !inet_aton(vdhcp_start, &dhcp)) {
        derror("Failed to parse DHCP start address");
        return nullptr;
    }
    if ((dhcp.s_addr & mask.s_addr) != net.s_addr) {
        derror("DHCP doesn't belong to network");
        return nullptr;
    }
    if (dhcp.s_addr == host.s_addr || dhcp.s_addr == dns.s_addr) {
        derror("DHCP must be different from host and DNS");
        return nullptr;
    }

    if (!vprefix6) {
        vprefix6 = "fec0::";
    }
    if (!inet_pton(AF_INET6, vprefix6, &ip6_prefix)) {
        derror("Failed to parse IPv6 prefix");
        return nullptr;
    }

    if (!vprefix6_len) {
        vprefix6_len = 64;
    }
    if (vprefix6_len < 0 || vprefix6_len > 126) {
        derror("Invalid IPv6 prefix provided "
               "(IPv6 prefix length must be between 0 and 126)");
        return nullptr;
    }

    if (vhost6) {
        if (!inet_pton(AF_INET6, vhost6, &ip6_host)) {
            derror("Failed to parse IPv6 host");
            return nullptr;
        }
        if (!in6_equal_net(&ip6_prefix, &ip6_host, vprefix6_len)) {
            derror("IPv6 Host doesn't belong to network");
            return nullptr;
        }
    } else {
        ip6_host = ip6_prefix;
        ip6_host.s6_addr[15] |= 2;
    }

    if (vnameserver6) {
        if (!inet_pton(AF_INET6, vnameserver6, &ip6_dns)) {
            derror("Failed to parse IPv6 DNS");
            return nullptr;
        }
        if (restricted && !in6_equal_net(&ip6_prefix, &ip6_dns, vprefix6_len)) {
            derror("IPv6 DNS doesn't belong to network");
            return nullptr;
        }
    } else {
        ip6_dns = ip6_prefix;
        ip6_dns.s6_addr[15] |= 3;
    }

    if (vdomainname && !*vdomainname) {
        derror("'domainname' parameter cannot be empty");
        return nullptr;
    }

    if (vdomainname && strlen(vdomainname) > 255) {
        derror("'domainname' parameter cannot exceed 255 bytes");
        return nullptr;
    }

    if (vhostname && strlen(vhostname) > 255) {
        derror("'vhostname' parameter cannot exceed 255 bytes");
        return nullptr;
    }

    if (tftp_server_name && strlen(tftp_server_name) > 255) {
        derror("'tftp-server-name' parameter cannot exceed 255 bytes");
        return nullptr;
    }

    cfg.version = 4;
    cfg.restricted = restricted;
    cfg.in_enabled = ipv4;
    cfg.vnetwork = net;
    cfg.vnetmask = mask;
    cfg.vhost = host;
    cfg.in6_enabled = ipv6;
    cfg.vprefix_addr6 = ip6_prefix;
    cfg.vprefix_len = vprefix6_len;
    cfg.vhost6 = ip6_host;
    cfg.vhostname = vhostname;
    cfg.tftp_server_name = tftp_server_name;
    cfg.tftp_path = tftp_export;
    cfg.bootfile = bootfile;
    cfg.vdhcp_start = dhcp;
    cfg.vnameserver = dns;
    cfg.vnameserver6 = ip6_dns;
    cfg.vdnssearch = dnssearch;
    cfg.vdomainname = vdomainname;
    slirp = slirp_new(&cfg, &slirp_cb, nullptr);
    s_slirp = slirp;
    return slirp;
}

}  // namespace qemu2
}  // namespace android
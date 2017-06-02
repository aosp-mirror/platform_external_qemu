/*
 * Copyright (c) 2017
 *	Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * Support ping using the system ping binary on Linux and Mac
 * On Linux/Mac, icmp protocol requires the process to have system permission.
 * Our emulator, by default, does not have system permission, so we have to
 * resort to the built-in ping binary to handle icmp messages.
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "slirp.h"
#include "ip_icmp.h"

int ping_binary_send(struct socket* so, struct mbuf* m, int hlen) {
    struct ip* ip = mtod(m, struct ip*);
    const char* dest_ip_addr = inet_ntoa(ip->ip_dst);
    char ping_cmd[1024];
    sprintf(ping_cmd, "ping -c 1 -t %d %s 2>&1", ip->ip_ttl, dest_ip_addr);
    FILE* p = popen(ping_cmd, "r");
    if (!p) {
        error_report("popen failed for ping binary");
        return -1;
    }

    so->ping_pipe = p;

    // this is rather critical, otherwise emulator blocks when pinging
    // an unreachable host
    so->s = fileno(p);
    fcntl(so->s, F_SETFL, O_NONBLOCK);

    return 0;
}

/*
ping -c 1 www.google.com
PING www.google.com (172.217.6.36) 56(84) bytes of data.
64 bytes from sfo03s08-in-f36.1e100.net (172.217.6.36): icmp_seq=1 ttl=57
time=1.31 ms

--- www.google.com ping statistics ---
1 packets transmitted, 1 received, 0% packet loss, time 0ms
rtt min/avg/max/mdev = 1.316/1.316/1.316/0.000 ms
*/
int ping_binary_recv(struct socket* so, struct ip* ip, struct icmp* icp) {
    if (so == NULL || so->ping_pipe == NULL) {
        return -1;
    }

    int seq = -1, ttl = -1;
    char line[4096] = "";
    char skip[4096] = "";
    char skip2[4096] = "";
    while (fgets(line, sizeof(line), so->ping_pipe)) {
        char* p = strstr(line, "icmp_seq=");
        if (p != NULL) {
            p += strlen("icmp_seq=");
            seq = atoi(p);
        }
        p = strstr(line, "ttl=");
        if (p != NULL) {
            p += strlen("ttl=");
            ttl = atoi(p);
        }

        if (seq != -1 && ttl != -1) {
            break;
        }
    }

    if (seq != -1 && ttl != -1) {
        ip->ip_ttl = ttl;
        ip->ip_p = IPPROTO_ICMP;
        icp->icmp_type = ICMP_ECHOREPLY;
        icp->icmp_code = 0;
        icp->icmp_cksum = 0;
        // icp->icmp_seq = seq;
        return sizeof(*icp);
    }

    return -1;
}

void ping_binary_close(struct socket* so) {
    if (so != NULL && so->ping_pipe) {
        pclose(so->ping_pipe);
        so->ping_pipe = NULL;
    }
}

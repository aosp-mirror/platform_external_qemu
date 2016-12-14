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

#include "android/utils/dns.h"

#include "android/base/Log.h"

#include <gtest/gtest.h>
#include <string>

static std::string ipToString(uint32_t ip) {
    char temp[32];
    snprintf(temp, sizeof(temp), "%d.%d.%d.%d", (ip >> 24) & 255,
             (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);
    return std::string(temp);
}

TEST(android_dns_parse_servers, IgnoreIpv6) {
    const int kMaxAddresses = 3;
    uint32_t addresses[kMaxAddresses] = {};
    const char kList[] = "127.0.0.1,::1,127.0.1.1";
    EXPECT_EQ(2, android_dns_parse_servers(kList, addresses, kMaxAddresses));
    EXPECT_EQ(0x7f000001U, addresses[0]);
    EXPECT_EQ(0x7f000101U, addresses[1]);
}

TEST(android_dns_get_system_servers, DumpList) {
    const size_t kMaxServers = ANDROID_MAX_DNS_SERVERS;
    uint32_t ips[kMaxServers];
    int count = android_dns_get_system_servers(ips, kMaxServers);
    EXPECT_GT(count, 0);
    EXPECT_LT(count, (int)kMaxServers);
    for (int n = 0; n < count; ++n) {
        LOG(INFO) << "DNS nameserver " << ipToString(ips[n]);
    }
}

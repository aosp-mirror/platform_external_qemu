// Copyright (C) 2022 The Android Open Source Project
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

#include "android-qemu2-glue/emulation/WifiService.h"

#include "aemu/base/Log.h"
#include "android-qemu2-glue/emulation/NetsimWifiForwarder.h"
#include "android-qemu2-glue/emulation/VirtioWifiForwarder.h"
#include "android-qemu2-glue/qemu-setup.h"
#include "android/emulation/HostapdController.h"
#include "android/utils/debug.h"

using android::network::MacAddress;

extern "C" {
#include "qemu/osdep.h"
#include "net/slirp.h"
#include "slirp/libslirp.h"
}

#ifdef _WIN32
#include "aemu/base/sockets/Winsock.h"
#else
#include <arpa/inet.h>
#endif

namespace android {
namespace qemu2 {

using Builder = WifiService::Builder;
static const uint8_t kBssID[] = {0x00, 0x13, 0x10, 0x85, 0xfe, 0x01};
static const char* kNetwork = "10.0.2.0";
static const char* kMask = "255.255.255.0";
static const char* kHost = "10.0.2.2";
static const char* kDhcp = "10.0.2.15";
static const char* kDns = "10.0.2.3";
static const uint8_t kPrefix6Len = 64;
static const char* kPrefix6 = "fec0::";
static const char* kHost6 = "fec0::2";
static const char* kDns6 = "fec0::3";

static WifiService::OnReceiveCallback sOnReceiveCallback = nullptr;

static Slirp* initializeSlirp(SlirpOptions opts) {
    if (opts.disabled) {
        return nullptr;
    }

    struct in_addr net;
    struct in_addr mask;
    struct in_addr host;
    struct in_addr dhcp;
    struct in_addr dns;
    struct in6_addr ip6_prefix;
    struct in6_addr ip6_host;
    struct in6_addr ip6_dns;
    int vprefix6_len = opts.vprefixLen ? opts.vprefixLen : kPrefix6Len;
    if (!inet_pton(AF_INET, opts.vnet.empty() ? kNetwork : opts.vnet.c_str(),
                   &net)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv4 vnet: " << opts.vnet
                   << ", use default option: " << kNetwork;
        inet_pton(AF_INET, kNetwork, &net);
    }
    if (!inet_pton(AF_INET, opts.vmask.empty() ? kMask : opts.vmask.c_str(),
                   &mask)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv4 vmask: " << opts.vmask
                   << ", use default option: " << kMask;
        inet_pton(AF_INET, kMask, &mask);
    }
    if (!inet_pton(AF_INET, opts.vhost.empty() ? kHost : opts.vhost.c_str(),
                   &host)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv4 vhost: " << opts.vhost
                   << ", use default option: " << kHost;
        inet_pton(AF_INET, kHost, &host);
    }
    if (!inet_pton(AF_INET,
                   opts.dhcpstart.empty() ? kDhcp : opts.dhcpstart.c_str(),
                   &dhcp)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv4 dhcp start: "
                   << opts.dhcpstart << ", use default option: " << kDhcp;
        inet_pton(AF_INET, kDhcp, &dhcp);
    }
    if (!inet_pton(AF_INET, opts.dns.empty() ? kDns : opts.dns.c_str(), &dns)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv4 dns: " << opts.dns
                   << ", use default option: " << kDns;
        inet_pton(AF_INET, kDns, &dns);
    }

    if (!inet_pton(AF_INET6,
                   opts.vprefix6.empty() ? kPrefix6 : opts.vprefix6.c_str(),
                   &ip6_prefix)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv6 vprefix: " << opts.vprefix6
                   << ", use default option: " << kPrefix6;
        inet_pton(AF_INET6, kPrefix6, &ip6_prefix);
    }

    if (!inet_pton(AF_INET6, opts.vhost6.empty() ? kHost6 : opts.vhost6.c_str(),
                   &ip6_host)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv6 vhost: " << opts.vhost6
                   << ", use default option: " << kHost6;
        inet_pton(AF_INET6, kHost6, &ip6_host);
    }

    if (!inet_pton(AF_INET6, opts.dns6.empty() ? kDns6 : opts.dns6.c_str(),
                   &ip6_dns)) {
        LOG(DEBUG) << "Failed to parse Slirp IPv6 dns: " << opts.dns6
                   << ", use default option: " << kDns6;
        inet_pton(AF_INET6, kHost6, &ip6_dns);
    }
    Slirp* s = static_cast<Slirp*>(net_slirp_init_custom_slirp_state(
            opts.restricted, opts.ipv4, net, mask, host, opts.ipv6, ip6_prefix,
            vprefix6_len, ip6_host, opts.vhostname.c_str(),
            opts.tftpath.c_str(), opts.bootfile.c_str(), dhcp, dns, ip6_dns,
            nullptr /*vdnssearch*/));
    return s;
}

Builder::Builder() = default;

Builder& Builder::withRedirectToNetsim(bool enable) {
    mRedirectToNetsim = enable;
    return *this;
}

Builder& Builder::withHostapd(HostapdOptions options) {
    mHostapdOpts = options;
    return *this;
}

Builder& Builder::withSlirp(SlirpOptions options) {
    mSlirpOpts = options;
    return *this;
}

Builder& Builder::withVerboseLogging(bool verbose) {
    mVerbose = verbose;
    return *this;
}
Builder& Builder::withBssid(std::vector<uint8_t> bssid) {
    mBssID = std::move(bssid);
    return *this;
}

Builder& Builder::withNicConf(NICConf* conf) {
    mNicConf = conf;
    return *this;
}

Builder& Builder::withServerPort(uint16_t port) {
    mServerPort = port;
    return *this;
}

Builder& Builder::withClientPort(uint16_t port) {
    mClientPort = port;
    return *this;
}

Builder& Builder::withOnReceiveCallback(WifiService::OnReceiveCallback cb) {
    // Set function callback for both stat
    sOnReceiveCallback = std::move(cb);
    return *this;
}

Builder& Builder::withOnSentCallback(WifiService::OnSentCallback cb) {
    mOnSentCallback = std::move(cb);
    return *this;
}

Builder& Builder::withCanReceiveCallback(WifiService::CanReceiveCallback cb) {
    mCanReceive = std::move(cb);
    return *this;
}

Builder& Builder::withOnLinkStatusChangedCallback(
        WifiService::OnLinkStatusChangedCallback cb) {
    mOnLinkStatusChanged = std::move(cb);
    return *this;
}

std::shared_ptr<WifiService> Builder::build() {
    if (!mHostapdOpts.disabled) {
        bool success = false;
        auto* hostapd = android::emulation::HostapdController::getInstance();
        if (hostapd->init(mVerbose)) {
            if (!mHostapdOpts.ssid.empty()) {
                hostapd->setSsid(
                    std::move(mHostapdOpts.ssid), std::move(mHostapdOpts.passwd));
            }
            success = hostapd->run();
        }
        if (!success) {
            LOG(ERROR) << "Failed to initialize WiFi service: hostpad event "
                          "loop failed to start";
            return nullptr;
        }
    }

    if (mRedirectToNetsim) {
        return std::static_pointer_cast<WifiService>(
                std::make_shared<NetsimWifiForwarder>(sOnReceiveCallback,
                                                      mCanReceive));
    }

    Slirp* slirp = initializeSlirp(mSlirpOpts);

    if (!mSlirpOpts.disabled && !slirp) {
        LOG(ERROR) << "Failed to initialize WiFi service: Slirp initialization "
                      "failure.";
        return nullptr;
    }
    auto virtioWifi = std::make_shared<VirtioWifiForwarder>(
            mBssID.empty() ? kBssID : mBssID.data(), sOnReceiveCallback,
            mOnLinkStatusChanged, mCanReceive, mOnSentCallback, mNicConf, slirp,
            mServerPort, mClientPort);
    return std::static_pointer_cast<WifiService>(virtioWifi);
}

}  // namespace qemu2
}  // namespace android
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
#ifdef LIBSLIRP
#include "android-qemu2-glue/emulation/libslirp_driver.h"
#endif

using android::network::MacAddress;

extern "C" {
#include "net/slirp.h"
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
#ifdef LIBSLIRP
static std::weak_ptr<VirtioWifiForwarder> sVirtioWifiWeakPtr;
static ssize_t libslirp_send_packet(const void* pkt, size_t pkt_len) {
    if (auto virtioWifi = sVirtioWifiWeakPtr.lock()) {
        return virtioWifi->onRxPacketAvailable((const uint8_t*)pkt, pkt_len);
    } else {
        dwarning("libslirp: virtio wifi forwarder is null.\n");
        return 0;
    }
}
#endif

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

    Slirp* slirp = nullptr;
#ifdef LIBSLIRP
    if (!mSlirpOpts.disabled) {
        slirp = libslirp_init(
                libslirp_send_packet, mSlirpOpts.restricted, mSlirpOpts.ipv4,
                mSlirpOpts.vnet.empty() ? nullptr : mSlirpOpts.vnet.c_str(),
                mSlirpOpts.vhost.empty() ? nullptr : mSlirpOpts.vhost.c_str(),
                mSlirpOpts.ipv6,
                mSlirpOpts.vprefix6.empty() ? nullptr
                                            : mSlirpOpts.vprefix6.c_str(),
                mSlirpOpts.vprefixLen,
                mSlirpOpts.vhost6.empty() ? nullptr : mSlirpOpts.vhost6.c_str(),
                mSlirpOpts.vhostname.empty() ? nullptr
                                             : mSlirpOpts.vhostname.c_str(),
                mSlirpOpts.tftpath.empty() ? nullptr
                                           : mSlirpOpts.tftpath.c_str(),
                mSlirpOpts.bootfile.empty() ? nullptr
                                            : mSlirpOpts.bootfile.c_str(),
                mSlirpOpts.dhcpstart.empty() ? nullptr
                                             : mSlirpOpts.dhcpstart.c_str(),
                mSlirpOpts.dns.empty() ? nullptr : mSlirpOpts.dns.c_str(),
                mSlirpOpts.dns6.empty() ? nullptr : mSlirpOpts.dns6.c_str(),
                nullptr /* dnssearch */, nullptr /* vdomainname */,
                nullptr /* tftp_server_name */);
    }
#endif

    if (!mSlirpOpts.disabled && !slirp) {
        LOG(ERROR) << "Failed to initialize WiFi service: Slirp initialization "
                      "failure.";
        return nullptr;
    }

    auto virtioWifi = std::make_shared<VirtioWifiForwarder>(
            mBssID.empty() ? kBssID : mBssID.data(), sOnReceiveCallback,
            mOnLinkStatusChanged, mCanReceive, mOnSentCallback, mNicConf, slirp,
            mServerPort, mClientPort);
#ifdef LIBSLIRP
    sVirtioWifiWeakPtr = virtioWifi;
#endif
    return std::static_pointer_cast<WifiService>(virtioWifi);
}

}  // namespace qemu2
}  // namespace android
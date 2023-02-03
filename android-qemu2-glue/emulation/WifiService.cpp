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
#include "android/emulation/HostapdController.h"
#include "android/utils/debug.h"

using android::network::MacAddress;

namespace android {
namespace qemu2 {

using Builder = WifiService::Builder;

Builder::Builder() = default;

Builder& Builder::withRedirectToNetsim(bool enable) {
    mRedirectToNetsim = enable;
    return *this;
}

Builder& Builder::withHostapd(HostapdOptions options) {
    mHostapdOpts = options;
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
    mOnReceiveCallback = std::move(cb);
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

std::unique_ptr<WifiService> Builder::build() {
    if (!mHostapdOpts.disabled) {
        auto* hostapd = android::emulation::HostapdController::getInstance();
        if (hostapd->init(mVerbose)) {
            if (!mHostapdOpts.ssid.empty()) {
                hostapd->setSsid(
                    std::move(mHostapdOpts.ssid), std::move(mHostapdOpts.passwd));
            }
            hostapd->run();
        } else {
            derror("Failed to initialize hostpad event loop.");
        }
    }
    if (mRedirectToNetsim) {
        return std::unique_ptr<WifiService>(
                new NetsimWifiForwarder(mOnReceiveCallback, mCanReceive));
    } else {
        return std::unique_ptr<WifiService>(new VirtioWifiForwarder(
                mBssID.data(), mOnReceiveCallback, mOnLinkStatusChanged,
                mCanReceive, mNicConf, mOnSentCallback, mServerPort,
                mClientPort));
    }
}

}  // namespace qemu2
}  // namespace android
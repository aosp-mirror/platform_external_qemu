// Copyright 2020 The Android Open Source Project
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

#include "android/emulation/control/wifi_agent.h"

#include "android/network/wifi.h"
#include "android/featurecontrol/FeatureControl.h"
#ifndef AEMU_GFXSTREAM_BACKEND
#include "android-qemu2-glue/emulation/HostapdController.h"
#endif

namespace fc = android::featurecontrol;

static const QAndroidWifiAgent sQAndroidWifiAgent = {

    .add_ssid = [] (const char* ssid, const char* password) {
#ifndef AEMU_GFXSTREAM_BACKEND
        if (fc::isEnabled(fc::VirtioWifi)) {
            auto* hostapd = android::qemu2::HostapdController::getInstance();
            if (hostapd)
                return hostapd->addSsid(ssid, password);
            else 
                return false;
        } 
#endif
        if (fc::isEnabled(fc::WifiConfigurable)) {
            return android_wifi_add_ssid(ssid, password) == 0;
        } else {
            return false;
        }
    },

	.set_ssid_block_on = [] (const char* ssid, bool blocked) {
#ifndef AEMU_GFXSTREAM_BACKEND
        if (fc::isEnabled(fc::VirtioWifi)) {
            auto* hostapd = android::qemu2::HostapdController::getInstance();
            if (hostapd)
                return hostapd->blockOnSsid(ssid, blocked);
            else
                return false;
        }
#endif
        if (fc::isEnabled(fc::WifiConfigurable)) {
            return android_wifi_set_ssid_block_on(ssid, false) == 0;
        } else {
            return false;
        }
    },
};

extern "C" const QAndroidWifiAgent* const gQAndroidWifiAgent =
        &sQAndroidWifiAgent;


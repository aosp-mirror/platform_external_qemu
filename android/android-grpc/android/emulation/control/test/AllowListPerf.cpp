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
#include "android/emulation/control/secure/AllowList.h"
#include "benchmark/benchmark.h"

using android::emulation::control::AllowList;


// Default access list
std::string emulator_access = R"#(
// This contains the allow lists of the emulator gRPC endpoint.
// This list defines which sets of methods are accessible by whom.
//
// You can protect the gRPC services as follows:
//
// - Unprotected: The set of methods that can be invoked even when
//                no access token is presented. No security checks will
//                be performed when these methods are invoked.
//
// - allowlist: A set of json objects that specificies for each token issuer,
//              what is allowed and what requires an "aud" field.
//
//             - "iss": The token issuer.
//             - "allowed": List of methods which are allowed, even if no "aud" field
//                        is present on the jwt token.
//             - "protected": List of methods which are allowed *ONLY IF* the given method
//                        is present in the "aud" field of the jwt token.
//             Note: Methods that are not on the allowed or protected list will ALWAYS be rejected.
{
    // Set of methods that do not require any validations, they do not require a token.
    // You are always able to invoke this method, without presenting any form of authentication.
    // This is a list of regular expressions. Access will be granted if the regular expression
    // matches the endpoint.
    "unprotected": [
        // ".*" // Matches every method, no authentication will be used **DANGER**
        // "/android.emulation.control.SnapshotService.*" // Everyone can make snapshots.
    ],
    // List of methods that require a token, these are the methods
    // we will allow if you present a signed JWT token.
    "allowlist": [
        {
            // Removing android-studio from the allowlist *WILL* break the embedded emulator.
            // You probably do not want to change this.
            "iss": "android-studio", // Tokens issued by android-studio
            // Can access the following set of methods, even if the AUD claim for
            // the given method is *NOT* present.
            "allowed": [
                "/android.emulation.control.EmulatorController/closeExtendedControls",
                "/android.emulation.control.EmulatorController/getClipboard",
                "/android.emulation.control.EmulatorController/getDisplayConfigurations",
                "/android.emulation.control.EmulatorController/getPhysicalModel",
                "/android.emulation.control.EmulatorController/getScreenshot",
                "/android.emulation.control.EmulatorController/getStatus",
                "/android.emulation.control.EmulatorController/getVmState",
                "/android.emulation.control.EmulatorController/injectAudio",
                "/android.emulation.control.EmulatorController/rotateVirtualSceneCamera",
                "/android.emulation.control.EmulatorController/sendKey",
                "/android.emulation.control.EmulatorController/sendMouse",
                "/android.emulation.control.EmulatorController/sendTouch",
                "/android.emulation.control.EmulatorController/setClipboard",
                "/android.emulation.control.EmulatorController/setDisplayConfigurations",
                "/android.emulation.control.EmulatorController/setDisplayMode",
                "/android.emulation.control.EmulatorController/setPhysicalModel",
                "/android.emulation.control.EmulatorController/setVirtualSceneCameraVelocity",
                "/android.emulation.control.EmulatorController/setVmState",
                "/android.emulation.control.EmulatorController/showExtendedControls",
                "/android.emulation.control.EmulatorController/streamClipboard",
                "/android.emulation.control.EmulatorController/streamNotification",
                "/android.emulation.control.EmulatorController/streamScreenshot",
                // Snapshot related functions
                "/android.emulation.control.SnapshotService/DeleteSnapshot",
                "/android.emulation.control.SnapshotService/ListSnapshots",
                "/android.emulation.control.SnapshotService/LoadSnapshot",
                "/android.emulation.control.SnapshotService/PushSnapshot",
                "/android.emulation.control.SnapshotService/SaveSnapshot"
            ]
        },
        {
            // For tokens issued by gradle we have the following restrictions:
            "iss": "gradle",
            // Can access the following set of methods, even if the AUD claim for
            // the given method is *NOT* present.
            //
            // Usually these are methods that do not present a significant amount
            // of danger.
            "allowed": [
                "/android.emulation.control.EmulatorController/getSensor",
                "/android.emulation.control.EmulatorController/setSensor",
                "/android.emulation.control.EmulatorController/setPhysicalModel",
                "/android.emulation.control.EmulatorController/getPhysicalModel",
                "/android.emulation.control.EmulatorController/streamPhysicalModel",
                "/android.emulation.control.EmulatorController/setBattery",
                "/android.emulation.control.EmulatorController/getBattery",
                "/android.emulation.control.EmulatorController/setGps",
                "/android.emulation.control.EmulatorController/getGps",
                "/android.emulation.control.EmulatorController/sendPhone",
                "/android.emulation.control.EmulatorController/sendSms",
                "/android.emulation.control.EmulatorController/getStatus",
                "/android.emulation.control.EmulatorController/setDisplayConfigurations",
                "/android.emulation.control.EmulatorController/getDisplayConfigurations",
                "/android.emulation.control.EmulatorController/rotateVirtualSceneCamera",
                "/android.emulation.control.EmulatorController/setVirtualSceneCameraVelocity",
                "/android.emulation.control.EmulatorController/setPosture",
                "/android.emulation.control.EmulatorController/getBrightness",
                "/android.emulation.control.EmulatorController/setBrightness"
            ],
            // Set of methods that can *ONLY* be accessed if given regex matches
            // the entry on the "aud" claim.
            "protected": [
                "/android.emulation.control.EmulatorController/getScreenshot",
                "/android.emulation.control.EmulatorController/streamScreenshot",
                // Clipboard access can be used to exchange data between the guest
                // and the host.
                "/android.emulation.control.EmulatorController/setClipboard",
                "/android.emulation.control.EmulatorController/getClipboard",
                "/android.emulation.control.EmulatorController/streamClipboard",
                // Can be used to "authenticate" with biodata.
                "/android.emulation.control.EmulatorController/sendFingerprint",
                // Touch, key and mouse can be used to manipulate device state
                "/android.emulation.control.EmulatorController/sendKey",
                "/android.emulation.control.EmulatorController/sendTouch",
                "/android.emulation.control.EmulatorController/sendMouse",
                // Could be used to trigger the assistant through "Hey Google!"
                "/android.emulation.control.EmulatorController/injectAudio",
                "/android.emulation.control.EmulatorController/streamAudio",
                "/android.emulation.control.EmulatorController/getLogcat",
                "/android.emulation.control.EmulatorController/streamLogcat",
                // Could be used to observe the device state.
                "/android.emulation.control.EmulatorController/streamNotification"
            ]
        }
    ]
}
)#";

void BM_cached_results(benchmark::State& state) {
    auto list = AllowList::fromJson(emulator_access);
    int i = 0;
    while (state.KeepRunning()) {
        list->isRed("gradle", "foo" + std::to_string(i) + "/getGps");
    }
}

void BM_uncached_results(benchmark::State& state) {
    auto list = AllowList::fromJson(emulator_access);
    int i = 0;
    while (state.KeepRunning()) {
        i++;
        list->isRed("gradle", "foo" + std::to_string(i) + "/getGps");
    }
}

BENCHMARK(BM_cached_results);
BENCHMARK(BM_uncached_results);
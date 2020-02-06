// Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <memory>  // for unique_ptr
#include <string>

#include "android/console.h"  // for AndroidConsoleAgents
#include "android/emulation/control/EmulatorService.h"
#include "android/emulation/control/RtcBridge.h"  // for RtcBridge

namespace android {
namespace emulation {

namespace control {
class RtcBridge;

// A Factory that can produce and a functioning gRPC service to control the
// emulator. You should call teardown when the emulator shuts down to properly
// close all active connections.
class GrpcServices {
public:
    // Enables the gRPC service that binds on the given address on the first
    // port available in the port range [startPart, endPort).
    //
    // - |startPort| The beginning port range
    // - |endPort| The last range in the port. The service will NOT bind to this
    // port, should be > startPort.
    // - |address| The ip address to bind to. For example: "127.0.0.1",
    // "0.0.0.0".
    // -  |consoleAgents| The emulator console agents, used to communicate with
    // the emulator engine. |rtcBridge| The RTC bridge used to establish WebRTC
    // streams. |waterfall| The named waterfall mode to use, if any. Unavailable
    // on windows platforms.
    static EmulatorControllerService* setup(
            int startPort,
            int endPort,
            std::string address,
            const AndroidConsoleAgents* const consoleAgents,
            RtcBridge* rtcBridge,
            const char* waterfall);

    // Call this on emulator shut down to gracefully close down active gRPC
    // connections. Closing of connections is not instanteanous and can take up
    // to a second.
    static void teardown();

private:
    static std::unique_ptr<EmulatorControllerService> g_controler_service;
    static std::unique_ptr<RtcBridge> g_rtc_bridge;

    static const std::string kCertFileName;
    static const std::string kPrivateKeyFileName;
};
}  // namespace control
}  // namespace emulation
}  // namespace android

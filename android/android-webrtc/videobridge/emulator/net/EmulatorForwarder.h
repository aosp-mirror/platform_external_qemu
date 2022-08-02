// Copyright (C) 2021 The Android Open Source Project
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

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "emulator/avd/FakeAvd.h"
#include "emulator/webrtc/StandaloneConnection.h"
#include "android/base/async/AsyncSocketServer.h"

namespace android {
namespace emulation {
namespace control {
class EmulatorAdvertisement;
class EmulatorControllerService;

using android::emulation::control::EmulatorGrpcClient;
using Properties = std::unordered_map<std::string, std::string>;
using emulator::webrtc::StandaloneConnection;
using android::emulation::control::TurnConfig;

using namespace android::base;
// An EmulatorForwarder opens up an EmulatorController service that
// is forwarded to a remote emulator. It will write out a:
//
// - Android studio compatible discovery file
// - A temporary "fake" avd based on the remote emulator
// - Host a gRPC endpoint that gets forwarded.
//
// This will result in android studio recognizing this process as
// an embedded emulator.
//
// TODO(jansene): Fake avds show up in list-avds, but are not launchable.
class EmulatorForwarder {
public:
    EmulatorForwarder(EmulatorGrpcClient* client, int adbPort, TurnConfig turnConfig);
    ~EmulatorForwarder();

    // Establish a remote connection.
    bool createRemoteConnection();

    // Waits until the gRPC servicing is completed.
    void wait();

    // Closes down the connection and cleans up the resources.
    // Note: Do not call this from a gRPC thread, you will deadlock.
    void close();

    StandaloneConnection* webrtcConnection() { return &mWebRtcConnection; }

private:
    void deleteRecursive(const std::string& path);
    bool retrieveRemoteProperties();
    bool createFakeAvd();
    void stopService();

    std::unique_ptr<EmulatorAdvertisement> mAdvertiser;
    std::unique_ptr<EmulatorControllerService> mFwdService;

    EmulatorGrpcClient* mClient;

    StandaloneConnection mWebRtcConnection;
    FakeAvd mAvd;
    std::mutex mCloseMutex;
    bool mAlive{false};
};

}  // namespace control
}  // namespace emulation
}  // namespace android

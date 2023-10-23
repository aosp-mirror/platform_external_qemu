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
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

class VmControlClient {
public:
    explicit VmControlClient(
            std::shared_ptr<EmulatorGrpcClient> client,
            EmulatorController::StubInterface* service = nullptr)
        : mClient(client), mService(service) {
        if (!service) {
            mService = client->stub<EmulatorController>();
        }
    }

    bool start();
    bool stop();
    void reset();
    void shutdown();
    bool pause();
    bool resume();
    bool isRunning();

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<EmulatorController::StubInterface> mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

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
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "avd_service.grpc.pb.h"
#include "avd_service.pb.h"

namespace android {
namespace emulation {
namespace control {

using namespace incubating;
/**
 * @brief A client for interacting with the AVD (Android Virtual Device) service.
 *
 * The AVD service provides information about the Android Virtual Device (AVD)
 * that is currently running in the emulator instance. This class provides methods
 * for retrieving AVD information, both asynchronously and synchronously.
 */
class AvdClient {
public:
    explicit AvdClient(std::shared_ptr<EmulatorGrpcClient> client,
                         AvdService::StubInterface* service = nullptr)
        : mClient(client), mService(service) {
        if (!service) {
            mService = client->stub<AvdService>();
        }
    }

    /**
     * @brief Asynchronously retrieves the AVD information from the emulator.
     *
     * The retrieved AVD information is cached once it has been received.
     *
     * @param onDone The callback to be invoked when the operation completes.
     *               The callback receives an `AvdInfo` object containing the AVD information.
     */
    void getAvdInfoAsync(OnCompleted<AvdInfo> onDone);

    /**
     * @brief Synchronously retrieves the AVD information from the emulator.
     *
     * The retrieved AVD information is cached once it has been received.
     *
     * @return An `absl::StatusOr<AvdInfo>` object containing the AVD information.
     *         If an error occurs, the status will be non-OK and the `AvdInfo` object will be empty.
     */
    absl::StatusOr<AvdInfo> getAvdInfo();

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<AvdService::StubInterface> mService;
    std::atomic_bool mLoaded;
    AvdInfo mCachedInfo;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

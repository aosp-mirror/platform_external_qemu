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

#include "absl/status/statusor.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"

namespace android {
namespace emulation {
namespace control {

using ::google::protobuf::Empty;

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;

/**
 * @brief A client for interacting with the Snapshot Service.
 *
 * The Simple Snapshot Service allows clients to create, load, save, update, and
 * delete snapshots, as well as list all snapshots and load their screenshots.
 * This class provides methods for asynchronously invoking these operations
 * through gRPC.
 *
 * @tparam T The type of object that the OnCompleted callback will receive.
 */
class SimpleEmulatorControlClient {
public:
    explicit SimpleEmulatorControlClient(
            std::shared_ptr<EmulatorGrpcClient> client)
        : mClient(client), mService(client->stub<EmulatorController>()) {}

    /**
     * @brief Asynchronously sets the battery state
     * @param state The battery state to set
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setBattery(BatteryState state, OnCompleted<Empty> onDone);

    /**
     * Maps an `absl::StatusOr<T*>` to an `absl::StatusOr<T>`.
     *
     * @tparam T The type of object to map.
     * @param status_or_ptr The `absl::StatusOr<T*>` to map.
     * @return An `absl::StatusOr<T>` that contains the mapped value, or the
     * original error status.
     *
     * This function uses C++11's type traits to ensure that `T` is a subclass
     * of `google::protobuf::Message`. If `T` is not a subclass of `Message`, a
     * static assertion will fail and the compiler will emit an error message.
     * The function returns an `absl::StatusOr<T>` that contains the mapped
     * value, or the original error status if the input `absl::StatusOr<T*>` is
     * not OK.
     */
    template <typename T, typename R>
    static absl::StatusOr<R> fmap(absl::StatusOr<T> status_or_ptr,
                                  std::function<R(T)> fn) {
        static_assert(std::is_base_of<google::protobuf::Message, T>::value,
                      "T must be a subclass of Message");
        if (!status_or_ptr.ok()) {
            return status_or_ptr.status();
        }
        return absl::StatusOr<R>(fn(status_or_ptr.value()));
    }

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<android::emulation::control::EmulatorController::Stub>
            mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

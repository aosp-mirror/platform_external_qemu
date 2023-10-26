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
#include <memory>
#include <mutex>
#include <string>

#include "absl/status/statusor.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "android/grpc/utils/SimpleAsyncGrpc.h"
#include "emulator_controller.grpc.pb.h"
#include "emulator_controller.pb.h"
#include "google/protobuf/empty.pb.h"

namespace android {
namespace emulation {
namespace control {

/**
 * @brief A client for interacting with the Emulator Controller service.
 *
 * The Emulator Controller service allows clients to control various aspects of
 * an Android emulator instance, such as setting battery state, capturing
 * screenshots, sending input events, and managing the virtual machine state.
 * This class provides methods for asynchronously invoking these operations
 * through gRPC.
 */
class EmulatorControlClient {
public:
    explicit EmulatorControlClient(
            std::shared_ptr<EmulatorGrpcClient> client,
            EmulatorController::StubInterface* service = nullptr)
        : mClient(client), mService(service) {
        if (!service) {
            mService = client->stub<EmulatorController>();
        }
    }

    /**
     * @brief Asynchronously sets the battery state of the emulator.
     *
     * @param state The battery state to set.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setBatteryAsync(BatteryState state, OnCompleted<Empty> onDone = nothing);

    /**
     * @brief Asynchronously gets an individual screenshot in the desired
     * format.
     *
     * @param format The desired image format.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void getScreenshotAsync(ImageFormat format, OnCompleted<Image> onDone);

    /**
     * @brief Asynchronously gets the status of the emulator.
     *
     * @param onDone The callback to be invoked when the operation completes.
     */
    void getStatusAsync(OnCompleted<EmulatorStatus> onDone);

    /**
     * @brief Simulates a touch event on the fingerprint sensor of the emulator.
     *
     * @param finger The fingerprint data to send.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void sendFingerprintAsync(Fingerprint finger,
                         OnCompleted<Empty> onDone = nothing);

    /**
     * @brief Asynchronously gets the display configuration of the emulator.
     *
     * @param onDone The callback to be invoked when the operation completes.
     */
    void getDisplayConfigurationsAsync(OnCompleted<DisplayConfigurations> onDone);

    /**
     * @brief Asynchronously sets the display configuration of the emulator.
     *
     * @param state The display configuration to set.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setDisplayConfigurationsAsync(DisplayConfigurations state,
                                  OnCompleted<DisplayConfigurations> onDone);
    /**
     * @brief Asynchronously sets the clipboard of the emulator
     * @param clip The clipboard data
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setClipboardAsync(std::string clip, OnCompleted<Empty> onDone = nothing);

    /**
     * @brief Asynchronously sets the virtual machine state of the emulator
     * @param state A Run State that describes the state of the Virtual Machine.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setVmStateAsync(VmRunState state, OnCompleted<Empty> onDone = nothing);

    /**
     * @brief Asynchronously sets the virtual machine state of the emulator
     * @param brightness The brightness of the device
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setBrightnessAsync(BrightnessValue brightness,
                       OnCompleted<Empty> onDone = nothing);

    /**
     * @brief Asynchronously sets the gps state of the emulator
     * @param gps The gps state of the device
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setGpsAsync(GpsState gps, OnCompleted<Empty> onDone = nothing);

    absl::StatusOr<GpsState> getGps();

    /**
     * @brief Registers a callback for notification events from the emulator
     * @param incoming Callback that will be invoked for each incoming event
     * @param onDone The callback to be invoked when the operation completes.
     */
    void registerNotificationListener(OnEvent<Notification> incoming,
                                      OnFinished onDone);

    /**
     * @brief Returns a SimpleClientWriter for writing InputEvents to the
     * emulator.
     *
     * There is a single shared inputEvent writer to the emulator.
     */
    std::shared_ptr<SimpleClientWriter<InputEvent>> asyncInputEventWriter();

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::shared_ptr<SimpleClientWriter<InputEvent>> mInputEventWriter;
    std::unique_ptr<EmulatorController::StubInterface> mService;

    std::mutex mInputWriterAccess;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

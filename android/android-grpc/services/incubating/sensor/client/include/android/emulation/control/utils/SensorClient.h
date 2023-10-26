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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "google/protobuf/empty.pb.h"
#include "sensor_service.grpc.pb.h"
#include "sensor_service.pb.h"

namespace android {
namespace emulation {
namespace control {

template <typename T>
using OnCompleted = std::function<void(absl::StatusOr<T*>)>;
using OnFinished = std::function<void(absl::Status)>;

template <typename T>
using OnEvent = std::function<void(const T*)>;
using ::google::protobuf::Empty;
using incubating::PhysicalStateEvent;
using incubating::SensorService;
/**
 * @brief A client for interacting with the Sensor service.
 *
 * The Sensor service provides a way to manage sensor data and the physical
 * model of the emulator. This class provides methods for getting and setting
 * sensor values, getting and setting physical model values, and receiving
 * events for physical model changes and physical state changes.
 */
class SensorClient {
public:
    explicit SensorClient(std::shared_ptr<EmulatorGrpcClient> client,
                          SensorService::StubInterface* service = nullptr)
        : mClient(client), mService(service) {
        if (!service) {
            mService = client->stub<SensorService>();
        }
    }

    /**
     * @brief Gets the value of a specific sensor.
     *
     * @param sensorId The ID of the sensor to get the value for.
     * @return An `absl::StatusOr<incubating::SensorValue>` object containing
     * the sensor value. If an error occurs, the status will be non-OK and the
     * `SensorValue` object will be empty.
     */
    absl::StatusOr<incubating::SensorValue> getSensorValue(int sensorId);

    /**
     * @brief Sets the value of a specific sensor.
     *
     * The sensor value will be updated asynchronously. An immediate call to
     * `getSensorValue` might not reflect the updated value. Setting the value
     * may trigger a sensor event if the value changes.
     *
     * @param value The new value to set for the sensor.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setSensorValueAsync(
            const incubating::SensorValue value,
            OnCompleted<Empty> onDone = [](auto status) {});

    /**
     * @brief Sets the value of a specific physical model parameter.
     *
     * The physical model value will be updated asynchronously. An immediate
     * call to `getPhysicalModel` might not reflect the updated value. Setting
     * the value may trigger a physical model event if the value changes.
     *
     * @param value The new value to set for the physical model parameter.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void setPhysicalModelAsync(
            const incubating::PhysicalModelValue value,
            OnCompleted<Empty> onDone = [](auto status) {});

    /**
     * @brief Gets the value of a specific physical model parameter.
     *
     * @param parameterId The ID of the physical model parameter to get the
     * value for.
     * @param type The type of the physical model parameter.
     * @return An `absl::StatusOr<incubating::PhysicalModelValue>` object
     * containing the physical model value. If an error occurs, the status will
     * be non-OK and the `PhysicalModelValue` object will be empty.
     */
    absl::StatusOr<incubating::PhysicalModelValue> getPhysicalModel(
            int parameterId,
            int type);

    /**
     * @brief Receives events for changes to a specific physical model
     * parameter.
     *
     * @param value The initial value of the physical model parameter to
     * subscribe to.
     * @param incoming The callback to be invoked for each incoming physical
     * model event.
     * @param onDone The callback to be invoked when the event stream has ended.
     */
    void receivePhysicalModelEvents(
            incubating::PhysicalModelValue value,
            OnEvent<incubating::PhysicalModelValue> incoming,
            OnFinished onDone);

    /**
     * @brief Receives events for changes to the physical state.
     *
     * @param incoming The callback to be invoked for each incoming physical
     * state event.
     * @param onDone The callback to be invoked when the event stream has ended.
     */
    void receivePhysicalStateEvents(OnEvent<PhysicalStateEvent> incoming,
                                    OnFinished onDone);

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<SensorService::StubInterface> mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

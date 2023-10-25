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

#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/emulation/control/utils/GenericCallbackFunctions.h"
#include "car_service.grpc.pb.h"
#include "car_service.pb.h"

namespace android {
namespace emulation {
namespace control {

using namespace incubating;
/**
 * @brief A client for interacting with the Car service.
 *
 * The Car service provides a way to send and receive car events between the
 * emulator and the host system. This class provides methods for sending car
 * events to the emulator and receiving car events from the emulator.
 */
class CarClient {
public:
    explicit CarClient(std::shared_ptr<EmulatorGrpcClient> client,
                       CarService::StubInterface* service = nullptr)
        : mClient(client), mService(service) {
        if (!service) {
            mService = client->stub<CarService>();
        }
    }

    /**
     * @brief Asynchronously sends a car event to the emulator.
     *
     * This method makes a best effort to deliver the car event to the emulator.
     *
     * @param event The car event to send.
     */
    void sendCarEventAsync(incubating::CarEvent event);

    /**
     * @brief Receives car events from the emulator.
     *
     * This method registers a callback to receive car events from the emulator.
     * The callback will be invoked for each incoming car event until the
     * `onDone` callback is invoked, indicating that the event stream has ended.
     *
     * @param incoming The callback to be invoked for each incoming car event.
     * @param onDone The callback to be invoked when the event stream has ended.
     */
    void receiveCarEvents(OnEvent<CarEvent> incoming, OnFinished onDone);

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<CarService::StubInterface> mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

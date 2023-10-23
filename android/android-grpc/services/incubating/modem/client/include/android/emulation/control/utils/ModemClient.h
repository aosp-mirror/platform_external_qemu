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
#include "google/protobuf/empty.pb.h"
#include "modem_service.grpc.pb.h"
#include "modem_service.pb.h"

namespace android {
namespace emulation {
namespace control {

using namespace incubating;
/**
 * @brief A client for interacting with the Modem service.
 *
 * The Modem service provides a way to manage the modem state of the emulator,
 * including cell info, calls, SMS messages, and modem events. This class provides
 * methods for setting cell info, creating and managing calls, sending SMS messages,
 * updating the modem clock, and receiving modem events.
 */
class ModemClient {
public:
    explicit ModemClient(std::shared_ptr<EmulatorGrpcClient> client,
                         Modem::StubInterface* service = nullptr)
        : mClient(client), mService(service) {
        if (!service) {
            mService = client->stub<Modem>();
        }
    }

    /**
     * @brief Sets the current cell info and returns the updated cell info state.
     *
     * @param info The cell info to set.
     * @param onDone The callback to be invoked when the operation completes.
     *               The callback receives an updated `CellInfo` object.
     */
    void setCellInfoAsync(CellInfo info, OnCompleted<CellInfo> onDone);

    /**
     * @brief Gets the current cell info.
     *
     * @note This method is not fully implemented yet.
     *
     * @param onDone The callback to be invoked when the operation completes.
     *               The callback receives the current `CellInfo` object.
     */
    void getCellInfoAsync(OnCompleted<CellInfo> onDone);

    /**
     * @brief Creates a new call object, resulting in an incoming call to the emulator.
     *
     * @param call The call object to create.
     * @param onDone The callback to be invoked when the operation completes.
     *               The callback receives the created `Call` object.
     */
    void createCallAsync(Call call, OnCompleted<Call> onDone);

    /**
     * @brief Updates the state of an existing call.
     *
     * The provided call object must have a valid call ID that corresponds to an existing call.
     *
     * @param call The call object with updated state.
     * @param onDone The callback to be invoked when the operation completes.
     *               The callback receives the updated `Call` object.
     */
    void updateCallAsync(Call call, OnCompleted<Call> onDone);

    /**
     * @brief Removes an active call, disconnecting the call.
     *
     * The provided call object must have a valid call ID that corresponds to an existing call.
     *
     * @param call The call object to remove.
     * @param onDone The callback to be invoked when the operation completes.
     */
    void deleteCallAsync(Call call, OnCompleted<Empty> onDone = nothing);

    /**
     * @brief Sends an SMS message to the emulator.
     *
     * The message can be either a plain text message or an encoded message.
     *
     * @param message The SMS message to send.
     * @param onDone The callback to be invoked when the operation completes.
     *
     * @throws INVALID_ARGUMENT if the message is invalid.
     */
    void receiveSmsAsync(incubating::SmsMessage message,
                         OnCompleted<::google::protobuf::Empty> onDone);

    /**
     * @brief Updates the modem clock.
     *
     * @note This method is likely to be deprecated in the future.
     *
     * @param onDone The callback to be invoked when the operation completes.
     */
    void updateTimeAsync(OnCompleted<::google::protobuf::Empty> onDone = nothing);

    /**
     * @brief Receives modem events as they arrive.
     *
     * @param incoming The callback to be invoked for each incoming modem event.
     * @param onDone The callback to be invoked when the event stream has ended.
     */
    void receivePhoneEvents(OnEvent<PhoneEvent> incoming, OnFinished onDone);

private:
    std::shared_ptr<EmulatorGrpcClient> mClient;
    std::unique_ptr<Modem::StubInterface> mService;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

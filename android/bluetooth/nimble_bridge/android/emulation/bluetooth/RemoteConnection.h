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
#pragma once

#include <grpcpp/grpcpp.h>     // for ClientContext
#include <condition_variable>  // for condition
#include <cstdint>             // for uint16_t
#include <memory>              // for unique_ptr
#include <mutex>               // for mutex
#include <string>              // for string
#include <unordered_map>       // for unordered_map

#include "aemu/base/Log.h"
#include "absl/status/status.h"                            // for Status
#include "absl/status/statusor.h"                          // for StatusOr
#include "android/emulation/bluetooth/RequestForwarder.h"  // for EmuGattCha...
#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for Emul...
#include "emulated_bluetooth_device.grpc.pb.h"  // for GattDevice...
#include "emulated_bluetooth_device.pb.h"       // for DeviceIden...

namespace android {
namespace emulation {
namespace bluetooth {

using android::emulation::control::EmulatorGrpcClient;

class RemoteConnection;

// A handle to a value/connection in nimble.
using BleHandle = uint16_t;

// A Subscription handles the notification stream between the nimble bluetooth
// stack and the gRPC service implementation.
class Subscription
    : public grpc::ClientReadReactor<CharacteristicValueResponse> {
public:
    Subscription(RemoteConnection* connection,
                 CharacteristicValueRequest&& cvr,
                 BleHandle conn_handle,
                 BleHandle val_handle);
    ~Subscription() = default;

    void OnReadDone(bool ok) override;
    void OnDone(const grpc::Status& s) override;
    void cancel() { mContext->TryCancel(); }

private:
    RemoteConnection* mConnection;
    std::shared_ptr<grpc::ClientContext> mContext;
    CharacteristicValueResponse mIncoming;
    CharacteristicValueRequest mRequest;

    const BleHandle mValHandle;
    const BleHandle mConnectionHandle;
};

// A remote connection handles forwarding of bluetooth calls to the gRPC service
// that is providing all the values.
class RemoteConnection {
public:
    RemoteConnection(EmulatorGrpcClient* client,
                     CallbackIdentifier myId,
                     BleHandle conn_handle);
    ~RemoteConnection();

    // Subscribe to receive notifications for the given characteristic.
    absl::Status subscribe(const EmuGattCharacteristic* ch);

    // Unsubscribe to stop receiving notifications for the given characteristic.
    absl::Status unsubscribe(const EmuGattCharacteristic* ch);

    // Read the given characteristic. Returns the raw data if any.
    absl::StatusOr<std::string> read(const EmuGattCharacteristic* ch);

    // Write the given characteristic. Returning the raw data if any.
    absl::StatusOr<std::string> write(const EmuGattCharacteristic* ch,
                                      const std::string value);

    // The client connection to the gRPC service.
    EmulatorGrpcClient* client() { return mClient; }

    // The active service stub.
    GattDeviceService::Stub* stub() { return mStub.get(); }

private:
    // Notify the gRPC service of the connection state change.
    void notifyConnectionState(ConnectionStateChange::ConnectionState newState,
                               BleHandle conn_handle);

    // Erase and clean up the subscription, called by the subscription class
    // when the connection has been finalized.
    void eraseSubscription(BleHandle val_handle);

    EmulatorGrpcClient* mClient;
    BleHandle mConnectionHandle;
    CallbackIdentifier mMe;
    DeviceIdentifier mRemoteService;
    std::unique_ptr<GattDeviceService::Stub> mStub;

    // Active subscriptions.
    std::unordered_map<BleHandle, std::unique_ptr<Subscription>>
            mSubscriptions{};
    std::mutex mSubcriptionMutex;
    std::condition_variable mSubscriptionCv;

    friend class Subscription;
};

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android

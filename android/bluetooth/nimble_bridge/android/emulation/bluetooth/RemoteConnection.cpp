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
#include "android/emulation/bluetooth/RemoteConnection.h"

#include <assert.h>         // for assert
#include <grpcpp/grpcpp.h>  // for Status
#include <stdint.h>         // for uint...
#include <utility>          // for move

#include "aemu/base/Log.h"
#include "absl/strings/string_view.h"                            // for stri...
#include "android/emulation/control/utils/EmulatorGrcpClient.h"  // for Emul...
#include "android/emulation/bluetooth/nimble/TypeTranslator.h"   // for look...
#include "android/utils/debug.h"                                 // for derror
#include "emulated_bluetooth_device.grpc.pb.h"                   // for Gatt...
#include "emulated_bluetooth_device.pb.h"                        // for Char...
#include "google/protobuf/empty.pb.h"                            // for Empty
#include "host/ble_gatt.h"                                       // for ble_...
#include "host/ble_hs_mbuf.h"                                    // for ble_...

struct os_mbuf;

#undef min
#undef max

namespace android {
namespace emulation {
namespace bluetooth {

Subscription::Subscription(RemoteConnection* connection,
                           CharacteristicValueRequest&& cvr,
                           BleHandle conn_handle,
                           BleHandle val_handle)
    : mConnection(connection),
      mConnectionHandle(conn_handle),
      mValHandle(val_handle),
      mRequest(cvr) {
    mContext = mConnection->client()->newContext();
    mConnection->stub()->async()->OnCharacteristicObserveRequest(
            mContext.get(), &mRequest, this);
    StartRead(&mIncoming);
    StartCall();
}

void Subscription::OnReadDone(bool ok) {
    if (ok) {
        os_mbuf* om = ble_hs_mbuf_from_flat(mIncoming.data().data(),
                                            mIncoming.data().size());
        int rc = ble_gattc_notify_custom(mConnectionHandle, mValHandle, om);
        if (rc != 0) {
            derror("Error during notification: %d, cancelling", rc);
            cancel();
        } else {
            StartRead(&mIncoming);
        }
    }
}

void Subscription::OnDone(const grpc::Status& s) {
    mConnection->eraseSubscription(mValHandle);
}

RemoteConnection::RemoteConnection(EmulatorGrpcClient* client,
                                   CallbackIdentifier myId,
                                   BleHandle conn_handle)
    : mClient(client), mConnectionHandle(conn_handle) {
    mMe.CopyFrom(myId);
    mRemoteService.CopyFrom(lookupDeviceIdentifier(conn_handle));
    assert(mClient->hasOpenChannel());
    mStub = mClient->stub<GattDeviceService>();
    notifyConnectionState(ConnectionStateChange::CONNECTION_STATE_CONNECTED,
                          mConnectionHandle);
}

RemoteConnection::~RemoteConnection() {
    notifyConnectionState(ConnectionStateChange::CONNECTION_STATE_DISCONNECTED,
                          mConnectionHandle);
    std::unique_lock<std::mutex> guard(mSubcriptionMutex);
    for (auto& [idx, sub] : mSubscriptions) {
        sub->cancel();
    }

    mSubscriptionCv.wait(guard, [&] { return mSubscriptions.empty(); });
}

absl::Status RemoteConnection::subscribe(const EmuGattCharacteristic* ch) {
    std::lock_guard<std::mutex> guard(mSubcriptionMutex);
    auto val_handle = ch->getValueHandle();
    if (mSubscriptions.count(val_handle)) {
        return absl::Status(absl::StatusCode::kAlreadyExists,
                            "This connections is already subscribed to this "
                            "characteristc.");
    }
    CharacteristicValueRequest readRequest;
    *readRequest.mutable_callback_device_id() = mMe;
    *readRequest.mutable_from_device() = mRemoteService;
    *readRequest.mutable_callback_id() = ch->callbackId();

    mSubscriptions[val_handle] = std::make_unique<Subscription>(
            this, std::move(readRequest), mConnectionHandle, val_handle);

    return absl::OkStatus();
}

absl::Status RemoteConnection::unsubscribe(const EmuGattCharacteristic* ch) {
    std::lock_guard<std::mutex> guard(mSubcriptionMutex);
    auto val_handle = ch->getValueHandle();

    if (!mSubscriptions.count(val_handle)) {
        return absl::Status(absl::StatusCode::kNotFound,
                            "This connections is not subscribed to this "
                            "characteristc.");
    }

    mSubscriptions[val_handle]->cancel();
    return absl::OkStatus();
}

void RemoteConnection::eraseSubscription(BleHandle val_handle) {
    std::lock_guard<std::mutex> guard(mSubcriptionMutex);
    assert(mSubscriptions.count(val_handle));
    mSubscriptions.erase(val_handle);
    mSubscriptionCv.notify_all();
}

void RemoteConnection::notifyConnectionState(
        ConnectionStateChange::ConnectionState newState,
        BleHandle conn_handle) {
    ::google::protobuf::Empty reply;
    ConnectionStateChange csc;
    csc.set_new_state(newState);
    *csc.mutable_callback_device_id() = mMe;
    *csc.mutable_from_device() = mRemoteService;
    auto ctx = mClient->newContext();
    auto grpcStatus = mStub->OnConnectionStateChange(ctx.get(), csc, &reply);

    if (!grpcStatus.ok()) {
        // Failure!
        derror("Failed to report %s due to %s", csc.ShortDebugString(),
               grpcStatus.error_message());
    }
}

absl::StatusOr<std::string> RemoteConnection::read(
        const EmuGattCharacteristic* ch) {
    CharacteristicValueResponse response;
    CharacteristicValueRequest readRequest;
    *readRequest.mutable_callback_device_id() = mMe;
    *readRequest.mutable_from_device() = mRemoteService;
    *readRequest.mutable_callback_id() = ch->callbackId();
    auto ctx = mClient->newContext();
    auto grpcStatus = mStub->OnCharacteristicReadRequest(ctx.get(), readRequest,
                                                         &response);
    if (!grpcStatus.ok()) {
        derror("Failed to read %s due to %s",
               ch->callbackId().ShortDebugString().c_str(),
               grpcStatus.error_message().c_str());
        return absl::Status((absl::StatusCode)grpcStatus.error_code(),
                            grpcStatus.error_message());
    }

    // This will be empty in case of failure..
    return response.data();
}

absl::StatusOr<std::string> RemoteConnection::write(
        const EmuGattCharacteristic* ch,
        const std::string value) {
    CharacteristicValueResponse response;
    CharacteristicValueRequest writeRequest;
    writeRequest.set_data(value);
    *writeRequest.mutable_callback_device_id() = mMe;
    *writeRequest.mutable_from_device() = mRemoteService;
    *writeRequest.mutable_callback_id() = ch->callbackId();
    auto ctx = mClient->newContext();
    auto grpcStatus = mStub->OnCharacteristicWriteRequest(
            ctx.get(), writeRequest, &response);

    if (!grpcStatus.ok()) {
        derror("Failed to write %s due to %s",
               ch->callbackId().ShortDebugString().c_str(),
               grpcStatus.error_message().c_str());
        return absl::Status((absl::StatusCode)grpcStatus.error_code(),
                            grpcStatus.error_message());
    }

    // This will be empty in case of failure..
    return response.data();
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
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

#include <stdint.h>
#include <string>
#include <vector>

#include "aemu/base/Log.h"
#include "absl/status/status.h"
#include "android/emulation/bluetooth/nimble/ServiceDefinition.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"

// Nibmle introduces these, causing trouble for almost everyone.
#undef min
#undef max

struct ble_gap_event;

namespace android {
namespace emulation {
namespace remote {
class Endpoint;
}  // namespace remote

namespace bluetooth {

using ::android::emulation::control::EmulatorGrpcClient;
using ::android::emulation::remote::Endpoint;

class RemoteConnection;

class RequestForwarder {
public:
    ~RequestForwarder() = default;
    RequestForwarder(const GattDevice& device);

    void start();
    void stop();
    void onSync();

    absl::Status initialize();

    static RequestForwarder* registerDevice(
            GattDevice device,
            std::unique_ptr<EmulatorGrpcClient> hciTransport);

private:
    friend class EmuGattCharacteristic;
    int handleConnect(ble_gap_event* event);
    int handleDisconnect(ble_gap_event* event);
    int handleSubscription(ble_gap_event* event);

    int handleGapEvent(ble_gap_event* event);
    int handleGattAccess(uint16_t connectionHandle,
                         uint16_t attributeHandle,
                         struct ble_gatt_access_ctxt* ctxt);

    void advertise();

    // Nimble callbacks..
    static int ble_gap_callback(struct ble_gap_event* event, void* arg);
    static int ble_gatt_callback(uint16_t connectionHandle,
                                 uint16_t attributeHandle,
                                 struct ble_gatt_access_ctxt* ctxt,
                                 void* arg);
    EmuGattProfile mProfile;
    bool mRunning{true};
    uint8_t mAddrType{0};
    GattDevice mDevice;
    std::unique_ptr<EmulatorGrpcClient> mClient;
    CallbackIdentifier mMyId;
    std::unordered_map<uint16_t, std::unique_ptr<RemoteConnection>>
            mConnections;
};

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android

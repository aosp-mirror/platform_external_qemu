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

#include <cstdint>                         // for uint16_t
#include <memory>                          // for unique_ptr
#include <vector>                          // for vector

#include "aemu/base/Compiler.h"
#include "emulated_bluetooth_device.pb.h"  // for Uuid, GattCharacteristic (...
#include "host/ble_gatt.h"                 // for ble_gatt_chr_def, ble_gatt...
#include "host/ble_uuid.h"                 // for ble_uuid_any_t

// Nibmle introduces these, causing trouble for almost everyone.
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace android {
namespace emulation {
namespace bluetooth {


// Represents a GattCharacteristic, that maps from the protobuf definition to
// the internal nimble representation.
class EmuGattCharacteristic {
public:
    EmuGattCharacteristic(const GattCharacteristic& ch, void* callbackArg);
    ~EmuGattCharacteristic() = default;
    DISALLOW_COPY_ASSIGN_AND_MOVE(EmuGattCharacteristic);

    const ble_gatt_chr_def getNimbleGattChrDef() const { return mGattDef; }
    const uint16_t getValueHandle() const { return mValHandle; }
    Uuid callbackId() const { return mCallbackId; }

private:
    ble_gatt_chr_def mGattDef{0};
    ble_uuid_any_t mUuid{0};
    uint16_t mValHandle{0};
    Uuid mCallbackId;

    static uint16_t sUniqueValueHandle;
};

// Represents a GattService, that maps from the protobuf definition to the
// internal nimble representation.
class EmuGattService {
public:
    EmuGattService(const GattService& service, void* callbackArg);
   ~EmuGattService() = default;
    DISALLOW_COPY_ASSIGN_AND_MOVE(EmuGattService);

    const ble_gatt_svc_def getNimbleServiceDef() { return mServiceDef; }
    EmuGattCharacteristic* findCharacteristicByHandle(uint16_t val);

private:
    std::vector<std::unique_ptr<EmuGattCharacteristic>> mCharacteristics;
    std::vector<ble_gatt_chr_def> mNimbleCharacteristics;
    ble_gatt_svc_def mServiceDef{0};
    ble_uuid_any_t mUuid{0};
};

// Represents a GattProfile, that maps from the protobuf definition to the
// internal nimble representation.
class EmuGattProfile {
public:
    EmuGattProfile(const GattProfile& profile, void* callbackArg);
    ~EmuGattProfile() = default;
    DISALLOW_COPY_ASSIGN_AND_MOVE(EmuGattProfile);

    const ble_gatt_svc_def* getNimbleServiceDef() const {
        return mNimbleServices.data();
    }
    EmuGattCharacteristic* findCharacteristicByHandle(uint16_t val);

private:
    std::vector<std::unique_ptr<EmuGattService>> mServices;
    std::vector<ble_gatt_svc_def> mNimbleServices;
};
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
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
#include "android/emulation/bluetooth/nimble/ServiceDefinition.h"

#include <utility>                                              // for move
#include "android/emulation/bluetooth/RequestForwarder.h"       // for EmuGa...
#include "android/emulation/bluetooth/nimble/TypeTranslator.h"  // for toNim...

namespace android {
namespace emulation {
namespace bluetooth {

const ble_gatt_chr_def kEMPTY_CHR = {0};
const ble_gatt_dsc_def kEMPTY_DSC = {0};
const ble_gatt_svc_def kEMPTY_SVC = {0};
uint16_t EmuGattCharacteristic::sUniqueValueHandle = 1;

EmuGattCharacteristic::EmuGattCharacteristic(const GattCharacteristic& ch,
                                             void* device)
    : mUuid(toNimbleUuid(ch.uuid())),
      mValHandle(sUniqueValueHandle++),
      mGattDef({.uuid = (ble_uuid_t*)&mUuid,
                .access_cb = &RequestForwarder::ble_gatt_callback,
                .arg = device,
                .descriptors = nullptr,
                .flags = static_cast<uint16_t>(ch.properties()),
                .val_handle = &mValHandle}) {
    mCallbackId.CopyFrom(ch.has_callback_id() ? ch.callback_id() : ch.uuid());
}

EmuGattService::EmuGattService(const GattService& service, void* device)
    : mUuid(toNimbleUuid(service.uuid())) {
    for (const auto& ch : service.characteristics()) {
        auto localChar = std::make_unique<EmuGattCharacteristic>(ch, device);
        mNimbleCharacteristics.push_back(localChar->getNimbleGattChrDef());
        mCharacteristics.push_back(std::move(localChar));
    };

    // Add the empty on to indicate no more characteristics in this service.
    mNimbleCharacteristics.push_back(kEMPTY_CHR);
    mServiceDef = {
            .type = toNimbleServiceType(service.service_type()),
            .uuid = (ble_uuid_t*)&mUuid,
            .characteristics = mNimbleCharacteristics.data(),
    };
}

EmuGattCharacteristic* EmuGattService::findCharacteristicByHandle(
        uint16_t val) {
    for (const auto& chr : mCharacteristics) {
        if (chr->getValueHandle() == val) {
            return chr.get();
        }
    }

    return nullptr;
}

EmuGattProfile::EmuGattProfile(const GattProfile& profile, void* device) {
    for (const GattService& service : profile.services()) {
        auto localService = std::make_unique<EmuGattService>(service, device);
        mNimbleServices.push_back(localService->getNimbleServiceDef());
        mServices.push_back(std::move(localService));
    };

    // Add the empty on to indicate no more services in this profile
    mNimbleServices.push_back(kEMPTY_SVC);
}

EmuGattCharacteristic* EmuGattProfile::findCharacteristicByHandle(
        uint16_t val) {
    for (const auto& service : mServices) {
        auto chr = service->findCharacteristicByHandle(val);
        if (chr != nullptr) {
            return chr;
        }
    }

    return nullptr;
}
}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
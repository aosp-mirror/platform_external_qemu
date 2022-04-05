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
#include <cstdint>                         // for uint16_t, uint8_t
#include <string>                          // for string

#include "host/ble_uuid.h"                 // for ble_uuid_any_t

// nibmle...
#undef min
#undef max

#include "emulated_bluetooth_device.pb.h"  // for Advertisement, Advertiseme...

namespace android {
namespace emulation {
namespace bluetooth {

DeviceIdentifier lookupDeviceIdentifier(uint16_t conn_handle);
std::string collectOmChain(struct os_mbuf* om);
std::string bleErrToString(int rc);

ble_uuid_any_t toNimbleUuid(Uuid uuid);
uint8_t toNimbleConnectionMode(const Advertisement::ConnectionMode mode);
uint8_t toNimbleDiscoveryMode(const Advertisement::DiscoveryMode mode);
uint8_t toNimbleServiceType(GattService::ServiceType type);



}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
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
#include "emulated_bluetooth.grpc.pb.h"  // for EmulatedBluetoothService

namespace android {
namespace base {
class Looper;
}  // namespace base
namespace bluetooth {
class Rootcanal;
}  // namespace bluetooth

namespace emulation {
namespace bluetooth {

using AsyncEmulatedBluetoothService = EmulatedBluetoothService::CallbackService;

// Initializes the EmulatedBluetooth gRPC service endpoint,
// backed by rootcanal and provided looper.
EmulatedBluetoothService::Service* getEmulatedBluetoothService(
        android::bluetooth::Rootcanal* rootcanal,
        base::Looper* looper);

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
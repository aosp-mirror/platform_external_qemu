// Copyright 2024 The Android Open Source Project
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
#pragma onces

#include "PacketProtocol.h"

namespace android {
namespace qemu2 {

// Factory that can produce a packet serializer for the given device name.
std::unique_ptr<PacketProtocol> getUwbPacketProtocol(
        std::string deviceType,
        std::shared_ptr<DeviceInfo> device_info);
}  // namespace qemu2
}  // namespace android

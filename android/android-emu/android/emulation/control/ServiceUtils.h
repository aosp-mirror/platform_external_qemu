// Copyright (C) 2019 The Android Open Source Project
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
#include <chrono>         // for seconds, milliseconds
#include <string>         // for string
#include <unordered_map>  // for unordered_map

namespace android {
namespace emulation {
namespace control {

std::unordered_map<std::string, std::string> getQemuConfig();

std::unordered_map<std::string, std::string> getUserConfig();
// Returns if the emulator has booted at least once.
//
// 1. For API >= 28 we rely on a boot services that informs qemu that boot has
// completed. This call will never block and wait.
// 2. For API < 28, we schedule an ADB command, and wait at most maxWaitTime for
// the result. The global boot state will be updated even if maxWaitTime has
// passed.
bool bootCompleted(
        std::chrono::milliseconds maxWaitTime = std::chrono::seconds(2));

}  // namespace control
}  // namespace emulation
}  // namespace android

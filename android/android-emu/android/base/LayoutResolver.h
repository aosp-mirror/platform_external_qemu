// Copyright 2019 The Android Open Source Project
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
#include <unordered_map>
#include <utility>

namespace android {
namespace base {
// rect: a mapping from display ID to a rectangle represented as pair of width
// and height. 
// monitorAspectRatio: current host monitor's aspect ratio.
// This function returns an "optimized" layout represented as a mapping from display
// ID to x-y coordinates w.r.t the lower left corner of the QT window.
std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> resolveLayout(
        std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> rect,
        const double monitorAspectRatio);

}  // namespace base
}  // namespace android

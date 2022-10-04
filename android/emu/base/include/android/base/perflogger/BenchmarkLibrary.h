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

#include <string_view>

namespace android {
namespace perflogger {

// OpenGL benchmarks
void logDrawCallOverheadTest(
        std::string_view glVendor,
        std::string_view glRenderer,
        std::string_view glVersion,
        // |variant|: drawArrays, drawElements, drawSwitchVao, drawSwitchVap
        std::string_view variant,
        // |indirectionLevel|: fullStack, decoder, or host
        std::string_view indirectionLevel,
        int count,
        long rateHz,
        long wallTime,
        long threadCpuTimeUs);

} // namespace perflogger
} // namespace android

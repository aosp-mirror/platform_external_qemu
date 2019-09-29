// Copyright (C) 2018 The Android Open Source Project
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

#include <stdint.h>                   // for uint32_t, uint8_t
#include <functional>                 // for function
#include <memory>                     // for unique_ptr

#include "android/recording/Frame.h"  // for VideoFormat

namespace android {
namespace recording {
class Producer;

// Creates a dummy video producer for testing.
std::unique_ptr<Producer> createDummyVideoProducer(
        uint32_t fbWidth,
        uint32_t fbHeight,
        uint8_t fps,
        uint8_t durationSecs,
        VideoFormat fmt,
        std::function<void()> onFinishedCb);

}  // namespace recording
}  // namespace android

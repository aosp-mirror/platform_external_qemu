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

#include "android/emulation/control/display_agent.h"
#include "android/recording/Producer.h"

namespace android {
namespace recording {

// Create a new VideoProducer instance. |cb| is a function that
// will be called from this thread with new video frames.
std::unique_ptr<Producer> createVideoProducer(
        uint32_t fbWidth,
        uint32_t fbHeight,
        uint8_t fps,
        const QAndroidDisplayAgent* dpyAgent = nullptr);

}  // namespace recording
}  // namespace android

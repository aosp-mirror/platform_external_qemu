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

// Create a new WebRTCProducer instance, it will start
// writing frames at the given fps on the given file handle.
std::unique_ptr<Producer> createWebRTCProducer(
        uint32_t fbWidth,
        uint32_t fbHeight,
        uint8_t fps,
        const std::string& handle);

static std::unique_ptr<Producer>  s_gWebrtcProducer;

void start_webrtc_module(std::string handle);

void stop_webrtc_module();
}  // namespace recording
}  // namespace android


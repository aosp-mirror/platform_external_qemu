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

#include "android/automation/AutomationController.h"
#include "android/emulation/AndroidAsyncMessagePipe.h"
#include "android/offworld/proto/offworld.pb.h"
#include "android/videoinjection/VideoInjectionController.h"

namespace android {
namespace offworld {

namespace pb = ::offworld;

static constexpr uint32_t kProtocolVersion = 1;

// Called during emulator initialization, do not call manually.
void registerOffworldPipeService();

// Register the offworld pipe service for test, does not check
// featurecontrol.
void registerOffworldPipeServiceForTest(
        automation::AutomationController* automationController,
        videoinjection::VideoInjectionController* videoInjectionController);

// Send a response to an Offworld pipe.
bool sendResponse(android::AsyncMessagePipeHandle pipe,
                  const pb::Response& response);

}  // namespace offworld
}  // namespace android

// Copyright (C) 2020 The Android Open Source Project
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

#include <atomic>                                         // for atomic

#include "android/base/EventNotificationSupport.h"        // for EventNotifi...
#include "android/emulation/control/utils/EventWaiter.h"  // for EventWaiter

namespace android {
namespace emulation {
namespace control {

// The Camera will register a callback with the camera qemud service and
// will get notified when camera is connected or disconnected.
class Camera : public base::EventNotificationSupport<Camera, bool> {
public:
    Camera();
    bool isVirtualSceneConnected() const { return mVirtualSceneConnected; };

    // Gets the event waiter that can be used to wait for new
    // camera updates.
    EventWaiter* eventWaiter();

    // Gets the Camera singleton that will register with the given agent.
    static Camera* getCamera();

private:
    static void virtualSceneCameraCallback(void* context, bool connected);
    std::atomic<bool> mVirtualSceneConnected;
    EventWaiter mEventWaiter;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

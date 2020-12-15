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

#include <atomic>

#include "android/emulation/control/utils/EventWaiter.h"  // for EventWaiter

namespace android {
namespace emulation {
namespace control {

// The Camera will register a callback with the camera qemud service and
// will get notified when camera is connected or disconnected.
class Camera {
public:
    bool isConnected() const { return mConnected.load(); };

    // Gets the event waiter that can be used to wait for new
    // camera updates.
    EventWaiter* eventWaiter();

    // Gets the Camera singleton that will register with the given agent.
    static Camera* getCamera();

private:
    Camera();
    static void guestCameraCallback(void* context, bool connected);
    std::atomic<bool> mConnected;
    EventWaiter mEventWaiter;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

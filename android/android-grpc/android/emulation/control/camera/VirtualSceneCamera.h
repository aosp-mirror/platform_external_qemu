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

#include <atomic>  // for atomic
#include <functional>

#include "aemu/base/Compiler.h"
#include "aemu/base/EventNotificationSupport.h"        // for EventNotifi...
#include "android/emulation/control/utils/EventWaiter.h"  // for EventWaiter
#include "android/virtualscene/WASDInputHandler.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace android {
namespace emulation {
namespace control {

// The VirtualSceneCamera will register a callback with the camera qemud service
// and will get notified when camera is connected or disconnected.
class VirtualSceneCamera : public base::EventNotificationSupport<bool> {
public:
    using Callback = std::function<void(bool)>;

    VirtualSceneCamera(const QAndroidSensorsAgent* sensorsAgent,
                       Callback cb = nullptr);
    bool isConnected() const { return mConnected; };
    // delta in radian
    bool rotate(const glm::vec3 delta);
    bool setVelocity(const glm::vec3 absolute);
    // Gets the event waiter that can be used to wait for new
    // VirtualSceneCamera updates.
    EventWaiter* eventWaiter();

private:
    DISALLOW_COPY_AND_ASSIGN(VirtualSceneCamera);
    static void virtualSceneCameraCallback(void* context, bool connected);
    std::atomic<bool> mConnected;
    EventWaiter mEventWaiter;
    Callback mCallback;

    android::virtualscene::PhysicalModel mModel;
    // Track base velocity because it is possible
    // to have different rotation but same base velocity and in that case,
    // sensor's velocity still needs to be updated.
    glm::vec3 mBaseVelocity = glm::vec3();
};

}  // namespace control
}  // namespace emulation
}  // namespace android

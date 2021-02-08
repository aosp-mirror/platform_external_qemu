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

#include "android/base/Compiler.h"
#include "android/base/EventNotificationSupport.h"  // for EventNotifi...
#include "android/emulation/control/sensors_agent.h"
#include "android/emulation/control/utils/EventWaiter.h"  // for EventWaiter

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace android {
namespace emulation {
namespace control {

// The VirtualSceneCamera will register a callback with the camera qemud service
// and will get notified when camera is connected or disconnected.
class VirtualSceneCamera
    : public base::EventNotificationSupport<VirtualSceneCamera, bool> {
public:
    using Callback = std::function<void(bool)>;

    VirtualSceneCamera(const QAndroidSensorsAgent* sensorsAgent,
                       Callback cb = nullptr);
    bool isConnected() const { return mConnected; };
    bool rotate(const glm::vec3 delta);
    bool updateVelocity(const glm::vec3 absolute);
    // Gets the event waiter that can be used to wait for new
    // VirtualSceneCamera updates.
    EventWaiter* eventWaiter();

private:
    DISALLOW_COPY_AND_ASSIGN(VirtualSceneCamera);
    static void virtualSceneCameraCallback(void* context, bool connected);
    std::atomic<bool> mConnected;
    EventWaiter mEventWaiter;
    Callback mCallback;
    const QAndroidSensorsAgent* mSensorsAgent;
    glm::vec3 mVelocity = glm::vec3();
    // x axis is horizontal and orthogonal to the view direction.
    // y axis points up and is perpendicular to the floor.
    // z axis is the view direction
    glm::vec3 mEulerRotationDegrees = glm::vec3();  // EulerAnglesYXZ
};

}  // namespace control
}  // namespace emulation
}  // namespace android

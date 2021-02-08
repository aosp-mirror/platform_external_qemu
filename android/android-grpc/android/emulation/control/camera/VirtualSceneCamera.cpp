
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
#include "android/emulation/control/camera/VirtualSceneCamera.h"

#include "android/base/memory/LazyInstance.h"  // for LazyInstance
#include "android/camera/camera-common.h"      // for kVirtualScene
#include "android/camera/camera-service.h"     // for register_ca...
#include "android/physics/GlmHelpers.h"
#include "android/physics/PhysicalModel.h"
#include "android/virtualscene/WASDInputHandler.h"

using android::virtualscene::kMaxVerticalRotationDegrees;
using android::virtualscene::kMinVerticalRotationDegrees;
using android::virtualscene::kPixelsToRotationRadians;

namespace android {
namespace emulation {
namespace control {

static const float kAmbientMotionExtentMeters = 0.005f;

// mVirtualSceneConnected will be updated in three scenarios:
// 1, snapshot load 2, virtual scene camera connected 3, virtual scene camera
// disconnected.
VirtualSceneCamera::VirtualSceneCamera(const QAndroidSensorsAgent* sensorsAgent,
                                       Callback cb)
    : mSensorsAgent(sensorsAgent), mConnected(false), mCallback(std::move(cb)) {
    register_camera_status_change_callback(
            &VirtualSceneCamera::virtualSceneCameraCallback, this,
            kVirtualScene);
}

void VirtualSceneCamera::virtualSceneCameraCallback(void* context,
                                                    bool connected) {
    auto self = static_cast<VirtualSceneCamera*>(context);
    self->mConnected = connected;
    if (self->mCallback != nullptr)
        self->mCallback(connected);
    self->mEventWaiter.newEvent();
    self->fireEvent(connected);

    if (connected) {
        self->mSensorsAgent->setPhysicalParameterTarget(
                PHYSICAL_PARAMETER_AMBIENT_MOTION, kAmbientMotionExtentMeters,
                0.0f, 0.0f, PHYSICAL_INTERPOLATION_SMOOTH);

        glm::vec3 eulerDegrees;
        self->mSensorsAgent->getPhysicalParameter(
                PHYSICAL_PARAMETER_ROTATION, &eulerDegrees.x, &eulerDegrees.y,
                &eulerDegrees.z, PARAMETER_VALUE_TYPE_TARGET);
        // The physical model represents rotation in the X Y Z order, but
        // for mouselook we need the Y X Z order. Convert the rotation order
        // via a quaternion and decomposing the rotations.
        const glm::quat rotation =
                fromEulerAnglesXYZ(glm::radians(eulerDegrees));
        self->mEulerRotationDegrees = toEulerAnglesYXZ(rotation);

        if (self->mEulerRotationDegrees.x > M_PI) {
            self->mEulerRotationDegrees.x -= static_cast<float>(2 * M_PI);
        }

    } else {
        self->mSensorsAgent->setPhysicalParameterTarget(
                PHYSICAL_PARAMETER_AMBIENT_MOTION, 0.0f, 0.0f, 0.0f,
                PHYSICAL_INTERPOLATION_SMOOTH);
        self->mSensorsAgent->setPhysicalParameterTarget(
                PHYSICAL_PARAMETER_VELOCITY, 0.0f, 0.0f, 0.0f,
                PHYSICAL_INTERPOLATION_SMOOTH);
    }
}

bool VirtualSceneCamera::rotate(const glm::vec3 delta) {
    if (!mConnected)
        return false;

    mEulerRotationDegrees.y += delta.x;
    mEulerRotationDegrees.x += delta.y;
    mEulerRotationDegrees.z += delta.z;

    // Clamp up/down rotation to -80 and +80 degrees, like a FPS camera.
    if (mEulerRotationDegrees.x < glm::radians(kMinVerticalRotationDegrees)) {
        mEulerRotationDegrees.x = glm::radians(kMinVerticalRotationDegrees);
    } else if (mEulerRotationDegrees.y >
               glm::radians(kMaxVerticalRotationDegrees)) {
        mEulerRotationDegrees.y = glm::radians(kMaxVerticalRotationDegrees);
    }

    // Rotations applied in the Y X Z order, but we need to convert to X
    // Y Z order for the physical model.
    const glm::vec3 rotationDegrees = glm::degrees(
            toEulerAnglesXYZ(fromEulerAnglesYXZ(mEulerRotationDegrees)));
    mSensorsAgent->setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_ROTATION, rotationDegrees.x, rotationDegrees.y,
            rotationDegrees.z, PHYSICAL_INTERPOLATION_SMOOTH);
    return updateVelocity(mVelocity);
}

bool VirtualSceneCamera::updateVelocity(const glm::vec3 absolute) {
    if (!mConnected)
        return false;

    const glm::vec3 velocity = glm::angleAxis(mEulerRotationDegrees.y,
                                              glm::vec3(0.0f, 1.0f, 0.0f)) *
                               absolute;
    if (velocity != mVelocity) {
        mVelocity = velocity;
        mSensorsAgent->setPhysicalParameterTarget(
                PHYSICAL_PARAMETER_VELOCITY, velocity.x, velocity.y, velocity.z,
                PHYSICAL_INTERPOLATION_SMOOTH);
    }
    return true;
}

EventWaiter* VirtualSceneCamera::eventWaiter() {
    return &mEventWaiter;
}

}  // namespace control
}  // namespace emulation
}  // namespace android

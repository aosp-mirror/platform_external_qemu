// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/virtualscene/WASDInputHandler.h"
#include "aemu/base/Log.h"
#include "android/utils/debug.h"
#include "android/physics/GlmHelpers.h"
#include "android/physics/PhysicalModel.h"

namespace android {
namespace virtualscene {

static inline size_t toIndex(ControlKey key) {
    return static_cast<size_t>(key);
}

static const float kAmbientMotionExtentMeters = 0.005f;
static const float kMovementVelocityMetersPerSecond = 1.0f;

PhysicalModel::PhysicalModel(const QAndroidSensorsAgent* sensorsAgent)
    : mSensorsAgent(sensorsAgent) {}

void PhysicalModel::onEnable() {
    // Get the starting rotation.
    glm::vec3 eulerDegrees;
    std::vector<float*> out = {&eulerDegrees.x, &eulerDegrees.y,
                               &eulerDegrees.z};
    mSensorsAgent->getPhysicalParameter(PHYSICAL_PARAMETER_ROTATION, out.data(),
                                        out.size(),
                                        PARAMETER_VALUE_TYPE_TARGET);

    VLOG(virtualscene) << "Starting WASD, initial rotation: (" << eulerDegrees.x
                       << ", " << eulerDegrees.y << ", " << eulerDegrees.z
                       << ")";

    // The physical model represents rotation in the X Y Z order, but
    // for mouselook we need the Y X Z order. Convert the rotation order
    // via a quaternion and decomposing the rotations.
    const glm::quat rotation = fromEulerAnglesXYZ(glm::radians(eulerDegrees));
    mEulerRotationRadians = toEulerAnglesYXZ(rotation);

    if (mEulerRotationRadians.x > M_PI) {
        mEulerRotationRadians.x -= static_cast<float>(2 * M_PI);
    }

    VLOG(virtualscene) << "Converted rotation, radians: ("
                       << mEulerRotationRadians.x << ", "
                       << mEulerRotationRadians.y << ", "
                       << mEulerRotationRadians.z << ")";

    std::vector<float> val = {kAmbientMotionExtentMeters};
    mSensorsAgent->setPhysicalParameterTarget(PHYSICAL_PARAMETER_AMBIENT_MOTION,
                                              val.data(), val.size(),
                                              PHYSICAL_INTERPOLATION_SMOOTH);
}

void PhysicalModel::onDisable() {
    mVelocity = {0.0f, 0.0f, 0.0f};
    std::vector<float> val = {0.0f};
    mSensorsAgent->setPhysicalParameterTarget(PHYSICAL_PARAMETER_AMBIENT_MOTION,
                                              val.data(), val.size(),
                                              PHYSICAL_INTERPOLATION_SMOOTH);
}

// radianDelta is in XYZ order.
void PhysicalModel::updateRotation(glm::vec3 radianDelta) {
    mEulerRotationRadians.x += radianDelta.x;
    mEulerRotationRadians.y += radianDelta.y;

    // Clamp up/down rotation to -80 and +80 degrees, like a FPS camera.
    if (mEulerRotationRadians.x < glm::radians(kMinVerticalRotationDegrees)) {
        mEulerRotationRadians.x = glm::radians(kMinVerticalRotationDegrees);
    } else if (mEulerRotationRadians.x >
               glm::radians(kMaxVerticalRotationDegrees)) {
        mEulerRotationRadians.x = glm::radians(kMaxVerticalRotationDegrees);
    }

    // Rotations applied in the Y X Z order, but we need to convert to X
    // Y Z order for the physical model.
    const glm::vec3 rotationDegrees = glm::degrees(
            toEulerAnglesXYZ(fromEulerAnglesYXZ(mEulerRotationRadians)));
    std::vector<float> val = {rotationDegrees.x, rotationDegrees.y,
                              rotationDegrees.z};
    mSensorsAgent->setPhysicalParameterTarget(PHYSICAL_PARAMETER_ROTATION,
                                              val.data(), val.size(),
                                              PHYSICAL_INTERPOLATION_SMOOTH);
}

void PhysicalModel::updateVelocity(glm::vec3 baseVelocity) {
    const glm::vec3 velocity = glm::angleAxis(mEulerRotationRadians.y,
                                              glm::vec3(0.0f, 1.0f, 0.0f)) *
                               baseVelocity;

    if (velocity != mVelocity) {
        mVelocity = velocity;
        std::vector<float> val = {velocity.x, velocity.y, velocity.z};

        mSensorsAgent->setPhysicalParameterTarget(
                PHYSICAL_PARAMETER_VELOCITY, val.data(), val.size(),
                PHYSICAL_INTERPOLATION_SMOOTH);
    }
}

WASDInputHandler::WASDInputHandler(const QAndroidSensorsAgent* sensorsAgent)
    : mModel(sensorsAgent) {}

void WASDInputHandler::enable() {
    CHECK(!mEnabled);

    mEnabled = true;
    mModel.onEnable();
}

void WASDInputHandler::disable() {
    CHECK(mEnabled);

    mEnabled = false;

    // Unset velocity.
    for (bool& pressed : mKeysHeld) {
        pressed = false;
    }
    mModel.onDisable();
}

void WASDInputHandler::keyDown(ControlKey key) {
    if (!mEnabled) {
        return;
    }

    mKeysHeld[toIndex(key)] = true;
    updateVelocity();
}

void WASDInputHandler::keyUp(ControlKey key) {
    if (!mEnabled) {
        return;
    }

    mKeysHeld[toIndex(key)] = false;
    updateVelocity();
}

void WASDInputHandler::mouseMove(int offsetX, int offsetY) {
    if (!mEnabled) {
        return;
    }
    // Angles are measured counter clock-wise while in mouselook, offsets are
    // measured the opposite way.
    if (offsetX != 0 || offsetY != 0) {
        mModel.updateRotation({-offsetY * kPixelsToRotationRadians,
                               -offsetX * kPixelsToRotationRadians, 0.0f});
        updateVelocity();
    }
}

void WASDInputHandler::updateVelocity() {
    // Moving in an additional direction should not slow down other axes, so
    // this is intentionally not normalized. As a result, moving along
    // additional axes increases the overall velocity.
    const glm::vec3 lookDirectionBaseVelocity(
            (mKeysHeld[toIndex(ControlKey::A)] ? -1.0f : 0.0f) +
                    (mKeysHeld[toIndex(ControlKey::D)] ? 1.0f : 0.0f),
            (mKeysHeld[toIndex(ControlKey::Q)] ? -1.0f : 0.0f) +
                    (mKeysHeld[toIndex(ControlKey::E)] ? 1.0f : 0.0f),
            (mKeysHeld[toIndex(ControlKey::W)] ? -1.0f : 0.0f) +
                    (mKeysHeld[toIndex(ControlKey::S)] ? 1.0f : 0.0f));
    mModel.updateVelocity(lookDirectionBaseVelocity *
                          kMovementVelocityMetersPerSecond);
}

}  // namespace virtualscene
}  // namespace android

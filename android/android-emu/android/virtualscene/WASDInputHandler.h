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

#pragma once

#include "android/emulation/control/sensors_agent.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace android {
namespace virtualscene {

static constexpr float kPixelsToRotationRadians =
        0.2f * static_cast<float>(M_PI / 180.0f);
static const float kMinVerticalRotationDegrees = -80.0f;
static const float kMaxVerticalRotationDegrees = 80.0f;

enum class ControlKey { W, A, S, D, Q, E, Count };

// The physical model in sensors agent represents rotation in the X Y Z order,
// but for mouselook we need the Y X Z order. Convert the rotation order via a
// quaternion and decomposing the rotations.

// x axis is horizontal and orthogonal to the view direction.
// y axis points up and is perpendicular to the floor.
// z axis is the view direction
class PhysicalModel {
public:
    PhysicalModel(const QAndroidSensorsAgent* sensorsAgent);
    void onEnable();
    void onDisable();
    void updateRotation(glm::vec3 radianDelta);
    void updateVelocity(glm::vec3 baseVelocity);

private:
    glm::vec3 mVelocity = glm::vec3();
    glm::vec3 mEulerRotationRadians = glm::vec3();
    const QAndroidSensorsAgent* mSensorsAgent;
};

class WASDInputHandler {
public:
    WASDInputHandler(const QAndroidSensorsAgent* sensorsAgent);

    void enable();
    void disable();

    void keyDown(ControlKey key);
    void keyUp(ControlKey key);

    void mouseMove(int offsetX, int offsetY);

private:
    void updateMouselook();
    void updateVelocity();

    bool mEnabled = false;
    PhysicalModel mModel;
    bool mKeysHeld[static_cast<size_t>(ControlKey::Count)] = {};
};

}  // namespace virtualscene
}  // namespace android

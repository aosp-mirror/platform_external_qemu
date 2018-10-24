/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/virtualscene/SceneCamera.h"
#include "android/automation/AutomationController.h"
#include "android/hw-sensors.h"
#include "android/physics/GlmHelpers.h"
#include "android/physics/PhysicalModel.h"
#include "android/utils/debug.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

constexpr float kDefaultAspectRatio = 640.0f / 480.0f;

namespace android {
namespace virtualscene {

static glm::mat4 projectionMatrixForCameraIntrinsics(float width,
                                                     float height,
                                                     float fx,
                                                     float fy,
                                                     float cx,
                                                     float cy,
                                                     float zNear,
                                                     float zFar) {
    const float xscale = zNear / fx;
    const float yscale = zNear / fy;
    const float xoffset = (cx - (width / 2.0f)) * xscale;
    // Color camera's coordinates has y pointing downwards so we negate this
    // term.
    const float yoffset = -(cy - (height / 2.0f)) * yscale;

    return glm::frustum(xscale * -width / 2.0f - xoffset,
                        xscale * width / 2.0f - xoffset,
                        yscale * -height / 2.0f - yoffset,
                        yscale * height / 2.0f - yoffset, zNear, zFar);
}

static glm::mat4 poseToOpenGl(glm::quat rotation, glm::vec3 translation) {
    glm::mat4 mat(rotation);
    mat[3] = glm::vec4(translation, 1.0f);
    return mat;
}

SceneCamera::SceneCamera() {
    setAspectRatio(kDefaultAspectRatio);

    const glm::mat4 openGlFromSensors = poseToOpenGl(
            glm::quat(sqrt(2.0f) / 2.0f, 0.0f, 0.0f, sqrt(2.0f) / 2.0f),
            glm::vec3(-0.03f, -0.06f, 0.002f));

    const glm::mat4 cameraFromOpenGl = glm::mat4(1.0f,  0.0f, 0.0f, 0.0f,
                                                 0.0f, -1.0f, 0.0f, 0.0f,
                                                 0.0f,  0.0f, 1.0f, 0.0f,
                                                 0.0f,  0.0f, 0.0f, 1.0f);

    mCameraFromSensors = cameraFromOpenGl * openGlFromSensors;
}

SceneCamera::~SceneCamera() = default;

void SceneCamera::setAspectRatio(float aspectRatio) {
    mProjection = projectionMatrixForCameraIntrinsics(aspectRatio,  // width
                                                      1.0f,         // height
                                                      1.0f,         // fx
                                                      1.0f,         // fy
                                                      aspectRatio / 2.0f,  // cx
                                                      0.5f,                // cy
                                                      0.1f,     // zNear
                                                      100.0f);  // zFar
}

void SceneCamera::update() {
    // Note: when getting the transform, we always update the current time
    //       so that consumers of this transform get the most up-to-date
    //       value possible, and so that transform timestamps progress even when
    //       sensors are not polling.
    automation::AutomationController::tryAdvanceTime();

    int64_t timestamp;
    glm::vec3 position;
    glm::vec3 rotationEulerDegrees;
    physicalModel_getTransform(android_physical_model_instance(), &position.x,
                               &position.y, &position.z,
                               &rotationEulerDegrees.x, &rotationEulerDegrees.y,
                               &rotationEulerDegrees.z, &timestamp);

    const glm::mat4 rotationMat(
            fromEulerAnglesXYZ(glm::radians(rotationEulerDegrees)));

    const glm::mat4 inverseSensorsPose =
            glm::inverse(rotationMat) * glm::translate(glm::mat4(), -position);

    mViewFromWorld = mCameraFromSensors * inverseSensorsPose;

    mLastUpdateTime = timestamp;
}

glm::mat4 SceneCamera::getViewProjection() const {
    return mProjection * mViewFromWorld;
}

int64_t SceneCamera::getTimestamp() const {
    return mLastUpdateTime;
}

}  // namespace virtualscene
}  // namespace android

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
#include "android/hw-sensors.h"
#include "android/utils/debug.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

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
    glm::mat4 mat = glm::toMat4(rotation);
    mat[3] = glm::vec4(translation, 1.0f);
    return mat;
}

SceneCamera::SceneCamera() {
    mProjection = projectionMatrixForCameraIntrinsics(640.0f,    // width
                                                      480.0f,    // height
                                                      478.818f,  // fx
                                                      478.745f,  // fy
                                                      320.398f,  // cx
                                                      241.369f,  // cy
                                                      0.1f,      // zNear
                                                      100.0f);   // zFar

    const glm::mat4 openGlFromSensors = poseToOpenGl(
            glm::quat(sqrt(2.0f) / 2.0f, 0.0f, 0.0f, sqrt(2.0f) / 2.0f),
            glm::vec3(0.03f, 0.06f, -0.002f));

    const glm::mat4 cameraFromOpenGl = glm::mat4(1.0f,  0.0f, 0.0f, 0.0f,
                                                 0.0f, -1.0f, 0.0f, 0.0f,
                                                 0.0f,  0.0f, 1.0f, 0.0f,
                                                 0.0f,  0.0f, 0.0f, 1.0f);

    mCameraFromSensors = cameraFromOpenGl * openGlFromSensors;
}

SceneCamera::~SceneCamera() = default;

int64_t SceneCamera::update() {
    int64_t timestamp;
    glm::vec3 position;
    glm::vec3 rotationEuler;
    android_physical_model_get_transform(&position.x, &position.y, &position.z,
            &rotationEuler.x, &rotationEuler.y, &rotationEuler.z, &timestamp);

    const glm::mat4 rotationMat = glm::eulerAngleXYZ(
            glm::radians(rotationEuler.x), glm::radians(rotationEuler.y),
            glm::radians(rotationEuler.z));

    const glm::mat4 inverseSensorsPose =
            glm::inverse(rotationMat) * glm::translate(glm::mat4(), -position);

    mViewFromWorld = mCameraFromSensors * inverseSensorsPose;

    return timestamp;
}

glm::mat4 SceneCamera::getViewProjection() const {
    return mProjection * mViewFromWorld;
}

}  // namespace virtualscene
}  // namespace android

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

#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

constexpr float kPhysicsEpsilon = 0.001f;

inline bool vecNearEqual(glm::vec3 lhs,
                         glm::vec3 rhs,
                         float epsilon = kPhysicsEpsilon) {
    return glm::all(glm::epsilonEqual(lhs, rhs, epsilon));
}

inline bool quaternionNearEqual(glm::quat lhs,
                                glm::quat rhs,
                                float epsilon = kPhysicsEpsilon) {
    return glm::all(glm::epsilonEqual(lhs, rhs, epsilon)) ||
           glm::all(glm::epsilonEqual(lhs, -rhs, epsilon));
}

inline glm::vec3 toEulerAnglesXYZ(const glm::quat& q) {
    const glm::quat sq(q.w * q.w, q.x * q.x, q.y * q.y, q.z * q.z);

    return glm::vec3(
            glm::atan(2.0f * (q.x * q.w - q.y * q.z),
                      sq.w - sq.x - sq.y + sq.z),
            glm::asin(glm::clamp(2.0f * (q.x * q.z + q.y * q.w), -1.0f, 1.0f)),
            glm::atan(2.0f * (q.z * q.w - q.x * q.y),
                      sq.w + sq.x - sq.y - sq.z));
}

inline glm::quat fromEulerAnglesXYZ(const glm::vec3& euler) {
    const glm::quat X = glm::angleAxis(euler.x, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat Y = glm::angleAxis(euler.y, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat Z = glm::angleAxis(euler.z, glm::vec3(0.0f, 0.0f, 1.0f));

    return X * Y * Z;
}

inline glm::vec3 toEulerAnglesYXZ(const glm::quat& q) {
    const glm::quat sq(q.w * q.w, q.x * q.x, q.y * q.y, q.z * q.z);

    return glm::vec3(
            glm::asin(glm::clamp(2.0f * (q.x * q.w - q.y * q.z), -1.0f, 1.0f)),
            glm::atan(2.0f * (q.x * q.z + q.y * q.w),
                      sq.w - sq.x - sq.y + sq.z),
            glm::atan(2.0f * (q.x * q.y + q.z * q.w),
                      sq.w - sq.x + sq.y - sq.z));
}

inline glm::quat fromEulerAnglesYXZ(const glm::vec3& euler) {
    const glm::quat X = glm::angleAxis(euler.x, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat Y = glm::angleAxis(euler.y, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat Z = glm::angleAxis(euler.z, glm::vec3(0.0f, 0.0f, 1.0f));

    return Y * X * Z;
}

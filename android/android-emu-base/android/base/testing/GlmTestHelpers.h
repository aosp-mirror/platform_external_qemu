// Copyright 2018 The Android Open Source Project
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

#include "android/physics/GlmHelpers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace glm {
inline void PrintTo(const glm::vec3& v, std::ostream* os) {
    *os << "vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
}

inline void PrintTo(const glm::quat& q, std::ostream* os) {
    *os << "quat(" << q.x << " " << q.y << " " << q.z << " " << q.w << ")";
}
}  // namespace glm

// Use as: EXPECT_THAT(quat, Vec3Near(expected, 0.01f));
MATCHER_P2(Vec3Near,
           expected,
           epsilon,
           std::string(negation ? "isn't" : "is") + " near " +
                   ::testing::PrintToString(expected) +
                   ", within epsilon=" + ::testing::PrintToString(epsilon)) {
    return vectorNearEqual(arg, expected, epsilon);
}

// Use as: EXPECT_THAT(quat, QuatNear(expected, 0.01f));
MATCHER_P2(QuatNear,
           expected,
           epsilon,
           std::string(negation ? "isn't" : "is") + " near " +
                   ::testing::PrintToString(expected) +
                   ", within epsilon=" + ::testing::PrintToString(epsilon)) {
    return quaternionNearEqual(arg, expected, epsilon);
}

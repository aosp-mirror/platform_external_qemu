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

#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <array>

struct QAndroidSensorsAgent;
class InertialModel {
public:
    InertialModel();

    void setTargetPosition(glm::vec3 position);
    void setTargetRotation(glm::quat rotation);
    void setMagneticValue(float north, float east, float vertical);

    glm::vec3 getPosition() const;
    glm::vec3 getAcceleration() const;
    glm::vec3 getMagneticVector() const;
    glm::quat getRotation() const;
    glm::vec3 getGyroscope() const;

    void setSensorsAgent(const QAndroidSensorsAgent* agent);
private:
    void updateSensorValues();
    void updateAccelerations();

    void recalcRotationUpdateInterval();

    glm::vec3 mLinearAcceleration;
    glm::vec3 mPreviousPosition;
    glm::vec3 mCurrentPosition;
    glm::quat mPreviousRotation;
    glm::quat mCurrentRotation;

    glm::vec3 mAcceleration;
    glm::vec3 mMagneticVector;
    glm::vec3 mGyroscope;

    const QAndroidSensorsAgent* mSensorsAgent;

    uint64_t mUpdateIntervalMs = 0;
    uint64_t mLastRotationUpdateMs = 0;
    static const uint64_t ROTATION_UPDATE_RESET_TIME_MS = 100;
    static const size_t ROTATION_UPDATE_TIME_WINDOW_SIZE = 8;
    std::array<uint64_t, ROTATION_UPDATE_TIME_WINDOW_SIZE>
        mLastUpdateIntervals = {};
    size_t mRotationUpdateTimeWindowElt = 0;
};


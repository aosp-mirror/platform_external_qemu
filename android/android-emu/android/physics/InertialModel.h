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

namespace android {
namespace physics {

/*
 * Implements a model of inertial motion of a rigid body such that smooth
 * movement occurs bringing the body to the target rotation and position with
 * dependent sensor data constantly available and up to date.
 *
 * The inertial model should be given a pointer to a valid sensor agent via
 * setSensorAgent, after which dependent sensor data (gyroscope,
 * accelerometer, magnetometer) will be regularly sent to the agent. A target
 * position and rotation should be set via user input (such as UI controls,
 * mouse, keyboard, etc...) - using setTargetPosition and setTargetRotation.
 * The various getXXXX() calls may be regularly polled to update UI, or if the
 * current sensor values are needed for any reason.
 */
class InertialModel {
public:
    InertialModel() = default;

    /*
     * Sets the position that the modeled object should move toward.
     */
    void setTargetPosition(glm::vec3 position, bool instantaneous);

    /*
     * Sets the rotation that the modeled object should move toward.
     */
    void setTargetRotation(glm::quat rotation, bool instantaneous);

    /*
     * Sets the strength of the magnetic field in which the modeled object
     * is moving, in order to simulate a magnetic vector.
     */
    void setMagneticValue(
        float north, float east, float vertical, bool instantaneous);

    /*
     * Gets current simulated state and sensor values of the modeled object.
     */
    glm::vec3 getPosition() const;
    glm::vec3 getAcceleration() const;
    glm::vec3 getMagneticVector() const;
    glm::quat getRotation() const;
    glm::vec3 getGyroscope() const;

    /*
     * Sets the agent to which sensor data will be sent.
     */
    void setSensorsAgent(const QAndroidSensorsAgent* agent);
private:
    void updateSensorValues();
    void updateAccelerations();

    void recalcRotationUpdateInterval();

    glm::vec3 mLinearAcceleration = glm::vec3(0.f);
    glm::vec3 mPreviousPosition = glm::vec3(0.f);
    glm::vec3 mCurrentPosition = glm::vec3(0.f);
    glm::quat mPreviousRotation;
    glm::quat mCurrentRotation;

    glm::vec3 mAcceleration = glm::vec3(0.f, 9.81f, 0.f);
    glm::vec3 mMagneticVector = glm::vec3(1.f, 0.f, 0.f);
    glm::vec3 mGyroscope = glm::vec3(0.f);

    const QAndroidSensorsAgent* mSensorsAgent = nullptr;

    uint64_t mUpdateIntervalMs = 0;
    uint64_t mLastRotationUpdateMs = 0;
    static const uint64_t ROTATION_UPDATE_RESET_TIME_MS = 100;
    static const size_t ROTATION_UPDATE_TIME_WINDOW_SIZE = 8;
    std::array<uint64_t, ROTATION_UPDATE_TIME_WINDOW_SIZE>
        mLastUpdateIntervals = {};
    size_t mRotationUpdateTimeWindowElt = 0;
};

}  // namespace physics
}  // namespace android

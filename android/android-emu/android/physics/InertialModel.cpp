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

#include "android/physics/InertialModel.h"

#include "android/base/system/System.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/hw-sensors.h"

namespace android {
namespace physics {

void InertialModel::setTargetPosition(glm::vec3 position, bool instantaneous) {
    if (instantaneous) {
        mPreviousPosition = position;
    }
    mCurrentPosition = position;
    updateAccelerations();
    updateSensorValues();
}

void InertialModel::setTargetRotation(glm::quat rotation, bool instantaneous) {
    if (instantaneous) {
        mPreviousRotation = rotation;
    } else {
        mPreviousRotation = mCurrentRotation;
    }
    mCurrentRotation = rotation;
    recalcRotationUpdateInterval();
    updateSensorValues();
}

void InertialModel::setMagneticValue(
        float north, float east, float vertical, bool instantanous) {
    mMagneticVector = glm::vec3(north, east, vertical);
    updateSensorValues();
}

glm::vec3 InertialModel::getPosition() const {
    return mCurrentPosition;
}

glm::vec3 InertialModel::getAcceleration() const {
    return mAcceleration;
}

glm::vec3 InertialModel::getMagneticVector() const {
    return mMagneticVector;
}

glm::quat InertialModel::getRotation() const {
    return mCurrentRotation;
}

glm::vec3 InertialModel::getGyroscope() const {
    return mGyroscope;
}

void InertialModel::setSensorsAgent(const QAndroidSensorsAgent* agent) {
    mSensorsAgent = agent;
    updateSensorValues();
}

void InertialModel::updateSensorValues() {
    // Gravity and magnetic vector in the device's frame of
    // reference.
    constexpr glm::vec3 gravityVector(0.f, 9.81f, 0.f);

    const glm::quat deviceRotationQuat = mCurrentRotation;
    const glm::quat rotationDelta =
            mCurrentRotation * glm::conjugate(mPreviousRotation);
    const glm::vec3 eulerAngles = glm::eulerAngles(rotationDelta);

    //Implementation Note:
    //Qt's rotation is around fixed axis, in the following
    //order: z first, x second and y last
    //refs:
    //http://doc.qt.io/qt-5/qquaternion.html#fromEulerAngles
    //
    // Gravity and magnetic vectors as observed by the device.
    // Note how we're applying the *inverse* of the transformation
    // represented by device_rotation_quat to the "absolute" coordinates
    // of the vectors.
    const glm::vec3 deviceGravityVector =
        glm::conjugate(deviceRotationQuat) * gravityVector;
    mMagneticVector =
        glm::conjugate(deviceRotationQuat) * mMagneticVector;
    mAcceleration = deviceGravityVector - mLinearAcceleration;

    const float gyroUpdateRate = mUpdateIntervalMs < 16 ? 0.016f :
            mUpdateIntervalMs / 1000.0f;

    // Convert raw UI pitch/roll/yaw delta
    // to device-space rotation in radians per second.
    mGyroscope = glm::conjugate(deviceRotationQuat) *
            (eulerAngles * static_cast<float>(M_PI) / (180.f * gyroUpdateRate));

    if (mSensorsAgent != nullptr) {
        mSensorsAgent->setSensor(
                   ANDROID_SENSOR_ACCELERATION,
                   mAcceleration.x,
                   mAcceleration.y,
                   mAcceleration.z);

        mSensorsAgent->setSensor(ANDROID_SENSOR_GYROSCOPE,
                   mGyroscope.x, mGyroscope.y, mGyroscope.z);

        mSensorsAgent->setSensor(ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED,
                   mGyroscope.x, mGyroscope.y, mGyroscope.z);

        mSensorsAgent->setSensor(
                   ANDROID_SENSOR_MAGNETIC_FIELD,
                   mMagneticVector.x,
                   mMagneticVector.y,
                   mMagneticVector.z);

        mSensorsAgent->setSensor(
                   ANDROID_SENSOR_MAGNETIC_FIELD_UNCALIBRATED,
                   mMagneticVector.x,
                   mMagneticVector.y,
                   mMagneticVector.z);
    }
}

void InertialModel::updateAccelerations() {
    constexpr float k = 100.f;
    constexpr float mass = 1.f;
    constexpr float meters_per_unit = 0.0254f;

    const glm::vec3 delta = glm::conjugate(mCurrentRotation) *
            (meters_per_unit * (mCurrentPosition - mPreviousPosition));
    mLinearAcceleration = delta * k / mass;
    mPreviousPosition = mCurrentPosition;

    updateSensorValues();
}

void InertialModel::recalcRotationUpdateInterval() {
    const uint64_t currTimeMs = static_cast<uint64_t>(
        android::base::System::get()->getHighResTimeUs() / 1000);

    if (currTimeMs - mLastRotationUpdateMs > ROTATION_UPDATE_RESET_TIME_MS) {
        std::fill(mLastUpdateIntervals.begin(),
                  mLastUpdateIntervals.end(), 0);
    }

    mLastUpdateIntervals[mRotationUpdateTimeWindowElt] =
        (currTimeMs - mLastRotationUpdateMs);
    mRotationUpdateTimeWindowElt++;

    if (mRotationUpdateTimeWindowElt >= ROTATION_UPDATE_TIME_WINDOW_SIZE) {
        mRotationUpdateTimeWindowElt = 0;
    }

    mUpdateIntervalMs = 0;

    // Filter out window entries where the update interval is
    // still calculated as 0 due to not being initialized yet.
    uint64_t nontrivialUpdateIntervals = 0;
    for (const auto& elt: mLastUpdateIntervals) {
        mUpdateIntervalMs += elt;
        if (elt) nontrivialUpdateIntervals++;
    }

    if (nontrivialUpdateIntervals)
        mUpdateIntervalMs /= nontrivialUpdateIntervals;

    mLastRotationUpdateMs = currTimeMs;
}

}  // namespace physics
}  // namespace android

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

#include "android/physics/AmbientEnvironment.h"

namespace android {
namespace physics {

void AmbientEnvironment::setMagneticField(
        float north, float east, float vertical, bool instantanous) {
    mMagneticField = glm::vec3(north, east, vertical);
}

void AmbientEnvironment::setGravity(glm::vec3 gravity, bool instantaneous) {
    mGravity = gravity;
}
    
void AmbientEnvironment::setTemperature(float celsius, bool instantaneous) {
    mTemperature = celsius;
}
    
void AmbientEnvironment::setProximity(float centimeters, bool instantaneous) {
    mProximity = centimeters;
}
    
void AmbientEnvironment::setLight(float lux, bool instantaneous) {
    mLight = lux;
}
    
void AmbientEnvironment::setPressure(float hPa, bool instantaneous) {
    mPressure = hPa;
}
    
void AmbientEnvironment::setHumidity(float percent, bool instantaneous) {
    mHumidity = percent;
}

glm::vec3 AmbientEnvironment::getMagneticField() const {
    return mMagneticField;
}

glm::vec3 AmbientEnvironment::getGravity() const {
    return mGravity;
}

float AmbientEnvironment::getTemperature() const {
    return mTemperature;
}

float AmbientEnvironment::getProximity() const {
    return mProximity;
}

float AmbientEnvironment::getLight() const {
    return mLight;
}

float AmbientEnvironment::getPressure() const {
    return mPressure;
}

float AmbientEnvironment::getHumidity() const {
    return mHumidity;
}

}  // namespace physics
}  // namespace android

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

#include "android/emulation/testing/MockAndroidSensorsAgent.h"
#include "aemu/base/Log.h"

static const QAndroidSensorsAgent sQAndroidSensorsAgent = {
        .setPhysicalParameterTarget =
                [](int parameterId,
                   const float* val,
                   const size_t count,
                   int interpolationMethod) {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock
                            ->setPhysicalParameterTarget(parameterId, val,
                                                         count,
                                                         interpolationMethod);
                },
        .getPhysicalParameter =
                [](int parameterId,
                   float* const* out,
                   const size_t count,
                   int parameterValueType) {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock
                            ->getPhysicalParameter(parameterId, out, count,
                                                   parameterValueType);
                },
        .setCoarseOrientation =
                [](int orientation) {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock
                            ->setCoarseOrientation(orientation);
                },
        .setSensorOverride =
                [](int sensorId, const float* val, const size_t count) {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock
                            ->setSensorOverride(sensorId, val, count);
                },
        .getSensor =
                [](int sensorId, float* const* val, const size_t count) {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock->getSensor(
                            sensorId, val, count);
                },
        .getDelayMs =
                []() {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock->getDelayMs();
                },
        .setPhysicalStateAgent =
                [](const struct QAndroidPhysicalStateAgent* agent) {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock
                            ->setPhysicalStateAgent(agent);
                },
        .advanceTime =
                []() {
                    CHECK(android::MockAndroidSensorsAgent::mock);
                    return android::MockAndroidSensorsAgent::mock
                            ->advanceTime();
                }};

extern "C" const QAndroidSensorsAgent* const gQAndroidSensorsAgent =
        &sQAndroidSensorsAgent;

namespace android {

MockAndroidSensorsAgent* MockAndroidSensorsAgent::mock = nullptr;

const QAndroidSensorsAgent* MockAndroidSensorsAgent::getAgent() {
    return gQAndroidSensorsAgent;
}

}  // namespace android

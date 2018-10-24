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

#include "android/virtualscene/WASDInputHandler.h"
#include "android/base/testing/GlmTestHelpers.h"
#include "android/base/testing/MockUtils.h"
#include "android/emulation/testing/MockAndroidSensorsAgent.h"
#include "android/physics/PhysicalModel.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/vec3.hpp>

using android::MockAndroidSensorsAgent;
using namespace android::virtualscene;

using testing::_;
using testing::AnyOf;
using testing::AllOf;
using testing::Eq;
using testing::Expectation;
using testing::FloatNear;
using testing::Invoke;
using testing::Mock;

namespace {

static constexpr int kPixelsFor180 =
        static_cast<int>(M_PI / kPixelsToRotationRadians);

struct TestCase {
    const char* name;
    ControlKey key;
    glm::vec3 velocity;
};

static constexpr TestCase kIsolatedDirectionCases[] = {
        {"W", ControlKey::W, glm::vec3(0.0f, 0.0f, -1.0f)},
        {"S", ControlKey::S, glm::vec3(0.0f, 0.0f, 1.0f)},
        {"A", ControlKey::A, glm::vec3(-1.0f, 0.0f, 0.0f)},
        {"D", ControlKey::D, glm::vec3(1.0f, 0.0f, 0.0f)},
        {"Q", ControlKey::Q, glm::vec3(0.0f, -1.0f, 0.0f)},
        {"E", ControlKey::E, glm::vec3(0.0f, 1.0f, 0.0f)},
};
}  // namespace
class WASDInputHandlerTest : public testing::Test {
protected:
    void SetUp() override {
        mMockReplacement = replaceMock(&MockAndroidSensorsAgent::mock, &mMock);
    }

    WASDInputHandler createHandler() {
        return WASDInputHandler(MockAndroidSensorsAgent::getAgent());
    }

    void enableWithRotation(WASDInputHandler& handler,
                            const glm::vec3& rotationDegrees) {
        EXPECT_CALL(mMock,
                    getPhysicalParameter(Eq(PHYSICAL_PARAMETER_ROTATION), _, _,
                                         _, Eq(PARAMETER_VALUE_TYPE_TARGET)))
                .WillOnce(Invoke([rotationDegrees](int, float* a, float* b,
                                                   float* c, int) {
                    *a = rotationDegrees.x;
                    *b = rotationDegrees.y;
                    *c = rotationDegrees.z;
                    return PHYSICAL_PARAMETER_STATUS_OK;
                }));

        EXPECT_CALL(mMock,
                    setPhysicalParameterTarget(
                            Eq(PHYSICAL_PARAMETER_AMBIENT_MOTION), _, _, _, _));
        handler.enable();
        Mock::VerifyAndClear(&mMock);
    }

    void disable(WASDInputHandler& handler) {
        EXPECT_CALL(mMock,
                    setPhysicalParameterTarget(
                            Eq(PHYSICAL_PARAMETER_AMBIENT_MOTION), _, _, _, _));
        handler.disable();
        Mock::VerifyAndClear(&mMock);
    }

    void RunIsolatedDirectionTestCases(WASDInputHandler& handler) {
        for (auto testCase : kIsolatedDirectionCases) {
            SCOPED_TRACE(testing::Message()
                         << "TestCase: " << testCase.name << ", direction ("
                         << testCase.velocity.x << ", " << testCase.velocity.y
                         << ", " << testCase.velocity.z << ")");
            EXPECT_CALL(mMock,
                        setPhysicalParameterTarget(
                                Eq(PHYSICAL_PARAMETER_VELOCITY),
                                FloatNear(testCase.velocity.x, kPhysicsEpsilon),
                                FloatNear(testCase.velocity.y, kPhysicsEpsilon),
                                FloatNear(testCase.velocity.z, kPhysicsEpsilon),
                                _));
            handler.keyDown(testCase.key);
            EXPECT_CALL(mMock, setPhysicalParameterTarget(
                                       Eq(PHYSICAL_PARAMETER_VELOCITY),
                                       FloatNear(0.0f, kPhysicsEpsilon),
                                       FloatNear(0.0f, kPhysicsEpsilon),
                                       FloatNear(0.0f, kPhysicsEpsilon), _));
            handler.keyUp(testCase.key);
            Mock::VerifyAndClear(&mMock);
        }
    }

    void ExpectTargetRotationNear(glm::vec3 expectedRotationDegrees) {
        const glm::quat expectedRotation =
                fromEulerAnglesXYZ(glm::radians(expectedRotationDegrees));

        EXPECT_CALL(mMock, setPhysicalParameterTarget(
                                   Eq(PHYSICAL_PARAMETER_ROTATION), _, _, _, _))
                .WillOnce(Invoke([=](int, float a, float b, float c, int) {
                    const glm::quat rotation = fromEulerAnglesXYZ(
                            glm::radians(glm::vec3(a, b, c)));
                    EXPECT_THAT(rotation,
                                QuatNear(expectedRotation, kPhysicsEpsilon))
                            << "Expected degrees: "
                            << testing::PrintToString(expectedRotationDegrees)
                            << "\nActual degrees: "
                            << testing::PrintToString(glm::vec3(a, b, c));
                    return PHYSICAL_PARAMETER_STATUS_OK;
                }));
    }

    MockAndroidSensorsAgent mMock;
    MockReplacementHandle<MockAndroidSensorsAgent*> mMockReplacement;
};

TEST_F(WASDInputHandlerTest, Create) {
    createHandler();
}

TEST_F(WASDInputHandlerTest, EnableDisable) {
    WASDInputHandler handler = createHandler();
    enableWithRotation(handler, glm::vec3());
    disable(handler);
}

TEST_F(WASDInputHandlerTest, MoveInIsolation) {
    WASDInputHandler handler = createHandler();
    enableWithRotation(handler, glm::vec3());

    for (auto testCase : kIsolatedDirectionCases) {
        SCOPED_TRACE(testing::Message()
                     << "TestCase: " << testCase.name << ", direction ("
                     << testCase.velocity.x << ", " << testCase.velocity.y
                     << ", " << testCase.velocity.z << ")");
        EXPECT_CALL(mMock,
                    setPhysicalParameterTarget(Eq(PHYSICAL_PARAMETER_VELOCITY),
                                               Eq(testCase.velocity.x),
                                               Eq(testCase.velocity.y),
                                               Eq(testCase.velocity.z), _));
        handler.keyDown(testCase.key);
        EXPECT_CALL(mMock, setPhysicalParameterTarget(
                                   Eq(PHYSICAL_PARAMETER_VELOCITY), Eq(0.0f),
                                   Eq(0.0f), Eq(0.0f), _));
        handler.keyUp(testCase.key);
        Mock::VerifyAndClear(&mMock);
    }
}

TEST_F(WASDInputHandlerTest, MoveWithXRotation) {
    for (float rotation :
         {-90.0f, -80.0f, -45.0f, -10.0f, 10.0f, 45.0f, 80.0f, 90.0f}) {
        SCOPED_TRACE(testing::Message() << "Rotation: " << rotation);

        WASDInputHandler handler = createHandler();
        enableWithRotation(handler, glm::vec3(rotation, 0.0f, 0.0f));
        RunIsolatedDirectionTestCases(handler);
    }
}

TEST_F(WASDInputHandlerTest, MoveWithYRotation) {
    for (float rotation :
         {-180.0f, -135.0f, -90.0f, -45.0f, 45.0f, 90.0f, 135.0f, 180.0f}) {
        SCOPED_TRACE(testing::Message() << "Rotation: " << rotation);

        WASDInputHandler handler = createHandler();
        enableWithRotation(handler, glm::vec3(0.0f, rotation, 0.0f));

        for (auto testCase : kIsolatedDirectionCases) {
            const glm::vec3 rotatedVelocity(
                    glm::rotateY(testCase.velocity, glm::radians(rotation)));

            SCOPED_TRACE(testing::Message()
                         << "TestCase: " << testCase.name << ", direction ("
                         << rotatedVelocity.x << ", " << rotatedVelocity.y
                         << ", " << rotatedVelocity.z << ")");
            EXPECT_CALL(
                    mMock,
                    setPhysicalParameterTarget(
                            Eq(PHYSICAL_PARAMETER_VELOCITY),
                            FloatNear(rotatedVelocity.x, kPhysicsEpsilon),
                            FloatNear(rotatedVelocity.y, kPhysicsEpsilon),
                            FloatNear(rotatedVelocity.z, kPhysicsEpsilon), _));
            handler.keyDown(testCase.key);
            EXPECT_CALL(mMock, setPhysicalParameterTarget(
                                       Eq(PHYSICAL_PARAMETER_VELOCITY),
                                       FloatNear(0.0f, kPhysicsEpsilon),
                                       FloatNear(0.0f, kPhysicsEpsilon),
                                       FloatNear(0.0f, kPhysicsEpsilon), _));
            handler.keyUp(testCase.key);
            Mock::VerifyAndClear(&mMock);
        }
    }
}

TEST_F(WASDInputHandlerTest, MoveWithZRotation) {
    for (float rotation :
         {-180.0f, -135.0f, -90.0f, -45.0f, 45.0f, 90.0f, 135.0f, 180.0f}) {
        SCOPED_TRACE(testing::Message() << "Rotation: " << rotation);

        WASDInputHandler handler = createHandler();
        enableWithRotation(handler, glm::vec3(0.0f, 0.0f, rotation));
        RunIsolatedDirectionTestCases(handler);
    }
}

TEST_F(WASDInputHandlerTest, MouseHorizontal) {
    WASDInputHandler handler = createHandler();
    enableWithRotation(handler, glm::vec3());

    // No movement, no update.
    handler.mouseMove(0, 0);

    // Move to the right and back.
    ExpectTargetRotationNear(glm::vec3(0.0f, -90.0f, 0.0f));
    handler.mouseMove(kPixelsFor180 / 2, 0);
    ExpectTargetRotationNear(glm::vec3(0.0f, 0.0f, 0.0f));
    handler.mouseMove(-kPixelsFor180 / 2, 0);
    Mock::VerifyAndClear(&mMock);

    // Move left and back.
    ExpectTargetRotationNear(glm::vec3(0.0f, 90.0f, 0.0f));
    handler.mouseMove(-kPixelsFor180 / 2, 0);
    ExpectTargetRotationNear(glm::vec3(0.0f, 0.0f, 0.0f));
    handler.mouseMove(kPixelsFor180 / 2, 0);
    Mock::VerifyAndClear(&mMock);

    // Full turn.
    ExpectTargetRotationNear(glm::vec3(0.0f, 0.0f, 0.0f));
    handler.mouseMove(kPixelsFor180 * 2, 0);
}

TEST_F(WASDInputHandlerTest, MouseHorizontalSweep) {
    WASDInputHandler handler = createHandler();
    enableWithRotation(handler, glm::vec3());

    // Smoothly go around in a circle.
    const int stepPixels = 10;
    const float degreesPerStep = 180.0f / kPixelsFor180 * stepPixels;

    float rotationY = 0.0f;
    for (int i = stepPixels; i <= kPixelsFor180 * 2; i += stepPixels) {
        SCOPED_TRACE(testing::Message() << "Movement x=" << i);

        rotationY -= degreesPerStep;
        ExpectTargetRotationNear(glm::vec3(0.0f, rotationY, 0.0f));
        handler.mouseMove(stepPixels, 0);
    }

    // Go the other way.
    rotationY = 0.0f;
    for (int i = stepPixels; i >= -kPixelsFor180 * 2; i -= stepPixels) {
        SCOPED_TRACE(testing::Message() << "Movement x=" << i);

        rotationY += degreesPerStep;
        ExpectTargetRotationNear(glm::vec3(0.0f, rotationY, 0.0f));
        handler.mouseMove(-stepPixels, 0);
    }
}

TEST_F(WASDInputHandlerTest, MouseVertical) {
    WASDInputHandler handler = createHandler();
    enableWithRotation(handler, glm::vec3());

    // Move up and back.
    ExpectTargetRotationNear(glm::vec3(-45.0f, 0.0f, 0.0f));
    handler.mouseMove(0, kPixelsFor180 / 4);
    ExpectTargetRotationNear(glm::vec3(0.0f, 0.0f, 0.0f));
    handler.mouseMove(0, -kPixelsFor180 / 4);
    Mock::VerifyAndClear(&mMock);

    // Move down and back.
    ExpectTargetRotationNear(glm::vec3(45.0f, 0.0f, 0.0f));
    handler.mouseMove(0, -kPixelsFor180 / 4);
    ExpectTargetRotationNear(glm::vec3(0.0f, 0.0f, 0.0f));
    handler.mouseMove(0, kPixelsFor180 / 4);
    Mock::VerifyAndClear(&mMock);
}

TEST_F(WASDInputHandlerTest, MouseVerticalLimits) {
    WASDInputHandler handler = createHandler();
    enableWithRotation(handler, glm::vec3());

    ExpectTargetRotationNear(
            glm::vec3(kMinVerticalRotationDegrees, 0.0f, 0.0f));
    handler.mouseMove(0, kPixelsFor180);
    ExpectTargetRotationNear(
            glm::vec3(kMaxVerticalRotationDegrees, 0.0f, 0.0f));
    handler.mouseMove(0, -kPixelsFor180);
    Mock::VerifyAndClear(&mMock);
}

TEST_F(WASDInputHandlerTest, MouseVerticalAndHorizontal) {
    WASDInputHandler handler = createHandler();
    enableWithRotation(handler, glm::vec3());

    ExpectTargetRotationNear(glm::vec3(0.0f, -90.0f, 0.0f));
    handler.mouseMove(kPixelsFor180 / 2, 0);
    Mock::VerifyAndClear(&mMock);

    // This isn't intuitive since the rotation is applied in a different order.
    ExpectTargetRotationNear(glm::vec3(-90.0f, -45.0f, -90.0f));
    handler.mouseMove(0, kPixelsFor180 / 4);
    Mock::VerifyAndClear(&mMock);

    ExpectTargetRotationNear(glm::vec3(0.0f, -180.0f, 0.0f));
    handler.mouseMove(kPixelsFor180 / 2, -kPixelsFor180 / 4);
    Mock::VerifyAndClear(&mMock);

    ExpectTargetRotationNear(glm::vec3(90.0f, 45.0f, -90.0f));
    handler.mouseMove(kPixelsFor180 / 2, -kPixelsFor180 / 4);
    Mock::VerifyAndClear(&mMock);

    ExpectTargetRotationNear(glm::vec3(0.0f, 0.0f, 0.0f));
    handler.mouseMove(kPixelsFor180 / 2, kPixelsFor180 / 4);
    Mock::VerifyAndClear(&mMock);
}

TEST_F(WASDInputHandlerTest, MouseToggleWithZRotation) {
    for (float rotation : {-180.0f, -135.0f, -90.0f, -45.0f, 0.0f, 45.0f, 90.0f,
                           135.0f, 180.0f}) {
        SCOPED_TRACE(testing::Message() << "Rotation: " << rotation);

        const glm::vec3 initialRotation(0.0f, 0.0f, rotation);

        WASDInputHandler handler = createHandler();
        enableWithRotation(handler, initialRotation);

        glm::vec3 parameterTarget;
        EXPECT_CALL(mMock, setPhysicalParameterTarget(
                                   Eq(PHYSICAL_PARAMETER_ROTATION), _, _, _, _))
                .WillOnce(Invoke([&parameterTarget](int, float a, float b,
                                                    float c, int) {
                    parameterTarget = glm::vec3(a, b, c);
                    return PHYSICAL_PARAMETER_STATUS_OK;
                }));
        handler.mouseMove(-kPixelsFor180 * 80 / 90, kPixelsFor180 / 4);

        disable(handler);
        enableWithRotation(handler, parameterTarget);

        ExpectTargetRotationNear(initialRotation);
        handler.mouseMove(kPixelsFor180 * 80 / 90, -kPixelsFor180 / 4);
    }
}

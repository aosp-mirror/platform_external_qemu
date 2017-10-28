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

#include "android/physics/PhysicalModel.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include "android/base/system/System.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/hw-sensors.h"
#include "android/physics/AmbientEnvironment.h"
#include "android/physics/InertialModel.h"

namespace android {
namespace physics {

/*
 * Implements a model of an ambient environment containing a rigid
 * body, and produces accurately simulated sensor values for various
 * sensors in this environment.
 *
 * The physical model should be updated with target ambient and rigid
 * body state, and regularly polled for the most recent sensor values.
 *
 * Components that only require updates when the model is actively
 * changing (i.e. not at rest) should register state change callbacks
 * via setStateChangingCallbacks.  TargetStateChange callbacks occur
 * on the same thread that setTargetXXXXXX is called from.  Sensor
 * state changing callbacks may occur on an arbitrary thread.
 */
class PhysicalModelImpl {
public:
    PhysicalModelImpl();

    /*
     * Gets the PhysicalModel interface for this object.
     */
    PhysicalModel* getPhysicalModel();

    static PhysicalModelImpl* getImpl(PhysicalModel* physicalModel);

    /*
     * Sets the position that the modeled object should move toward.
     */
    void setTargetPosition(vec3 position, bool instantaneous);

    /*
     * Sets the rotation that the modeled object should move toward.
     */
    void setTargetRotation(vec3 rotation, bool instantaneous);

    /*
     * Sets the strength of the magnetic field in which the modeled object
     * is moving, in order to simulate a magnetic vector.
     */
    void setMagneticField(
        float north, float east, float vertical, bool instantaneous);

    /*
     * Gets current simulated state and sensor values of the modeled object.
     */
    vec3 getPosition() const;
    vec3 getRotation() const;

#define OVERRIDE_FUNCTION_NAME(x) override##x
#define SENSOR_(x,y,z) void OVERRIDE_FUNCTION_NAME(y)(z override_value);
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_FUNCTION_NAME

    /*
     * Getters for all sensor values.
     */
#define GET_FUNCTION_NAME(x) get##x
#define SENSOR_(x,y,z) z GET_FUNCTION_NAME(y)() const;
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef GET_FUNCTION_NAME

    /*
     * Sets the callbacks used to signal changing state.
     */
    void setPhysicalStateAgent(const QAndroidPhysicalStateAgent* agent);
private:
    /*
     * Getters for non-overridden physical sensor values.
     */
#define GET_PHYSICAL_NAME(x) getPhysical##x
#define SENSOR_(x,y,z) z GET_PHYSICAL_NAME(y)() const;
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef GET_PHYSICAL_NAME

    void physicalStateChanging();
    void physicalStateStabilized();
    void targetStateChanged();

    InertialModel mInertialModel;
    AmbientEnvironment mAmbientEnvironment;

    const QAndroidPhysicalStateAgent* mAgent = nullptr;

    bool mUseOverride[PHYSICAL_SENSORS_MAX] = {false};
#define OVERRIDE_NAME(x) m##x##Override
#define SENSOR_(x,y,z) z OVERRIDE_NAME(y){0.f};
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_NAME

    PhysicalModel mPhysicalModelInterface;
};


glm::vec3 toGlm(vec3 input) {
    return glm::vec3(input.x, input.y, input.z);
}

vec3 fromGlm(glm::vec3 input) {
    vec3 value;
    value.x = input.x;
    value.y = input.y;
    value.z = input.z;
    return value;
}

PhysicalModelImpl::PhysicalModelImpl() {
    mPhysicalModelInterface.opaque = static_cast<void*>(this);
}

PhysicalModel* PhysicalModelImpl::getPhysicalModel() {
    return &mPhysicalModelInterface;
}

PhysicalModelImpl* PhysicalModelImpl::getImpl(PhysicalModel* physicalModel) {
    return physicalModel != nullptr ?
            static_cast<PhysicalModelImpl*>(physicalModel->opaque) :
            nullptr;
}

void PhysicalModelImpl::setTargetPosition(vec3 position, bool instantaneous) {
    physicalStateChanging();
    mInertialModel.setTargetPosition(toGlm(position), instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetRotation(vec3 rotation, bool instantaneous) {
    physicalStateChanging();
    mInertialModel.setTargetRotation(glm::quat(toGlm(rotation)), instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setMagneticField(
        float north, float east, float vertical, bool instantaneous) {
    physicalStateChanging();
    mAmbientEnvironment.setMagneticField(
        north, east, vertical, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

vec3 PhysicalModelImpl::getPosition() const {
    return fromGlm(mInertialModel.getPosition());
}

vec3 PhysicalModelImpl::getRotation() const {
    return fromGlm(glm::eulerAngles(mInertialModel.getRotation()));
}

#define GET_FUNCTION_NAME(x) get##x
#define OVERRIDE_FUNCTION_NAME(x) override##x
#define OVERRIDE_NAME(x) m##x##Override
#define SENSOR_NAME(x) PHYSICAL_SENSOR_##x
#define PHYSICAL_NAME(x) getPhysical##x

// Implement sensor overrides.
#define SENSOR_(x,y,z) \
void PhysicalModelImpl::OVERRIDE_FUNCTION_NAME(y)(z override_value) {\
    physicalStateChanging();\
    mUseOverride[SENSOR_NAME(x)] = true;\
    OVERRIDE_NAME(y) = override_value;\
    physicalStateStabilized();\
}
PHYSICAL_SENSORS_LIST
#undef SENSOR_

// Implement getters that respect overrides.
#define SENSOR_(x,y,z) \
z PhysicalModelImpl::GET_FUNCTION_NAME(y)() const {\
    return mUseOverride[SENSOR_NAME(x)] ?\
            OVERRIDE_NAME(y) : PHYSICAL_NAME(y)();\
}

PHYSICAL_SENSORS_LIST

#undef SENSOR_

#undef PHYSICAL_NAME
#undef SENSOR_NAME
#undef OVERRIDE_NAME
#undef OVERRIDE_FUNCTION_NAME
#undef GET_FUNCTION_NAME

vec3 PhysicalModelImpl::getPhysicalAccelerometer() const {
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
    return fromGlm(glm::conjugate(mInertialModel.getRotation()) *
        (mInertialModel.getAcceleration() - mAmbientEnvironment.getGravity()));
}

vec3 PhysicalModelImpl::getPhysicalGyroscope() const {
    return fromGlm(glm::conjugate(mInertialModel.getRotation()) *
        mInertialModel.getRotationalVelocity());
}

vec3 PhysicalModelImpl::getPhysicalMagnetometer() const {
    return fromGlm(glm::conjugate(mInertialModel.getRotation()) *
        mAmbientEnvironment.getMagneticField());
}

/* (x, y, z) == (azimuth, pitch, roll) */
vec3 PhysicalModelImpl::getPhysicalOrientation() const {
    // TODO: implement
    return fromGlm(glm::vec3(0.f));
}

/* celsius */
float PhysicalModelImpl::getPhysicalTemperature() const {
    // TODO: implement
    return 0.f;
}

float PhysicalModelImpl::getPhysicalProximity() const {
    // TODO: implement
    return 0.f;
}

float PhysicalModelImpl::getPhysicalLight() const {
    // TODO: implement
    return 0.f;
}

float PhysicalModelImpl::getPhysicalPressure() const {
    // TODO: implement
    return 0.f;
}

float PhysicalModelImpl::getPhysicalHumidity() const {
    // TODO: implement
    return 0.f;
}

vec3 PhysicalModelImpl::getPhysicalMagnetometerUncalibrated() const {
    // TODO: implement
    return fromGlm(glm::vec3(0.f));
}

vec3 PhysicalModelImpl::getPhysicalGyroscopeUncalibrated() const {
    // TODO: implement
    return fromGlm(glm::vec3(0.f));
}

void PhysicalModelImpl::setPhysicalStateAgent(
        const QAndroidPhysicalStateAgent* agent) {
    // TODO(tpsiaki) make sure that this is up to date with the current state -
    // i.e. if physical state is currently changing, set physicalStateChanging.
    // Maybe send target state changed immediately too?
    mAgent = agent;
}

void PhysicalModelImpl::physicalStateChanging() {
    if (nullptr != mAgent) {
        mAgent->onPhysicalStateChanging(mAgent->context);
    }
}

void PhysicalModelImpl::physicalStateStabilized() {
    if (nullptr != mAgent) {
        mAgent->onPhysicalStateStabilized(mAgent->context);
    }
}

void PhysicalModelImpl::targetStateChanged() {
    /* when target state changes we reset all sensor overrides */
    for (int i = 0; i < PHYSICAL_SENSORS_MAX; i++) {
        mUseOverride[i] = false;
    }
    if (nullptr != mAgent) {
        mAgent->onTargetStateChanged(mAgent->context);
    }
}

}  // namespace physics
}  // namespace android

using android::physics::PhysicalModelImpl;

PhysicalModel* _physicalModel_new() {
    PhysicalModelImpl* impl = new PhysicalModelImpl();
    return impl != nullptr ? impl->getPhysicalModel() : nullptr;
}

void _physicalModel_free(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        delete impl;
    }
}

void _physicalModel_setTargetPosition(PhysicalModel* model,
        vec3 position, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetPosition(position, instantaneous);
    }
}

void _physicalModel_setTargetRotation(PhysicalModel* model,
        vec3 rotation, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetRotation(rotation, instantaneous);
    }
}

void _physicalModel_setMagneticField(PhysicalModel* model,
        float north, float east, float vertical, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setMagneticField(north, east, vertical, instantaneous);
    }
}

vec3 _physicalModel_getPosition(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    vec3 result;
    if (impl != nullptr) {
        result = impl->getPosition();
    } else {
        result.x = 0.f;
        result.y = 0.f;
        result.z = 0.f;
    }
    return result;
}

vec3 _physicalModel_getRotation(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    vec3 result;
    if (impl != nullptr) {
        result = impl->getRotation();
    } else {
        result.x = 0.f;
        result.y = 0.f;
        result.z = 0.f;
    }
    return result;
}

#define OVERRIDE_FUNCTION_NAME(x) override##x
#define PHYSICAL_OVERRIDE_FUNCTION_NAME(x) _physicalModel_override##x
#define SENSOR_(x,y,z) void PHYSICAL_OVERRIDE_FUNCTION_NAME(y)(\
        PhysicalModel* model,\
        z override_value) {\
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);\
    if (impl != nullptr) {\
        impl->OVERRIDE_FUNCTION_NAME(y)(override_value);\
    }\
}
PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef PHYSICAL_OVERRIDE_FUNCTION_NAME
#undef OVERRIDE_FUNCTION_NAME

#define GET_FUNCTION_NAME(x) get##x
#define PHYSICAL_GET_FUNCTION_NAME(x) _physicalModel_get##x
#define SENSOR_(x,y,z) z PHYSICAL_GET_FUNCTION_NAME(y)(PhysicalModel* model) {\
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);\
    return impl != nullptr ?\
            impl->GET_FUNCTION_NAME(y)() :\
            z{0.f};\
}
PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef PHYSICAL_GET_FUNCTION_NAME
#undef GET_FUNCTION_NAME

void _physicalModel_setPhysicalStateAgent(PhysicalModel* model,
        const QAndroidPhysicalStateAgent* agent) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setPhysicalStateAgent(agent);
    }
}

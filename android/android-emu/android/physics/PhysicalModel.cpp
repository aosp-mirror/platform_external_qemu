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

#include "android/base/synchronization/Lock.h"
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
     * is moving, in order to simulate a magnetic vector where x is north,
     * y is east, and z is vertical
     */
    void setTargetMagneticField(vec3 field, bool instantaneous);

    /*
     * Sets the ambient temperature in the modeled environment in order to
     * simulate a temperature sensor.
     */
    void setTargetTemperature(float celsius, bool instantaneous);

    /*
     * Sets the target proximity value in order to drive a model of a
     * proximity sensor.
     */
    void setTargetProximity(float centimeters, bool instantaneous);

    /*
     * Sets the ambient light value in the environment in order to drive a
     * modeled ambient light sensor.
     */
    void setTargetLight(float lux, bool instantaneous);

    /*
     * Set the ambient barometric pressure in the environment in order to
     * drive a modeled barometer.
     */
    void setTargetPressure(float hPa, bool instantaneous);

    /*
     * Set the ambient relative humidity in the environment in order to drive
     * a modeled hygrometer.
     */
    void setTargetHumidity(float percentage, bool instantaneous);
    
    /*
     * Gets current target state of the modeled object.
     */
    vec3 getTargetPosition() const;
    vec3 getTargetRotation() const;
    vec3 getTargetMagneticField() const;
    float getTargetTemperature() const;
    float getTargetProximity() const;
    float getTargetLight() const;
    float getTargetPressure() const;
    float getTargetHumidity() const;

    /*
     * Sets the current override states for each simulated sensor.
     */
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

    android::base::Lock mAgentMutex;
    const QAndroidPhysicalStateAgent* mAgent = nullptr;
    bool mIsPhysicalStateChanging = false;

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
    mInertialModel.setTargetRotation(glm::quat(glm::radians(toGlm(rotation))), instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetMagneticField(
        vec3 field, bool instantaneous) {
    physicalStateChanging();
    mAmbientEnvironment.setMagneticField(
        field.x, field.y, field.z, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetTemperature(float celsius, bool instantaneous) {
    physicalStateChanging();
    mAmbientEnvironment.setTemperature(celsius, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetProximity(float centimeters, bool instantaneous) {
    physicalStateChanging();
    mAmbientEnvironment.setProximity(centimeters, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetLight(float lux, bool instantaneous) {
    physicalStateChanging();
    mAmbientEnvironment.setLight(lux, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetPressure(float hPa, bool instantaneous) {
    physicalStateChanging();
    mAmbientEnvironment.setPressure(hPa, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetHumidity(float percentage, bool instantaneous) {
    physicalStateChanging();
    mAmbientEnvironment.setHumidity(percentage, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

vec3 PhysicalModelImpl::getTargetPosition() const {
    return fromGlm(mInertialModel.getPosition());
}

vec3 PhysicalModelImpl::getTargetRotation() const {
    return fromGlm(glm::degrees(glm::eulerAngles(
            mInertialModel.getRotation())));
}

vec3 PhysicalModelImpl::getTargetMagneticField() const {
    return fromGlm(mAmbientEnvironment.getMagneticField());
}

float PhysicalModelImpl::getTargetTemperature() const {
    return mAmbientEnvironment.getTemperature();
}

float PhysicalModelImpl::getTargetProximity() const {
    return mAmbientEnvironment.getProximity();
}

float PhysicalModelImpl::getTargetLight() const {
    return mAmbientEnvironment.getLight();
}

float PhysicalModelImpl::getTargetPressure() const {
    return mAmbientEnvironment.getPressure();
}

float PhysicalModelImpl::getTargetHumidity() const {
    return mAmbientEnvironment.getHumidity();
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
    return fromGlm(glm::eulerAngles(mInertialModel.getRotation()));
}

float PhysicalModelImpl::getPhysicalTemperature() const {
    return mAmbientEnvironment.getTemperature();
}

float PhysicalModelImpl::getPhysicalProximity() const {
    return mAmbientEnvironment.getProximity();
}

float PhysicalModelImpl::getPhysicalLight() const {
    return mAmbientEnvironment.getLight();
}

float PhysicalModelImpl::getPhysicalPressure() const {
    return mAmbientEnvironment.getPressure();
}

float PhysicalModelImpl::getPhysicalHumidity() const {
    return mAmbientEnvironment.getHumidity();
}

vec3 PhysicalModelImpl::getPhysicalMagnetometerUncalibrated() const {
    return fromGlm(glm::conjugate(mInertialModel.getRotation()) *
        mAmbientEnvironment.getMagneticField());
}

vec3 PhysicalModelImpl::getPhysicalGyroscopeUncalibrated() const {
    return fromGlm(glm::conjugate(mInertialModel.getRotation()) *
        mInertialModel.getRotationalVelocity());
}

void PhysicalModelImpl::setPhysicalStateAgent(
        const QAndroidPhysicalStateAgent* agent) {
    base::AutoLock lock(mAgentMutex);
    // Ensure that each agent gets matching changing/stabilized callbacks.
    if (mAgent != nullptr && mIsPhysicalStateChanging) {
        mAgent->onPhysicalStateStabilized(mAgent->context);
    }
    mAgent = agent;
    if (mAgent != nullptr) {
        if (mIsPhysicalStateChanging) {
            // Ensure the new agent is set correctly if the is a pending state
            // change.
            mAgent->onPhysicalStateChanging(mAgent->context);
        } else {
            // If no state change is pending, we still send a change/stabilize
            // message so that agents can depend on them for initialization.
            mAgent->onPhysicalStateChanging(mAgent->context);
            mAgent->onPhysicalStateStabilized(mAgent->context);
        }
        // We send an initial target state change so agents can depend on this
        // for initialization.
        mAgent->onTargetStateChanged(mAgent->context);
    }
}

void PhysicalModelImpl::physicalStateChanging() {
    android::base::AutoLock lock(mAgentMutex);
    assert(!mIsPhysicalStateChanging);
    if (nullptr != mAgent) {
        mAgent->onPhysicalStateChanging(mAgent->context);
    }
    mIsPhysicalStateChanging = true;
}

void PhysicalModelImpl::physicalStateStabilized() {
    android::base::AutoLock lock(mAgentMutex);
    assert(mIsPhysicalStateChanging);
    if (nullptr != mAgent) {
        mAgent->onPhysicalStateStabilized(mAgent->context);
    }
    mIsPhysicalStateChanging = false;
}

void PhysicalModelImpl::targetStateChanged() {
    android::base::AutoLock lock(mAgentMutex);
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

void _physicalModel_setTargetMagneticField(PhysicalModel* model,
        vec3 field, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetMagneticField(field, instantaneous);
    }
}


void _physicalModel_setTargetTemperature(struct PhysicalModel* model,
        float celsius, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetTemperature(celsius, instantaneous);
    }
}

void _physicalModel_setTargetProximity(struct PhysicalModel* model,
        float centimeters, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetProximity(centimeters, instantaneous);
    }
}

void _physicalModel_setTargetLight(struct PhysicalModel* model,
        float lux, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetLight(lux, instantaneous);
    }
}

void _physicalModel_setTargetPressure(struct PhysicalModel* model,
        float hPa, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetPressure(hPa, instantaneous);
    }
}

void _physicalModel_setTargetHumidity(struct PhysicalModel* model,
        float percentage, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetHumidity(percentage, instantaneous);
    }
}

vec3 _physicalModel_getTargetPosition(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    vec3 result;
    if (impl != nullptr) {
        result = impl->getTargetPosition();
    } else {
        result.x = 0.f;
        result.y = 0.f;
        result.z = 0.f;
    }
    return result;
}

vec3 _physicalModel_getTargetRotation(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    vec3 result;
    if (impl != nullptr) {
        result = impl->getTargetRotation();
    } else {
        result.x = 0.f;
        result.y = 0.f;
        result.z = 0.f;
    }
    return result;
}

vec3 _physicalModel_getTargetMagneticField(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    vec3 result;
    if (impl != nullptr) {
        result = impl->getTargetMagneticField();
    } else {
        result.x = 0.f;
        result.y = 0.f;
        result.z = 0.f;
    }
    return result;
}

float _physicalModel_getTargetTemperature(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetTemperature();
    } else {
        result = 0.f;
    }
    return result;
}

float _physicalModel_getTargetProximity(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetProximity();
    } else {
        result = 0.f;
    }
    return result;
}

float _physicalModel_getTargetLight(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetLight();
    } else {
        result = 0.f;
    }
    return result;
}

float _physicalModel_getTargetPressure(struct PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetPressure();
    } else {
        result = 0.f;
    }
    return result;
}

float _physicalModel_getTargetHumidity(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetHumidity();
    } else {
        result = 0.f;
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

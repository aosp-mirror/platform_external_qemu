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
#include "android/utils/debug.h"
#include "android/utils/stream.h"

#include <mutex>

#define D(...) VERBOSE_PRINT(sensors, __VA_ARGS__)

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
    ~PhysicalModelImpl();

    /*
     * Gets the PhysicalModel interface for this object.
     * Can be called from any thread.
     */
    PhysicalModel* getPhysicalModel();

    static PhysicalModelImpl* getImpl(PhysicalModel* physicalModel);

    /*
     * Sets the position that the modeled object should move toward.
     * Can be called from any thread.
     */
    void setTargetPosition(vec3 position, bool instantaneous);

    /*
     * Sets the rotation that the modeled object should move toward.
     * Can be called from any thread.
     */
    void setTargetRotation(vec3 rotation, bool instantaneous);

    /*
     * Sets the strength of the magnetic field in which the modeled object
     * is moving, in order to simulate a magnetic vector where x is north,
     * y is east, and z is vertical.
     * Can be called from any thread.
     */
    void setTargetMagneticField(vec3 field, bool instantaneous);

    /*
     * Sets the ambient temperature in the modeled environment in order to
     * simulate a temperature sensor.
     * Can be called from any thread.
     */
    void setTargetTemperature(float celsius, bool instantaneous);

    /*
     * Sets the target proximity value in order to drive a model of a
     * proximity sensor.
     * Can be called from any thread.
     */
    void setTargetProximity(float centimeters, bool instantaneous);

    /*
     * Sets the ambient light value in the environment in order to drive a
     * modeled ambient light sensor.
     * Can be called from any thread.
     */
    void setTargetLight(float lux, bool instantaneous);

    /*
     * Set the ambient barometric pressure in the environment in order to
     * drive a modeled barometer.
     * Can be called from any thread.
     */
    void setTargetPressure(float hPa, bool instantaneous);

    /*
     * Set the ambient relative humidity in the environment in order to drive
     * a modeled hygrometer.
     * Can be called from any thread.
     */
    void setTargetHumidity(float percentage, bool instantaneous);
    
    /*
     * Gets current target state of the modeled object.
     * Can be called from any thread.
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
     * Can be called from any thread.
     */
#define OVERRIDE_FUNCTION_NAME(x) override##x
#define SENSOR_(x,y,z) void OVERRIDE_FUNCTION_NAME(y)(z override_value);
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_FUNCTION_NAME

    /*
     * Getters for all sensor values.
     * Can be called from any thread.
     */
#define GET_FUNCTION_NAME(x) get##x
#define SENSOR_(x,y,z) z GET_FUNCTION_NAME(y)(\
        long* measurement_id) const;
    PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef GET_FUNCTION_NAME

    /*
     * Set or unset callbacks used to signal changing state.
     * Can be called from any thread.
     */
    void setPhysicalStateAgent(const QAndroidPhysicalStateAgent* agent);
    
    /*
     * Save the current physical state to the given stream.
     */
    void save(Stream* f);
    
    /*
     * Load the current physical state from the given stream.
     */
    int load(Stream* f);
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
    
    mutable std::recursive_mutex mMutex;

    InertialModel mInertialModel;
    AmbientEnvironment mAmbientEnvironment;

    const QAndroidPhysicalStateAgent* mAgent = nullptr;
    bool mIsPhysicalStateChanging = false;

    bool mUseOverride[PHYSICAL_SENSORS_MAX] = {false};
    mutable long mMeasurementId[PHYSICAL_SENSORS_MAX] = {0};
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

PhysicalModelImpl::~PhysicalModelImpl() {
    assert(mAgent == nullptr);
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
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mInertialModel.setTargetPosition(toGlm(position), instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetRotation(vec3 rotation, bool instantaneous) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mInertialModel.setTargetRotation(
            glm::quat(glm::radians(toGlm(rotation))), instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetMagneticField(
        vec3 field, bool instantaneous) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setMagneticField(
        field.x, field.y, field.z, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetTemperature(
        float celsius, bool instantaneous) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setTemperature(celsius, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetProximity(
        float centimeters, bool instantaneous) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setProximity(centimeters, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetLight(float lux, bool instantaneous) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setLight(lux, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetPressure(float hPa, bool instantaneous) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setPressure(hPa, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

void PhysicalModelImpl::setTargetHumidity(
        float percentage, bool instantaneous) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setHumidity(percentage, instantaneous);
    targetStateChanged();
    physicalStateStabilized();
}

vec3 PhysicalModelImpl::getTargetPosition() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return fromGlm(mInertialModel.getPosition());
}

vec3 PhysicalModelImpl::getTargetRotation() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return fromGlm(glm::degrees(glm::eulerAngles(
            mInertialModel.getRotation())));
}

vec3 PhysicalModelImpl::getTargetMagneticField() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return fromGlm(mAmbientEnvironment.getMagneticField());
}

float PhysicalModelImpl::getTargetTemperature() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getTemperature();
}

float PhysicalModelImpl::getTargetProximity() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getProximity();
}

float PhysicalModelImpl::getTargetLight() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getLight();
}

float PhysicalModelImpl::getTargetPressure() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getPressure();
}

float PhysicalModelImpl::getTargetHumidity() const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
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
    std::lock_guard<std::recursive_mutex> lock(mMutex);\
    physicalStateChanging();\
    mUseOverride[SENSOR_NAME(x)] = true;\
    mMeasurementId[SENSOR_NAME(x)]++;\
    OVERRIDE_NAME(y) = override_value;\
    physicalStateStabilized();\
}
PHYSICAL_SENSORS_LIST
#undef SENSOR_

// Implement getters that respect overrides.
#define SENSOR_(x,y,z) \
z PhysicalModelImpl::GET_FUNCTION_NAME(y)(\
        long* measurement_id) const {\
    std::lock_guard<std::recursive_mutex> lock(mMutex);\
    if (mUseOverride[SENSOR_NAME(x)]) {\
        *measurement_id = mMeasurementId[SENSOR_NAME(x)];\
        return OVERRIDE_NAME(y);\
    } else {\
        if (mIsPhysicalStateChanging) {\
            mMeasurementId[SENSOR_NAME(x)]++;\
        }\
        *measurement_id = mMeasurementId[SENSOR_NAME(x)];\
        return PHYSICAL_NAME(y)();\
    }\
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
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    
    if (mAgent != nullptr) {
        if (mAgent->onPhysicalStateStabilized != nullptr) {
            mAgent->onPhysicalStateStabilized(mAgent->context);
        }
    }
    mAgent = agent;
    if (mAgent != nullptr) {
        if (mIsPhysicalStateChanging) {
            // Ensure the new agent is set correctly if the is a pending state
            // change.
            if (mAgent->onPhysicalStateChanging != nullptr) {
                mAgent->onPhysicalStateChanging(mAgent->context);
            }
        } else {
            // If no state change is pending, we still send a change/stabilize
            // message so that agents can depend on them for initialization.
            if (mAgent->onPhysicalStateChanging != nullptr) {
                mAgent->onPhysicalStateChanging(mAgent->context);
            }
            if (mAgent->onPhysicalStateStabilized != nullptr) {
                mAgent->onPhysicalStateStabilized(mAgent->context);
            }
        }
        // We send an initial target state change so agents can depend on this
        // for initialization.
        if (mAgent->onTargetStateChanged != nullptr) {
            mAgent->onTargetStateChanged(mAgent->context);
        }
    }
}

void readValueFromStream(Stream* f, vec3* value) {
    value->x = stream_get_float(f);
    value->y = stream_get_float(f);
    value->z = stream_get_float(f);
}

void readValueFromStream(Stream* f, float* value) {
    *value = stream_get_float(f);
}

void writeValueToStream(Stream* f, vec3 value) {
    stream_put_float(f, value.x);
    stream_put_float(f, value.y);
    stream_put_float(f, value.z);
}

void writeValueToStream(Stream* f, float value) {
    stream_put_float(f, value);
}
    

void PhysicalModelImpl::save(Stream* f) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    
    // first save targets
    stream_put_be32(f, MAX_PHYSICAL_PARAMETERS);
    
    for (int parameter = 0;
         parameter < MAX_PHYSICAL_PARAMETERS;
         parameter++) {
        switch (parameter) {
            case PHYSICAL_PARAMETER_POSITION: {
                vec3 targetPosition = getTargetPosition();
                stream_put_float(f, targetPosition.x);
                stream_put_float(f, targetPosition.y);
                stream_put_float(f, targetPosition.z);
                break;
            }
            case PHYSICAL_PARAMETER_ROTATION: {
                vec3 targetRotation = getTargetRotation();
                stream_put_float(f, targetRotation.x);
                stream_put_float(f, targetRotation.y);
                stream_put_float(f, targetRotation.z);
                break;
            }
            case PHYSICAL_PARAMETER_MAGNETIC_FIELD: {
                vec3 magneticField = getTargetMagneticField();
                stream_put_float(f, magneticField.x);
                stream_put_float(f, magneticField.y);
                stream_put_float(f, magneticField.z);
                break;
            }
            case PHYSICAL_PARAMETER_TEMPERATURE:
                stream_put_float(f, getTargetTemperature());
                break;
            case PHYSICAL_PARAMETER_PROXIMITY:
                stream_put_float(f, getTargetProximity());
                break;
            case PHYSICAL_PARAMETER_LIGHT:
                stream_put_float(f, getTargetLight());
                break;
            case PHYSICAL_PARAMETER_PRESSURE:
                stream_put_float(f, getTargetPressure());
                break;
            case PHYSICAL_PARAMETER_HUMIDITY:
                stream_put_float(f, getTargetHumidity());
                break;
        }
    }
    
    // then save overrides
    stream_put_be32(f, PHYSICAL_SENSORS_MAX);
    
    for (int sensor = 0;
         sensor < PHYSICAL_SENSORS_MAX;
         sensor++) {
        stream_put_be32(f, mUseOverride[sensor]);
        if (mUseOverride) {
            switch(sensor) {
#define PHYSICAL_SENSOR_NAME(x) PHYSICAL_SENSOR_##x
#define OVERRIDE_NAME(x) m##x##Override
#define SENSOR_(x, y, z)\
                case PHYSICAL_SENSOR_NAME(x): {\
                    writeValueToStream(f, OVERRIDE_NAME(y));\
                    break;\
                }
PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_NAME
#undef PHYSICAL_SENSOR_NAME
            }
        }
    }
}

int PhysicalModelImpl::load(Stream* f) {
    // first load targets
    int32_t num_physical_parameters = stream_get_be32(f);
    if (num_physical_parameters > MAX_PHYSICAL_PARAMETERS) {
        D("%s: cannot load: snapshot requires %d physical parameters, %d available\n",
          __FUNCTION__, num_physical_parameters, MAX_PHYSICAL_PARAMETERS);
        return -EIO;
    }
    
    // Note: any new target params will remain at their defaults.
    
    for (int parameter = 0;
         parameter < num_physical_parameters;
         parameter++) {

        switch (parameter) {
            case PHYSICAL_PARAMETER_POSITION: {
                vec3 targetPosition;
                targetPosition.x = stream_get_float(f);
                targetPosition.y = stream_get_float(f);
                targetPosition.z = stream_get_float(f);
                setTargetPosition(targetPosition, true);
                break;
            }
            case PHYSICAL_PARAMETER_ROTATION: {
                vec3 targetRotation;
                targetRotation.x = stream_get_float(f);
                targetRotation.y = stream_get_float(f);
                targetRotation.z = stream_get_float(f);
                setTargetRotation(targetRotation, true);
                break;
            }
            case PHYSICAL_PARAMETER_MAGNETIC_FIELD: {
                vec3 targetMagneticField;
                targetMagneticField.x = stream_get_float(f);
                targetMagneticField.y = stream_get_float(f);
                targetMagneticField.z = stream_get_float(f);
                setTargetMagneticField(targetMagneticField, true);
                break;
            }
            case PHYSICAL_PARAMETER_TEMPERATURE:
                setTargetTemperature(stream_get_float(f), true);
                break;
            case PHYSICAL_PARAMETER_PROXIMITY:
                setTargetProximity(stream_get_float(f), true);
                break;
            case PHYSICAL_PARAMETER_LIGHT:
                setTargetLight(stream_get_float(f), true);
                break;
            case PHYSICAL_PARAMETER_PRESSURE:
                setTargetPressure(stream_get_float(f), true);
                break;
            case PHYSICAL_PARAMETER_HUMIDITY:
                setTargetHumidity(stream_get_float(f), true);
                break;
        }
    }
    
    // then load overrides

    /* check number of physical sensors */
    int32_t num_sensors = stream_get_be32(f);
    if (num_sensors > PHYSICAL_SENSORS_MAX) {
        D("%s: cannot load: snapshot requires %d physical sensors, %d available\n",
          __FUNCTION__, num_sensors, PHYSICAL_SENSORS_MAX);
        return -EIO;
    }
    
    for (int sensor = 0;
         sensor < num_sensors;
         sensor++) {
        if (stream_get_be32(f)) {
            switch(sensor) {
#define PHYSICAL_SENSOR_NAME(x) PHYSICAL_SENSOR_##x
#define OVERRIDE_FUNCTION_NAME(x) override##x
#define SENSOR_(x, y, z)\
                case PHYSICAL_SENSOR_NAME(x): {\
                    z value;\
                    readValueFromStream(f, &value);\
                    OVERRIDE_FUNCTION_NAME(y)(value);\
                    break;\
                }
PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_FUNCTION_NAME
#undef PHYSICAL_SENSOR_NAME
            }
        }
    }
    
    return 0;
}

void PhysicalModelImpl::physicalStateChanging() {
    assert(!mIsPhysicalStateChanging);
    if (mAgent != nullptr && mAgent->onPhysicalStateChanging != nullptr) {
        mAgent->onPhysicalStateChanging(mAgent->context);
    }
    mIsPhysicalStateChanging = true;
}

void PhysicalModelImpl::physicalStateStabilized() {
    assert(mIsPhysicalStateChanging);
    if (mAgent != nullptr && mAgent->onPhysicalStateStabilized != nullptr) {
        mAgent->onPhysicalStateStabilized(mAgent->context);
    }
    // Increment all of the measurement ids because the physical state has
    // stabilized.
    for (int i = 0; i < PHYSICAL_SENSORS_MAX; i++) {
        mMeasurementId[i]++;
    }
    mIsPhysicalStateChanging = false;
}

void PhysicalModelImpl::targetStateChanged() {
    /* when target state changes we reset all sensor overrides */
    for (int i = 0; i < PHYSICAL_SENSORS_MAX; i++) {
        mUseOverride[i] = false;
    }
    if (mAgent != nullptr && mAgent->onTargetStateChanged != nullptr) {
        mAgent->onTargetStateChanged(mAgent->context);
    }
}

}  // namespace physics
}  // namespace android

using android::physics::PhysicalModelImpl;

PhysicalModel* physicalModel_new() {
    PhysicalModelImpl* impl = new PhysicalModelImpl();
    return impl != nullptr ? impl->getPhysicalModel() : nullptr;
}

void physicalModel_free(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        delete impl;
    }
}

void physicalModel_setTargetPosition(PhysicalModel* model,
        vec3 position, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetPosition(position, instantaneous);
    }
}

void physicalModel_setTargetRotation(PhysicalModel* model,
        vec3 rotation, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetRotation(rotation, instantaneous);
    }
}

void physicalModel_setTargetMagneticField(PhysicalModel* model,
        vec3 field, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetMagneticField(field, instantaneous);
    }
}


void physicalModel_setTargetTemperature(struct PhysicalModel* model,
        float celsius, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetTemperature(celsius, instantaneous);
    }
}

void physicalModel_setTargetProximity(struct PhysicalModel* model,
        float centimeters, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetProximity(centimeters, instantaneous);
    }
}

void physicalModel_setTargetLight(struct PhysicalModel* model,
        float lux, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetLight(lux, instantaneous);
    }
}

void physicalModel_setTargetPressure(struct PhysicalModel* model,
        float hPa, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetPressure(hPa, instantaneous);
    }
}

void physicalModel_setTargetHumidity(struct PhysicalModel* model,
        float percentage, bool instantaneous) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setTargetHumidity(percentage, instantaneous);
    }
}

vec3 physicalModel_getTargetPosition(PhysicalModel* model) {
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

vec3 physicalModel_getTargetRotation(PhysicalModel* model) {
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

vec3 physicalModel_getTargetMagneticField(PhysicalModel* model) {
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

float physicalModel_getTargetTemperature(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetTemperature();
    } else {
        result = 0.f;
    }
    return result;
}

float physicalModel_getTargetProximity(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetProximity();
    } else {
        result = 0.f;
    }
    return result;
}

float physicalModel_getTargetLight(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetLight();
    } else {
        result = 0.f;
    }
    return result;
}

float physicalModel_getTargetPressure(struct PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    float result;
    if (impl != nullptr) {
        result = impl->getTargetPressure();
    } else {
        result = 0.f;
    }
    return result;
}

float physicalModel_getTargetHumidity(PhysicalModel* model) {
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
#define PHYSICAL_OVERRIDE_FUNCTION_NAME(x) physicalModel_override##x
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
#define PHYSICAL_GET_FUNCTION_NAME(x) physicalModel_get##x
#define SENSOR_(x,y,z) z PHYSICAL_GET_FUNCTION_NAME(y)(\
        PhysicalModel* model, long* measurement_id) {\
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);\
    *measurement_id = 0L;\
    return impl != nullptr ?\
            impl->GET_FUNCTION_NAME(y)(measurement_id) :\
            z{0.f};\
}
PHYSICAL_SENSORS_LIST
#undef SENSOR_
#undef PHYSICAL_GET_FUNCTION_NAME
#undef GET_FUNCTION_NAME

void physicalModel_setPhysicalStateAgent(PhysicalModel* model,
        const QAndroidPhysicalStateAgent* agent) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setPhysicalStateAgent(agent);
    }
}

void physicalModel_save(PhysicalModel* model, Stream* f) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->save(f);
    }
}

int physicalModel_load(PhysicalModel* model, Stream* f) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        return impl->load(f);
    } else {
        return -EIO;
    }
}

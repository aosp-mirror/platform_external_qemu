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
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/vec3.hpp>

#include "android/automation/AutomationEventSink.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/StdioStream.h"
#include "android/base/system/System.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/hw-sensors.h"
#include "android/physics/AmbientEnvironment.h"
#include "android/physics/InertialModel.h"
#include "android/utils/debug.h"
#include "android/utils/looper.h"
#include "android/utils/stream.h"

#include <cstdio>
#include <mutex>

#define D(...) VERBOSE_PRINT(sensors, __VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define E(...) derror(__VA_ARGS__)

using android::base::PathUtils;
using android::base::StdioStream;
using android::base::System;

namespace pb = emulator_automation;

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
    PhysicalModelImpl(bool shouldTick);
    ~PhysicalModelImpl();

    /*
     * Gets the PhysicalModel interface for this object.
     * Can be called from any thread.
     */
    PhysicalModel* getPhysicalModel();

    static PhysicalModelImpl* getImpl(PhysicalModel* physicalModel);

    /*
     * Sets the current time of the PhysicalModel simulation.  This time is
     * used as the current time in calculating all sensor values, along with
     * the time when target parameter change requests are recorded as taking
     * place.  Time values must be non-decreasing.
     *
     * Returns whether the physical state is stable or changing at the set time.
     */
    void setCurrentTime(int64_t time_ns);

    /*
     * Sets the target value for the given physical parameter that the physical
     * model should move towards and records the result.
     * Can be called from any thread.
     */
#define SET_TARGET_FUNCTION_NAME(x) setTarget##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
    void SET_TARGET_FUNCTION_NAME(z)(\
        w value, PhysicalInterpolation mode);

    PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef SET_TARGET_FUNCTION_NAME

    /*
     * Gets current target state of the modeled object.
     * Can be called from any thread.
     */
#define GET_TARGET_FUNCTION_NAME(x) getParameter##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
    w GET_TARGET_FUNCTION_NAME(z)(ParameterValueType parameterValueType) const;

    PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef GET_TARGET_FUNCTION_NAME

    /*
     * Sets the current override states for each simulated sensor.
     * Can be called from any thread.
     */
#define OVERRIDE_FUNCTION_NAME(x) override##x
#define SENSOR_(x,y,z,v,w) \
    void OVERRIDE_FUNCTION_NAME(z)(v override_value);

    SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_FUNCTION_NAME

    /*
     * Getters for all sensor values.
     * Can be called from any thread.
     */
#define GET_FUNCTION_NAME(x) get##x
#define SENSOR_(x,y,z,v,w) \
    v GET_FUNCTION_NAME(z)(long* measurement_id) const;

    SENSORS_LIST
#undef SENSOR_
#undef GET_FUNCTION_NAME

    /* Get the physical rotation and translation simulatenousely. */
    void getTransform(
            float* out_translation_x, float* out_translation_y, float* out_translation_z,
            float* out_rotation_x, float* out_rotation_y, float* out_rotation_z,
            int64_t* out_timestamp);

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

    /*
     * Start recording physical model changes to the given file.
     * Returns 0 if successful.
     */
    int record(const char* filename);

    /*
     * Start recording physical model ground truth to the given file.
     * Returns 0 if successful.
     */
    int recordGroundTruth(const char* filename);

    /*
     * Play a recording from the given file.
     * Returns 0 if successful.
     */
    int playback(const char* filename);

    /*
     * Stop the current recording or playback of physical state changes.
     */
    void stopRecordAndPlayback();

    /*
     * Stop recording the ground truth.
     */
    void stopRecordGroundTruth();

private:

    void setCurrentTimeInternal(int64_t time_ns);

    /*
     * Sets the target value for the given physical parameter that the physical
     * model should move towards.
     * Can be called from any thread.
     */
#define SET_TARGET_INTERNAL_FUNCTION_NAME(x) setTargetInternal##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
    void SET_TARGET_INTERNAL_FUNCTION_NAME(z)(w value, \
                                              PhysicalInterpolation mode);

    PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef SET_TARGET_INTERNAL_FUNCTION_NAME

    /*
     * Getters for non-overridden physical sensor values.
     */
#define GET_PHYSICAL_NAME(x) getPhysical##x
#define SENSOR_(x,y,z,v,w) \
    v GET_PHYSICAL_NAME(z)() const;

    SENSORS_LIST
#undef SENSOR_
#undef GET_PHYSICAL_NAME

    /*
     * Helper for setting overrides.
     */
    template<class T>
    void setOverride(AndroidSensor sensor,
                     T* overrideMemberPointer,
                     T overrideValue) {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        physicalStateChanging();
        mUseOverride[sensor] = true;
        mMeasurementId[sensor]++;
        *overrideMemberPointer = overrideValue;
    }

    /*
     * Helper for getting current sensor values.
     */
    template<class T>
    T getSensorValue(AndroidSensor sensor,
                     const T* overrideMemberPointer,
                     std::function<T()> physicalGetter,
                     long* measurement_id) const {
        std::lock_guard<std::recursive_mutex> lock(mMutex);
        if (mUseOverride[sensor]) {
            *measurement_id = mMeasurementId[sensor];
            return *overrideMemberPointer;
        } else {
            if (mIsPhysicalStateChanging) {
                mMeasurementId[sensor]++;
            }
            *measurement_id = mMeasurementId[sensor];
            return physicalGetter();
        }
    }

    void physicalStateChanging();
    void physicalStateStabilized();
    void targetStateChanged();

    template <typename ValueType>
    void generateEvent(PhysicalParameter type,
                       PhysicalInterpolation mode,
                       ValueType value);

    /*
     * Ticks the physical model.
     */
    static void tick(void* opaque, LoopTimer* unused);

    mutable std::recursive_mutex mMutex;

    InertialModel mInertialModel;
    AmbientEnvironment mAmbientEnvironment;

    const QAndroidPhysicalStateAgent* mAgent = nullptr;
    bool mIsPhysicalStateChanging = false;

    bool mUseOverride[MAX_SENSORS] = {false};
    mutable long mMeasurementId[MAX_SENSORS] = {0};
#define OVERRIDE_NAME(x) m##x##Override
#define SENSOR_(x,y,z,v,w) \
    v OVERRIDE_NAME(z){0.f};

    SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_NAME

    enum PlaybackState {
        PLAYBACK_STATE_NONE,
        PLAYBACK_STATE_RECORDING,
        PLAYBACK_STATE_PLAYING,
    };

    PlaybackState mPlaybackState = PLAYBACK_STATE_NONE;
    int64_t mPlaybackAndRecordingStartTimeNs = 0L;

    pb::Time mNextPlaybackCommandTime;
    pb::PhysicalModelEvent mNextPlaybackCommand;
    std::unique_ptr<StdioStream> mRecordingAndPlaybackStream;
    std::unique_ptr<StdioStream> mGroundTruthStream;

    const uint32_t kPlaybackFileVersion = 1;

    int64_t mModelTimeNs = 0L;

    LoopTimer* mLoopTimer = nullptr;

    PhysicalModel mPhysicalModelInterface;
};

static glm::vec3 toGlm(vec3 input) {
    return glm::vec3(input.x, input.y, input.z);
}

static vec3 fromGlm(glm::vec3 input) {
    vec3 value;
    value.x = input.x;
    value.y = input.y;
    value.z = input.z;
    return value;
}

PhysicalModelImpl::PhysicalModelImpl(bool shouldTick) {
    mPhysicalModelInterface.opaque = reinterpret_cast<void*>(this);

    if (shouldTick) {
        mLoopTimer = loopTimer_newWithClock(
                looper_getForThread(),
                PhysicalModelImpl::tick,
                reinterpret_cast<void*>(&mPhysicalModelInterface),
                LOOPER_CLOCK_VIRTUAL);
    }
}

PhysicalModelImpl::~PhysicalModelImpl() {
    assert(mAgent == nullptr);
}

PhysicalModel* PhysicalModelImpl::getPhysicalModel() {
    return &mPhysicalModelInterface;
}

PhysicalModelImpl* PhysicalModelImpl::getImpl(PhysicalModel* physicalModel) {
    return physicalModel != nullptr ?
            reinterpret_cast<PhysicalModelImpl*>(physicalModel->opaque) :
            nullptr;
}

pb::PhysicalModelEvent_InterpolationMethod toProto(
        PhysicalInterpolation interpolation) {
    switch (interpolation) {
        case PHYSICAL_INTERPOLATION_SMOOTH:
            return pb::PhysicalModelEvent_InterpolationMethod_Smooth;
        case PHYSICAL_INTERPOLATION_STEP:
            return pb::PhysicalModelEvent_InterpolationMethod_Step;
        default:
            assert(false);  // should never happen
            return pb::PhysicalModelEvent_InterpolationMethod_Smooth;
    }
}

PhysicalInterpolation fromProto(
        pb::PhysicalModelEvent_InterpolationMethod interpolation) {
    switch (interpolation) {
        case pb::PhysicalModelEvent_InterpolationMethod_Smooth:
            return PHYSICAL_INTERPOLATION_SMOOTH;
        case pb::PhysicalModelEvent_InterpolationMethod_Step:
            return PHYSICAL_INTERPOLATION_STEP;
        default:
            W("%s: Unknown interpolation mode %d.  Defaulting to smooth.",
              __FUNCTION__, interpolation);
            return PHYSICAL_INTERPOLATION_SMOOTH;
    }
}

pb::PhysicalModelEvent_ParameterType toProto(PhysicalParameter param) {
    switch (param) {
#define PHYSICAL_PARAMETER_ENUM(x) PHYSICAL_PARAMETER_##x
#define PROTO_ENUM(x) pb::PhysicalModelEvent_ParameterType_##x

#define PHYSICAL_PARAMETER_(x, y, z, w) \
    case PHYSICAL_PARAMETER_ENUM(x):    \
        return PROTO_ENUM(x);

    PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef PHYSICAL_PARAMETER_ENUM
        default:
            assert(false);  // should never happen
            return pb::PhysicalModelEvent_ParameterType_ParameterType_MIN;
    }
}

PhysicalParameter fromProto(pb::PhysicalModelEvent_ParameterType param) {
    switch (param) {
#define PHYSICAL_PARAMETER_ENUM(x) PHYSICAL_PARAMETER_##x
#define PROTO_ENUM(x) pb::PhysicalModelEvent_ParameterType_##x

#define PHYSICAL_PARAMETER_(x,y,z,w) \
        case PROTO_ENUM(x):\
            return PHYSICAL_PARAMETER_ENUM(x);

        PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef PHYSICAL_PARAMETER_ENUM
        default:
            W("%s: Unknown physical parameter %d.", __FUNCTION__, param);
            return MAX_PHYSICAL_PARAMETERS;
    }
}

float getProtoValue_float(const pb::PhysicalModelEvent& target) {
    if (!target.has_value() || target.value().data_size() != 1) {
        W("%s: Error in parsed physics command.  Float parameters should have "
          "exactly one value.  Found %d.",
          __FUNCTION__, target.value().data_size());
        return 0.f;
    }
    return target.value().data(0);
}

vec3 getProtoValue_vec3(const pb::PhysicalModelEvent& target) {
    if (!target.has_value() || target.value().data_size() != 3) {
        W("%s: Error in parsed physics command.  Vec3 parameters should have "
          "exactly three values.  Found %d.",
          __FUNCTION__, target.value().data_size());
        return vec3{0.f, 0.f, 0.f};
    }
    const pb::PhysicalModelEvent_ParameterValue& value = target.value();
    return vec3{value.data(0), value.data(1), value.data(2)};
}

void setProtoValue(pb::PhysicalModelEvent* target, float value) {
    target->mutable_value()->add_data(value);
}

void setProtoValue(pb::PhysicalModelEvent* target, vec3 value) {
    pb::PhysicalModelEvent_ParameterValue* pbValue = target->mutable_value();
    pbValue->add_data(value.x);
    pbValue->add_data(value.y);
    pbValue->add_data(value.z);
}

static bool isPhysicalModelEventValid(const pb::PhysicalModelEvent& event) {
    return (event.has_type() && event.has_interpolation_method());
}

static bool parseNextCommand(StdioStream* inStream,
                             pb::Time* outTime,
                             pb::PhysicalModelEvent* outCommand) {
    const std::string nextCommand = inStream->getString();

    pb::RecordedEvent event;
    if (nextCommand.empty() || !event.ParseFromString(nextCommand)) {
        return false;
    }

    if (!event.has_stream_type() ||
        event.stream_type() != pb::RecordedEvent_StreamType_PHYSICAL_MODEL) {
        // Currently if other stream types are present the recording is
        // considered invalid.
        return false;
    }

    if (!event.has_event_time() || !event.has_physical_model() ||
        !isPhysicalModelEventValid(event.physical_model())) {
        return false;
    }

    *outTime = event.event_time();
    *outCommand = event.physical_model();
    return true;
}

void PhysicalModelImpl::setCurrentTime(int64_t time_ns) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    while (mPlaybackState == PLAYBACK_STATE_PLAYING &&
           time_ns >= mPlaybackAndRecordingStartTimeNs +
                              mNextPlaybackCommandTime.timestamp()) {
        setCurrentTimeInternal(mPlaybackAndRecordingStartTimeNs +
                               mNextPlaybackCommandTime.timestamp());
        switch (fromProto(mNextPlaybackCommand.type())) {
#define PHYSICAL_PARAMETER_ENUM(x) PHYSICAL_PARAMETER_##x
#define SET_TARGET_INTERNAL_FUNCTION_NAME(x) setTargetInternal##x
#define GET_PROTO_VALUE_FUNCTION_NAME(x) getProtoValue_##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
            case PHYSICAL_PARAMETER_ENUM(x):\
                SET_TARGET_INTERNAL_FUNCTION_NAME(z)(\
                        GET_PROTO_VALUE_FUNCTION_NAME(w)(mNextPlaybackCommand),\
                        fromProto(mNextPlaybackCommand.interpolation_method()));\
                break;

            PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef GET_PROTO_VALUE_FUNCTION_NAME
#undef SET_TARGET_INTERNAL_FUNCTION_NAME
#undef PHYSICAL_PARAMETER_ENUM
            default:
                break;
        }
        if (!parseNextCommand(mRecordingAndPlaybackStream.get(),
                              &mNextPlaybackCommandTime,
                              &mNextPlaybackCommand)) {
            stopRecordAndPlayback();
        }
    }

    setCurrentTimeInternal(time_ns);
}

void PhysicalModelImpl::setCurrentTimeInternal(int64_t time_ns) {
    mModelTimeNs = time_ns;
    bool isInertialModelStable = mInertialModel.setCurrentTime(time_ns) ==
            INERTIAL_STATE_STABLE;
    bool isAmbientModelStable = mAmbientEnvironment.setCurrentTime(time_ns) ==
            AMBIENT_STATE_STABLE;
    if (isInertialModelStable && isAmbientModelStable &&
            mIsPhysicalStateChanging) {
        physicalStateStabilized();
    }
}

void PhysicalModelImpl::setTargetInternalPosition(vec3 position,
        PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mInertialModel.setTargetPosition(toGlm(position), mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalVelocity(vec3 velocity,
        PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mInertialModel.setTargetVelocity(toGlm(velocity), mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalAmbientMotion(float bounds,
        PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mInertialModel.setTargetAmbientMotion(bounds, mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalRotation(vec3 rotation,
        PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mInertialModel.setTargetRotation(glm::toQuat(glm::eulerAngleXYZ(
            glm::radians(rotation.x),
            glm::radians(rotation.y),
            glm::radians(rotation.z))), mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalMagneticField(
        vec3 field, PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setMagneticField(
        field.x, field.y, field.z, mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalTemperature(
        float celsius, PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setTemperature(celsius, mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalProximity(
        float centimeters, PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setProximity(centimeters, mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalLight(
        float lux, PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setLight(lux, mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalPressure(
        float hPa, PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setPressure(hPa, mode);
    targetStateChanged();
}

void PhysicalModelImpl::setTargetInternalHumidity(
        float percentage, PhysicalInterpolation mode) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    physicalStateChanging();
    mAmbientEnvironment.setHumidity(percentage, mode);
    targetStateChanged();
}

vec3 PhysicalModelImpl::getParameterPosition(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return fromGlm(mInertialModel.getPosition(parameterValueType));
}

vec3 PhysicalModelImpl::getParameterVelocity(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return fromGlm(mInertialModel.getVelocity(parameterValueType));
}

float PhysicalModelImpl::getParameterAmbientMotion(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mInertialModel.getAmbientMotion(parameterValueType);
}

vec3 PhysicalModelImpl::getParameterRotation(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    glm::vec3 rotationRadians;
    glm::extractEulerAngleXYZ(
            glm::mat4_cast(mInertialModel.getRotation(parameterValueType)),
            rotationRadians.x, rotationRadians.y, rotationRadians.z);
    return fromGlm(glm::degrees(rotationRadians));
}

vec3 PhysicalModelImpl::getParameterMagneticField(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return fromGlm(mAmbientEnvironment.getMagneticField(parameterValueType));
}

float PhysicalModelImpl::getParameterTemperature(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getTemperature(parameterValueType);
}

float PhysicalModelImpl::getParameterProximity(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getProximity(parameterValueType);
}

float PhysicalModelImpl::getParameterLight(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getLight(parameterValueType);
}

float PhysicalModelImpl::getParameterPressure(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getPressure(parameterValueType);
}

float PhysicalModelImpl::getParameterHumidity(
        ParameterValueType parameterValueType) const {
    std::lock_guard<std::recursive_mutex> lock(mMutex);
    return mAmbientEnvironment.getHumidity(parameterValueType);
}

#define GET_FUNCTION_NAME(x) get##x
#define OVERRIDE_FUNCTION_NAME(x) override##x
#define OVERRIDE_NAME(x) m##x##Override
#define SENSOR_NAME(x) ANDROID_SENSOR_##x
#define PHYSICAL_NAME(x) getPhysical##x

// Implement sensor overrides.
#define SENSOR_(x,y,z,v,w) \
void PhysicalModelImpl::OVERRIDE_FUNCTION_NAME(z)(v override_value) {\
    setOverride(SENSOR_NAME(x), &OVERRIDE_NAME(z), override_value);\
}

SENSORS_LIST
#undef SENSOR_

// Implement getters that respect overrides.
#define SENSOR_(x,y,z,v,w) \
v PhysicalModelImpl::GET_FUNCTION_NAME(z)(\
        long* measurement_id) const {\
    return getSensorValue<v>(SENSOR_NAME(x),\
                             &OVERRIDE_NAME(z),\
                             std::bind(&PhysicalModelImpl::PHYSICAL_NAME(z),\
                                       this),\
                             measurement_id);\
}

SENSORS_LIST
#undef SENSOR_

#undef PHYSICAL_NAME
#undef SENSOR_NAME
#undef OVERRIDE_NAME
#undef OVERRIDE_FUNCTION_NAME
#undef GET_FUNCTION_NAME

vec3 PhysicalModelImpl::getPhysicalAccelerometer() const {
    //Implementation Note:
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

void PhysicalModelImpl::getTransform(
        float* out_translation_x, float* out_translation_y, float* out_translation_z,
        float* out_rotation_x, float* out_rotation_y, float* out_rotation_z,
        int64_t* out_timestamp) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    // Note: when getting the transform, we always update the current time
    //       so that consumers of this transform get the most up-to-date
    //       value possible, and so that transform timestamps progress even when
    //       sensors are not polling.
    const DurationNs now_ns =
            looper_nowNsWithClock(looper_getForThread(), LOOPER_CLOCK_VIRTUAL);
    setCurrentTime(now_ns);

    const vec3 position = getParameterPosition(PARAMETER_VALUE_TYPE_CURRENT);
    *out_translation_x = position.x;
    *out_translation_y = position.y;
    *out_translation_z = position.z;
    const vec3 rotation = getParameterRotation(PARAMETER_VALUE_TYPE_CURRENT);
    *out_rotation_x = rotation.x;
    *out_rotation_y = rotation.y;
    *out_rotation_z = rotation.z;
    *out_timestamp = mModelTimeNs;

    if (mGroundTruthStream) {
        fprintf(mGroundTruthStream->get(), "%" PRId64 " %f %f %f %f %f %f\n",
                *out_timestamp, *out_translation_x, *out_translation_y,
                *out_translation_z, *out_rotation_x, *out_rotation_y,
                *out_rotation_z);
    }
}

void PhysicalModelImpl::setPhysicalStateAgent(
        const QAndroidPhysicalStateAgent* agent) {
    std::lock_guard<std::recursive_mutex> lock(mMutex);

    if (mAgent != nullptr && mAgent->onPhysicalStateStabilized != nullptr) {
        mAgent->onPhysicalStateStabilized(mAgent->context);
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

static void readValueFromStream(Stream* f, vec3* value) {
    value->x = stream_get_float(f);
    value->y = stream_get_float(f);
    value->z = stream_get_float(f);
}

static void readValueFromStream(Stream* f, float* value) {
    *value = stream_get_float(f);
}

static void writeValueToStream(Stream* f, vec3 value) {
    stream_put_float(f, value.x);
    stream_put_float(f, value.y);
    stream_put_float(f, value.z);
}

static void writeValueToStream(Stream* f, float value) {
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
#define PHYSICAL_PARAMETER_NAME(x) PHYSICAL_PARAMETER_##x
#define GET_PARAMETER_FUNCTION_NAME(x) getParameter##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
            case PHYSICAL_PARAMETER_NAME(x): {\
                w targetValue = GET_PARAMETER_FUNCTION_NAME(z)(\
                        PARAMETER_VALUE_TYPE_TARGET);\
                writeValueToStream(f, targetValue);\
                break;\
            }

            PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef GET_PARAMETER_FUNCTION_NAME
#undef PHYSICAL_PARAMETER_NAME
            default:
                assert(false);  // should never happen
                break;
        }
    }

    // then save overrides
    stream_put_be32(f, MAX_SENSORS);

    for (int sensor = 0;
         sensor < MAX_SENSORS;
         sensor++) {
        stream_put_be32(f, mUseOverride[sensor]);
        if (mUseOverride[sensor]) {
            switch(sensor) {
#define SENSOR_NAME(x) ANDROID_SENSOR_##x
#define OVERRIDE_NAME(x) m##x##Override
#define SENSOR_(x,y,z,v,w)\
                case SENSOR_NAME(x): {\
                    writeValueToStream(f, OVERRIDE_NAME(z));\
                    break;\
                }

                SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_NAME
#undef SENSOR_NAME
                default:
                    assert(false);  // should never happen
                    break;
            }
        }
    }
}

int PhysicalModelImpl::load(Stream* f) {
    // first load targets
    const int32_t num_physical_parameters = stream_get_be32(f);
    if (num_physical_parameters > MAX_PHYSICAL_PARAMETERS) {
        D("%s: cannot load: snapshot requires %d physical parameters, %d "
          "available\n",
          __FUNCTION__, num_physical_parameters, MAX_PHYSICAL_PARAMETERS);
        return -EIO;
    }

    stopRecordAndPlayback();

    // Note: any new target params will remain at their defaults.

    for (int parameter = 0;
         parameter < num_physical_parameters;
         parameter++) {
        switch (parameter) {
#define PHYSICAL_PARAMETER_NAME(x) PHYSICAL_PARAMETER_##x
#define SET_TARGET_FUNCTION_NAME(x) setTargetInternal##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
            case PHYSICAL_PARAMETER_NAME(x): {\
                w value;\
                readValueFromStream(f, &value);\
                SET_TARGET_FUNCTION_NAME(z)(\
                        value, PHYSICAL_INTERPOLATION_STEP);\
                break;\
            }

            PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef SET_TARGET_FUNCTION_NAME
#undef PHYSICAL_PARAMETER_NAME
            default:
                assert(false);  // should never happen
                break;
        }
    }

    // then load overrides

    /* check number of physical sensors */
    int32_t num_sensors = stream_get_be32(f);
    if (num_sensors > MAX_SENSORS) {
        D("%s: cannot load: snapshot requires %d physical sensors, %d "
          "available\n",
          __FUNCTION__, num_sensors, MAX_SENSORS);
        return -EIO;
    }

    for (int sensor = 0;
         sensor < num_sensors;
         sensor++) {
        if (stream_get_be32(f)) {
            switch(sensor) {
#define SENSOR_NAME(x) ANDROID_SENSOR_##x
#define OVERRIDE_FUNCTION_NAME(x) override##x
#define SENSOR_(x,y,z,v,w)\
                case SENSOR_NAME(x): {\
                    v value;\
                    readValueFromStream(f, &value);\
                    OVERRIDE_FUNCTION_NAME(z)(value);\
                    break;\
                }

                SENSORS_LIST
#undef SENSOR_
#undef OVERRIDE_FUNCTION_NAME
#undef SENSOR_NAME
                default:
                    assert(false);  // should never happen
                    break;
            }
        }
    }

    return 0;
}

int PhysicalModelImpl::record(const char* filename) {
    stopRecordAndPlayback();

    if (!filename) {
        E("%s: Must specify filename for writing.  Physical Motion will not be "
          "recorded.", __FUNCTION__, filename);
        return -1;
    }

    const std::string path = PathUtils::join(
            System::get()->getHomeDirectory(), filename);

    mRecordingAndPlaybackStream.reset(new StdioStream(fopen(path.c_str(), "wb"),
            StdioStream::kOwner));
    if (!mRecordingAndPlaybackStream.get()) {
        E("%s: Error unable to open physics file %s for writing.  "
          "Physical Motion will not be recorded.", __FUNCTION__, filename);
        return -1;
    }

    mPlaybackState = PLAYBACK_STATE_RECORDING;
    mPlaybackAndRecordingStartTimeNs = mModelTimeNs;
    mRecordingAndPlaybackStream->putBe32(kPlaybackFileVersion);
    automation::AutomationEventSink::get().registerStream(
            mRecordingAndPlaybackStream.get(),
            automation::StreamEncoding::BinaryPbChunks);
    return 0;
}

int PhysicalModelImpl::recordGroundTruth(const char* filename) {
    stopRecordGroundTruth();

    if (!filename) {
        E("%s: Must specify filename for writing.  Physical Motion will not be "
          "recorded.",
          __FUNCTION__, filename);
        return -1;
    }

    std::string path = filename;
    if (!PathUtils::isAbsolute(path)) {
        path = PathUtils::join(System::get()->getHomeDirectory(), filename);
    }

    mGroundTruthStream.reset(
            new StdioStream(fopen(path.c_str(), "wb"), StdioStream::kOwner));
    if (!mGroundTruthStream.get()) {
        E("%s: Error unable to open file %s for writing.  "
          "Physical motion ground truth will not be recorded.",
          __FUNCTION__, filename);
        return -1;
    }

    return 0;
}

int PhysicalModelImpl::playback(const char* filename) {
    stopRecordAndPlayback();

    if (!filename) {
        E("%s: Must specify filename for reading.  Physical Motion will not be "
          "played back.", __FUNCTION__, filename);
        return -1;
    }

    const std::string path = PathUtils::join(
            System::get()->getHomeDirectory(), filename);

    std::unique_ptr<StdioStream> playbackStream(
            new StdioStream(fopen(path.c_str(), "rb"), StdioStream::kOwner));
    if (!playbackStream.get()) {
        E("%s: Error unable to open physics file %s for reading.  "
          "Physical Motion will not be played back.", __FUNCTION__, filename);
        return -1;
    }

    uint32_t version = playbackStream->getBe32();
    if (kPlaybackFileVersion != version) {
        E("%s: Unsupported version %d in physics file %s.  Required Version "
          "is %d.  Physical Motion will not be played back.",
          __FUNCTION__, version, filename, kPlaybackFileVersion);
        return -1;
    }

    if (!parseNextCommand(playbackStream.get(), &mNextPlaybackCommandTime,
                          &mNextPlaybackCommand)) {
        E("%s: Invalid physics file %s.  Physical Motion will not be played "
          "back.",
          __FUNCTION__, filename);
        return -1;
    }
    mRecordingAndPlaybackStream.swap(playbackStream);
    mPlaybackState = PLAYBACK_STATE_PLAYING;
    mPlaybackAndRecordingStartTimeNs = mModelTimeNs;
    return 0;
}

void PhysicalModelImpl::stopRecordAndPlayback() {
    if (mRecordingAndPlaybackStream &&
        mPlaybackState == PLAYBACK_STATE_RECORDING) {
        automation::AutomationEventSink::get().unregisterStream(
                mRecordingAndPlaybackStream.get());
    }
    mRecordingAndPlaybackStream.reset(nullptr);
    mPlaybackState = PLAYBACK_STATE_NONE;
    mPlaybackAndRecordingStartTimeNs = 0;
    stopRecordGroundTruth();
}

void PhysicalModelImpl::stopRecordGroundTruth() {
    mGroundTruthStream.reset(nullptr);
}

#define SET_TARGET_FUNCTION_NAME(x) setTarget##x
#define SET_TARGET_INTERNAL_FUNCTION_NAME(x) setTargetInternal##x
#define PHYSICAL_PARAMETER_ENUM(x) PHYSICAL_PARAMETER_##x
#define PHYSICAL_PARAMETER_(x, y, z, w)                         \
    void PhysicalModelImpl::SET_TARGET_FUNCTION_NAME(z)(        \
            w value, PhysicalInterpolation mode) {              \
        generateEvent(PHYSICAL_PARAMETER_ENUM(x), mode, value); \
        SET_TARGET_INTERNAL_FUNCTION_NAME(z)(value, mode);      \
    }

PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef PHYSICAL_PARAMETER_ENUM
#undef SET_TARGET_INTERNAL_FUNCTION_NAME
#undef SET_TARGET_FUNCTION_NAME

void PhysicalModelImpl::physicalStateChanging() {
    // Note: We only call onPhysicalStateChanging if this is a transition from
    // stable to changing (i.e. don't call if we get to physicalStateChanging
    // calls in a row without a physicalStateStabilized call in between).
    if (!mIsPhysicalStateChanging && mAgent != nullptr &&
            mAgent->onPhysicalStateChanging != nullptr) {
        mAgent->onPhysicalStateChanging(mAgent->context);
    }
    if (mLoopTimer != nullptr) {
        loopTimer_startRelative(mLoopTimer, 0);
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
    for (int i = 0; i < MAX_SENSORS; i++) {
        mMeasurementId[i]++;
    }
    mIsPhysicalStateChanging = false;
}

void PhysicalModelImpl::targetStateChanged() {
    /* when target state changes we reset all sensor overrides */
    for (int i = 0; i < MAX_SENSORS; i++) {
        mUseOverride[i] = false;
    }
    if (mAgent != nullptr && mAgent->onTargetStateChanged != nullptr) {
        mAgent->onTargetStateChanged(mAgent->context);
    }
}

template <typename ValueType>
void PhysicalModelImpl::generateEvent(PhysicalParameter type,
                   PhysicalInterpolation mode,
                   ValueType value) {
    // TODO(jwmcglynn): For recordings we need some way to save the initial
    // state.
    pb::Time time;
    time.set_timestamp(mModelTimeNs - mPlaybackAndRecordingStartTimeNs);

    pb::PhysicalModelEvent command;
    command.set_type(toProto(type));
    command.set_interpolation_method(toProto(mode));
    setProtoValue(&command, value);

    automation::AutomationEventSink::get().recordPhysicalModelEvent(time,
                                                                    command);
}

void PhysicalModelImpl::tick(void* opaque, LoopTimer* unused) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(
            reinterpret_cast<PhysicalModel*>(opaque));
    if (impl != nullptr) {
        const DurationNs now_ns =
                looper_nowNsWithClock(looper_getForThread(),
                LOOPER_CLOCK_VIRTUAL);
        impl->setCurrentTime(now_ns);

        // if the model is still changing, schedule another timer.
        if (impl->mIsPhysicalStateChanging) {
            assert(impl->mLoopTimer != nullptr);
            loopTimer_startRelative(impl->mLoopTimer, 10);
        }
    }
}

}  // namespace physics
}  // namespace android

using android::physics::PhysicalModelImpl;

PhysicalModel* physicalModel_new(bool shouldTick) {
    PhysicalModelImpl* impl = new PhysicalModelImpl(shouldTick);
    return impl != nullptr ? impl->getPhysicalModel() : nullptr;
}

void physicalModel_free(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        delete impl;
    }
}

void physicalModel_setCurrentTime(
        PhysicalModel* model, int64_t time_ns) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->setCurrentTime(time_ns);
    }
}

#define SET_PHYSICAL_TARGET_FUNCTION_NAME(x) physicalModel_setTarget##x
#define SET_TARGET_FUNCTION_NAME(x) setTarget##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
void SET_PHYSICAL_TARGET_FUNCTION_NAME(z)(\
        PhysicalModel* model, w value, PhysicalInterpolation mode) {\
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);\
    if (impl != nullptr) {\
        impl->SET_TARGET_FUNCTION_NAME(z)(value, mode);\
    }\
}

PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef SET_TARGET_FUNCTION_NAME
#undef SET_PHYSICAL_TARGET_FUNCTION_NAME


#define GET_PHYSICAL_PARAMETER_FUNCTION_NAME(x) physicalModel_getParameter##x
#define GET_PARAMETER_FUNCTION_NAME(x) getParameter##x
#define PHYSICAL_PARAMETER_(x,y,z,w) \
w GET_PHYSICAL_PARAMETER_FUNCTION_NAME(z)(\
        PhysicalModel* model, ParameterValueType parameterValueType) {\
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);\
    w result;\
    if (impl != nullptr) {\
        result = impl->GET_PARAMETER_FUNCTION_NAME(z)(parameterValueType);\
    } else {\
        result = {0.f};\
    }\
    return result;\
}

PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef GET_PARAMETER_FUNCTION_NAME
#undef GET_PHYSICAL_PARAMETER_FUNCTION_NAME

#define OVERRIDE_FUNCTION_NAME(x) override##x
#define PHYSICAL_OVERRIDE_FUNCTION_NAME(x) physicalModel_override##x
#define SENSOR_(x,y,z,v,w) \
void PHYSICAL_OVERRIDE_FUNCTION_NAME(z)(\
        PhysicalModel* model,\
        v override_value) {\
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);\
    if (impl != nullptr) {\
        impl->OVERRIDE_FUNCTION_NAME(z)(override_value);\
    }\
}

SENSORS_LIST
#undef SENSOR_
#undef PHYSICAL_OVERRIDE_FUNCTION_NAME
#undef OVERRIDE_FUNCTION_NAME

#define GET_FUNCTION_NAME(x) get##x
#define PHYSICAL_GET_FUNCTION_NAME(x) physicalModel_get##x
#define SENSOR_(x,y,z,v,w) \
v PHYSICAL_GET_FUNCTION_NAME(z)(\
        PhysicalModel* model, long* measurement_id) {\
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);\
    *measurement_id = 0L;\
    return impl != nullptr ?\
            impl->GET_FUNCTION_NAME(z)(measurement_id) :\
            v{0.f};\
}

SENSORS_LIST
#undef SENSOR_
#undef PHYSICAL_GET_FUNCTION_NAME
#undef GET_FUNCTION_NAME

void physicalModel_getTransform(PhysicalModel* model,
    float* out_translation_x, float* out_translation_y, float* out_translation_z,
    float* out_rotation_x, float* out_rotation_y, float* out_rotation_z,
    int64_t* out_timestamp) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->getTransform(out_translation_x, out_translation_y, out_translation_z,
                out_rotation_x, out_rotation_y, out_rotation_z,
                out_timestamp);
    }
}

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

int physicalModel_record(PhysicalModel* model, const char* filename) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        return impl->record(filename);
    }
    return -1;
}

int physicalModel_playback(PhysicalModel* model, const char* filename) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        return impl->playback(filename);
    }
    return -1;
}

int physicalModel_recordGroundTruth(PhysicalModel* model,
                                    const char* filename) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        return impl->recordGroundTruth(filename);
    }
    return -1;
}

int physicalModel_stopRecordAndPlayback(PhysicalModel* model) {
    PhysicalModelImpl* impl = PhysicalModelImpl::getImpl(model);
    if (impl != nullptr) {
        impl->stopRecordAndPlayback();
        impl->stopRecordGroundTruth();
        return 0;
    }
    return -1;
}

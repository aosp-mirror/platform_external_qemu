/* Copyright (C) 2009 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#pragma once

#include "android/physics/Physics.h"
#include "android/utils/compiler.h"
#include "host-common/window_agent.h"

#include <stdint.h>
#include <math.h>

/* forward declaration */
struct QAndroidPhysicalStateAgent;
typedef struct PhysicalModel PhysicalModel;

ANDROID_BEGIN_HEADER

/* initialize sensor emulation */
extern void  android_hw_sensors_init(const QAndroidEmulatorWindowAgent* windowAgent);

const QAndroidEmulatorWindowAgent* android_hw_sensors_get_window_agent();

/* initialize remote controller for HW sensors. This must be called after
 * init_clocks(), i.e. later than android_hw_sensors_init(). */
extern void android_hw_sensors_init_remote_controller( void );

/* struct for use in sensor values */
struct vec3 {
    float x;
    float y;
    float z;

#ifdef __cplusplus
    bool operator==(const vec3& rhs) const {
        const float kEpsilon = 0.00001f;

        const double diffX = fabs(x - rhs.x);
        const double diffY = fabs(y - rhs.y);
        const double diffZ = fabs(z - rhs.z);
        return (diffX < kEpsilon && diffY < kEpsilon && diffZ < kEpsilon);
    }

    bool operator!=(const vec3& rhs) const {
        return !(*this == rhs);
    }
#endif  // __cplusplus
};

typedef struct vec3 vec3;

/* struct for use in sensor values */
struct vec4 {
    float x;
    float y;
    float z;
    float w;

#ifdef __cplusplus
    bool operator==(const vec4& rhs) const {
        const float kEpsilon = 0.00001f;

        const double diffX = fabs(x - rhs.x);
        const double diffY = fabs(y - rhs.y);
        const double diffZ = fabs(z - rhs.z);
        const double diffW = fabs(w - rhs.w);
        return (diffX < kEpsilon && diffY < kEpsilon && diffZ < kEpsilon &&
                diffW < kEpsilon);
    }

    bool operator!=(const vec4& rhs) const { return !(*this == rhs); }
#endif  // __cplusplus
};

typedef struct vec4 vec4;

/* NOTE: Sensor status Error definition, It will be used in the Sensors Command
 *       part in android-qemu1-glue/console.c. Details:
 *       SENSOR_STATUS_NO_SERVICE: "sensors" qemud service is not available/initiated.
 *       SENSOR_STATUS_DISABLED: sensor is disabled.
 *       SENSOR_STATUS_UNKNOWN: wrong sensor name.
 *       SENSOR_STATUS_OK: Everything is OK to the current sensor.
 * */
typedef enum{
    SENSOR_STATUS_NO_SERVICE = -3,
    SENSOR_STATUS_DISABLED   = -2,
    SENSOR_STATUS_UNKNOWN    = -1,
    SENSOR_STATUS_OK         = 0,
} SensorStatus;

/* NOTE: this list must be the same that the one defined in
 *       the sensors_qemu.c source of the libsensors.goldfish.so
 *       library.
 *
 *       DO NOT CHANGE THE ORDER IN THIS LIST, UNLESS YOU INTEND
 *       TO BREAK SNAPSHOTS!
 */
// clang-format off
#define  SENSORS_LIST  \
    SENSOR_(ACCELERATION,"acceleration",Accelerometer,vec3,"acceleration:%g:%g:%g") \
    SENSOR_(GYROSCOPE,"gyroscope",Gyroscope,vec3,"gyroscope:%g:%g:%g") \
    SENSOR_(MAGNETIC_FIELD,"magnetic-field",Magnetometer,vec3,"magnetic:%g:%g:%g") \
    SENSOR_(ORIENTATION,"orientation",Orientation,vec3,"orientation:%g:%g:%g") \
    SENSOR_(TEMPERATURE,"temperature",Temperature,float,"temperature:%g") \
    SENSOR_(PROXIMITY,"proximity",Proximity,float,"proximity:%g") \
    SENSOR_(LIGHT,"light",Light,float,"light:%g") \
    SENSOR_(PRESSURE,"pressure",Pressure,float,"pressure:%g") \
    SENSOR_(HUMIDITY,"humidity",Humidity,float,"humidity:%g") \
    SENSOR_(MAGNETIC_FIELD_UNCALIBRATED,"magnetic-field-uncalibrated",MagnetometerUncalibrated,vec3,"magnetic-uncalibrated:%g:%g:%g") \
    SENSOR_(GYROSCOPE_UNCALIBRATED,"gyroscope-uncalibrated",GyroscopeUncalibrated,vec3,"gyroscope-uncalibrated:%g:%g:%g") \
    SENSOR_(HINGE_ANGLE0, "hinge-angle0", HingeAngle0, float, "hinge-angle0:%g") \
    SENSOR_(HINGE_ANGLE1, "hinge-angle1", HingeAngle1, float, "hinge-angle1:%g") \
    SENSOR_(HINGE_ANGLE2, "hinge-angle2", HingeAngle2, float, "hinge-angle2:%g") \
    SENSOR_(HEART_RATE, "heart-rate", HeartRate, float, "heart-rate:%g") \
    SENSOR_(RGBC_LIGHT, "rgbc-light", RgbcLight, vec4, "rgbc-light:%g:%g:%g:%g") \
    SENSOR_(WRIST_TILT, "wrist-tilt", WristTilt, float, "wrist-tilt:%g") \
// clang-format on
typedef enum {
#define  SENSOR_(x,y,z,v,w)  ANDROID_SENSOR_##x,
    SENSORS_LIST
#undef   SENSOR_
    MAX_SENSORS  /* do not remove */
} AndroidSensor;

/* NOTE: Physical parameters Error definition, It will be used in the Physics
 *       Command part in console.c. Details:
 *       PHYSICAL_PARAMETER_STATUS_NO_SERVICE: no physical model present.
 *       PHYSICAL_PARAMETER_STATUS_UNKNOWN: wrong physical parameter name.
 *       PHYSICAL_PARAMETER_STATUS_OK: Everything is OK to the current physical
 *                                     param.
 * */
typedef enum{
    PHYSICAL_PARAMETER_STATUS_NO_SERVICE = -2,
    PHYSICAL_PARAMETER_STATUS_UNKNOWN    = -1,
    PHYSICAL_PARAMETER_STATUS_OK         = 0,
} PhysicalParameterStatus;

/*
 * Note: DO NOT CHANGE THE ORDER IN THIS LIST, UNLESS YOU INTEND
 *       TO BREAK SNAPSHOTS!
 */
// clang-format off
#define  PHYSICAL_PARAMETERS_LIST  \
    PHYSICAL_PARAMETER_(POSITION,"position",Position,vec3) \
    PHYSICAL_PARAMETER_(ROTATION,"rotation",Rotation,vec3) \
    PHYSICAL_PARAMETER_(MAGNETIC_FIELD,"magnetic-field",MagneticField,vec3) \
    PHYSICAL_PARAMETER_(TEMPERATURE,"temperature",Temperature,float) \
    PHYSICAL_PARAMETER_(PROXIMITY,"proximity",Proximity,float) \
    PHYSICAL_PARAMETER_(LIGHT,"light",Light,float) \
    PHYSICAL_PARAMETER_(PRESSURE,"pressure",Pressure,float) \
    PHYSICAL_PARAMETER_(HUMIDITY,"humidity",Humidity,float) \
    PHYSICAL_PARAMETER_(VELOCITY,"velocity",Velocity,vec3) \
    PHYSICAL_PARAMETER_(AMBIENT_MOTION,"ambientMotion",AmbientMotion,float) \
    PHYSICAL_PARAMETER_(HINGE_ANGLE0,"hinge-angle0",HingeAngle0,float) \
    PHYSICAL_PARAMETER_(HINGE_ANGLE1,"hinge-angle1",HingeAngle1,float) \
    PHYSICAL_PARAMETER_(HINGE_ANGLE2,"hinge-angle2",HingeAngle2,float) \
    PHYSICAL_PARAMETER_(ROLLABLE0,"rollable0",Rollable0,float) \
    PHYSICAL_PARAMETER_(ROLLABLE1,"rollable1",Rollable1,float) \
    PHYSICAL_PARAMETER_(ROLLABLE2,"rollable2",Rollable2,float) \
    PHYSICAL_PARAMETER_(POSTURE,"posture",Posture,float) \
    PHYSICAL_PARAMETER_(HEART_RATE, "heart-rate", HeartRate, float) \
    PHYSICAL_PARAMETER_(RGBC_LIGHT, "rgbc-light", RgbcLight, vec4) \
    PHYSICAL_PARAMETER_(WRIST_TILT, "wrist-tilt", WristTilt, float) \
// clang-format on
typedef enum {
#define PHYSICAL_PARAMETER_(x,y,z,w)  PHYSICAL_PARAMETER_##x,
    PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
    MAX_PHYSICAL_PARAMETERS
} PhysicalParameter;

extern void  android_hw_sensor_enable( AndroidSensor  sensor );

/* COARSE ORIENTATION VALUES */
typedef enum {
    ANDROID_COARSE_PORTRAIT,            // 0 degrees
    ANDROID_COARSE_REVERSE_LANDSCAPE,   // 90 degrees
    ANDROID_COARSE_REVERSE_PORTRAIT,    // 180 degrees
    ANDROID_COARSE_LANDSCAPE,           // 270 degrees
} AndroidCoarseOrientation;

/* change the coarse orientation value */
extern int android_sensors_set_coarse_orientation(
        AndroidCoarseOrientation  orient, float tilt_degrees);

/* get the coarse orientation value */
extern AndroidCoarseOrientation android_sensors_get_coarse_orientation();

/* get sensor values */
extern int android_sensors_get(int sensor_id,
                               float* const* out,
                               const size_t count);

/* get sensor values size */
extern int android_sensors_get_size(int sensor_id, size_t* size);

/* set sensor override values */
extern int android_sensors_override_set(int sensor_id,
                                        const float* val,
                                        const size_t count);

/* Get sensor id from sensor name */
extern int android_sensors_get_id_from_name( const char* sensorname );

/* Get sensor name from sensor id */
extern const char* android_sensors_get_name_from_id( int sensor_id );

/* Get sensor from sensor id */
extern uint8_t android_sensors_get_sensor_status( int sensor_id );

/* Get the host-to-guest time offset */
extern int64_t android_sensors_get_time_offset();

/* Get the current sensor delay (determines update rate) */
extern int32_t android_sensors_get_delay_ms();

/* Get the physical model instance. */
extern PhysicalModel* android_physical_model_instance();

/* Get physical model values size */
extern int android_physical_model_get_size(int physical_parameter,
                                           size_t* size);

/* Get physical model values */
extern int android_physical_model_get(int physical_parameter,
                                      float* const* out,
                                      const size_t count,
                                      ParameterValueType parameter_value_type);

/* Set physical model target values */
extern int android_physical_model_set(int physical_parameter,
                                      const float* val,
                                      const size_t count,
                                      int interpolation_method);

/* Set agent to receive physical state change callbacks */
extern int android_physical_agent_set(
    const struct QAndroidPhysicalStateAgent* agent);

/* Get physical parameter id from name */
extern int android_physical_model_get_parameter_id_from_name(
    const char* physical_parameter_name );

/* Get physical parameter name from id */
extern const char* android_physical_model_get_parameter_name_from_id(
    int physical_parameter_id );

// Start recording ground truth to the specified file.
extern int android_physical_model_record_ground_truth(const char* file_name);

// Stop recording ground truth.
extern int android_physical_model_stop_recording();

extern void android_hw_sensor_set_window_agent(void* agent);

/* Foldable state */

#define ANDROID_FOLDABLE_MAX_HINGES 3
#define ANDROID_FOLDABLE_MAX_ROLLS 2
#if ANDROID_FOLDABLE_MAX_HINGES > ANDROID_FOLDABLE_MAX_ROLLS
#define ANDROID_FOLDABLE_MAX_HINGES_ROLLS ANDROID_FOLDABLE_MAX_HINGES
#else
#define ANDROID_FOLDABLE_MAX_HINGES_ROLLS ANDROID_FOLDABLE_MAX_ROLLS
#endif
#define ANDROID_FOLDABLE_MAX_DISPLAY_REGIONS 3

enum FoldablePostures {
    POSTURE_UNKNOWN = 0,
    POSTURE_CLOSED = 1,
    POSTURE_HALF_OPENED = 2,
    POSTURE_OPENED = 3,
    POSTURE_FLIPPED = 4,
    POSTURE_TENT = 5,
    POSTURE_MAX
};

struct AnglesToPosture {
    struct {
        float left;
        float right;
        float default_value;
    } angles[ANDROID_FOLDABLE_MAX_HINGES_ROLLS];
    enum FoldablePostures posture;
};

enum FoldableDisplayType {
    // Horizontal split means something like a laptop, i.e.
    // |-----| Camera is here
    // | top |
    // |-----| hinge 0
    // |     |
    // |-----| hinge 1
    // |     |
    // |-----|
    ANDROID_FOLDABLE_HORIZONTAL_SPLIT = 0,

    // Vertical split is left to right, rotated version of horizontal split:
    // |-camera|-------|------|-------|
    // |       |       |      |       |
    // |       |       |      |       |
    // |-------|-------|------|-------|
    // hinge:  0       1      2
    // |-----|
    // | top |
    // |-----| hinge 0
    // |     |
    // |-----| hinge 1
    // |     |
    // |-----|
    ANDROID_FOLDABLE_VERTICAL_SPLIT = 1,

    // Roll configurations (essentially the # hinges are infinite,
    // representable via separate parameters)
    ANDROID_FOLDABLE_HORIZONTAL_ROLL = 2,
    ANDROID_FOLDABLE_VERTICAL_ROLL = 3,
    ANDROID_FOLDABLE_TYPE_MAX
};

enum FoldableHingeSubType {
    ANDROID_FOLDABLE_HINGE_FOLD = 0,
    ANDROID_FOLDABLE_HINGE_HINGE = 1,
    ANDROID_FOLDABLE_HINGE_SUB_TYPE_MAX
};

struct FoldableHingeParameters {
    int x, y, width, height;
    int displayId;
    float minDegrees;
    float maxDegrees;
    float defaultDegrees;
};

struct RollableParameters {
    float rollRadiusAsDisplayPercent; // % of display height (horiz. roll) or display width (vertical roll) that determines the radius of the rollable
    int displayId;
    float minRolledPercent;
    float maxRolledPercent;
    float defaultRolledPercent;
    int direction;
};

struct FoldableConfig {
    enum FoldableDisplayType type;
    enum FoldableHingeSubType hingesSubType;
    bool supportedFoldablePostures[POSTURE_MAX];

    // For hinges only
    int numHinges;
    enum FoldablePostures foldAtPosture;
    struct FoldableHingeParameters hingeParams[ANDROID_FOLDABLE_MAX_HINGES];
    // For rollables only
    int numRolls;
    enum FoldablePostures resizeAtPosture[ANDROID_FOLDABLE_MAX_DISPLAY_REGIONS];
    struct RollableParameters rollableParams[ANDROID_FOLDABLE_MAX_ROLLS];
};

struct FoldableState {
    struct FoldableConfig config;
    float currentHingeDegrees[ANDROID_FOLDABLE_MAX_HINGES];
    float currentRolledPercent[ANDROID_FOLDABLE_MAX_ROLLS];
    enum FoldablePostures currentPosture;
};

int android_foldable_get_state(struct FoldableState* state);
bool android_foldable_hinge_configured();
bool android_foldable_any_folded_area_configured();
bool android_foldable_folded_area_configured(int area);
bool android_foldable_is_folded();
bool android_foldable_fold();
bool android_foldable_unfold();
bool android_foldable_set_posture(int posture);
bool android_foldable_get_folded_area(int* x, int* y, int* w, int* h);
bool android_foldable_rollable_configured();
bool android_hw_sensors_is_loading_snapshot();
bool android_heart_rate_sensor_configured();
bool android_foldable_posture_name(int posture, char* name);
ANDROID_END_HEADER

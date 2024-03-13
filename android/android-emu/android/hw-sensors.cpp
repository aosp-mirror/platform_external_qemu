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

#include "android/hw-sensors.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "aemu/base/async/CallbackRegistry.h"
#include "aemu/base/memory/LazyInstance.h"
#include "aemu/base/misc/StringUtils.h"
#include "aemu/base/utils/stream.h"
#include "android/automation/AutomationController.h"
#include "android/avd/info.h"
#include "android/console.h"
#include "android/emulation/android_qemud.h"
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/emulation/control/utils/EventSupport.h"
#include "android/emulation/resizable_display_config.h"
#include "android/physics/PhysicalModel.h"
#include "android/sensors-port.h"
#include "android/utils/debug.h"
#include "android/utils/looper.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#include "host-common/FeatureControl.h"

namespace fc = android::featurecontrol;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(sensors, __VA_ARGS__)
#define V(...) VERBOSE_PRINT(init, __VA_ARGS__)

/* define T_ACTIVE to 1 to debug transport communications */
#define T_ACTIVE 0

#if T_ACTIVE
#define T(...) VERBOSE_PRINT(sensors, __VA_ARGS__)
#else
#define T(...) ((void)0)
#endif

/* this code supports emulated sensor hardware
 *
 * Note that currently, only the accelerometer is really emulated, and only
 * for the purpose of allowing auto-rotating the screen in keyboard-less
 * configurations.
 *
 *
 */

/* the version number used for compatibility checks when saving and loading */
#define SENSOR_FILE_VERSION 0

static const struct {
    const char* name;
    int id;
} _sSensors[MAX_SENSORS] = {
#define SENSOR_(x, y, z, v, w) {y, ANDROID_SENSOR_##x},
        SENSORS_LIST
#undef SENSOR_
};

static int _sensorIdFromName(const char* name) {
    int nn;
    for (nn = 0; nn < MAX_SENSORS; nn++)
        if (!strcmp(_sSensors[nn].name, name))
            return _sSensors[nn].id;
    return -1;
}

static const char* _sensorNameFromId(int id) {
    int nn;
    for (nn = 0; nn < MAX_SENSORS; nn++)
        if (id == _sSensors[nn].id)
            return _sSensors[nn].name;
    return NULL;
}

static const struct {
    const char* name;
    int id;
} _sPhysicalParameters[MAX_PHYSICAL_PARAMETERS] = {
#define PHYSICAL_PARAMETER_(x, y, z, w) {y, PHYSICAL_PARAMETER_##x},
        PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
};

static int _physicalParamIdFromName(const char* name) {
    int nn;
    for (nn = 0; nn < MAX_PHYSICAL_PARAMETERS; nn++)
        if (!strcmp(_sPhysicalParameters[nn].name, name))
            return _sPhysicalParameters[nn].id;
    return -1;
}

static const char* _physicalParamNameFromId(int id) {
    int nn;
    for (nn = 0; nn < MAX_SENSORS; nn++)
        if (id == _sPhysicalParameters[nn].id)
            return _sPhysicalParameters[nn].name;
    return NULL;
}

typedef struct {
    unsigned long measurement_id;
    int length;
    char value[128];
} SerializedSensor;

typedef struct {
    bool enabled;
    SerializedSensor serialized;
} Sensor;

/*
 * - when the qemu-specific sensors HAL module starts, it sends
 *   "list-sensors"
 *
 * - this code replies with a string containing an integer corresponding
 *   to a bitmap of available hardware sensors in the current AVD
 *   configuration (e.g. "1" a.k.a (1 << ANDROID_SENSOR_ACCELERATION))
 *
 * - the HAL module sends "set:<sensor>:<flag>" to enable or disable
 *   the report of a given sensor state. <sensor> must be the name of
 *   a given sensor (e.g. "accelerometer"), and <flag> must be either
 *   "1" (to enable) or "0" (to disable).
 *
 * - Once at least one sensor is "enabled", this code should periodically
 *   send information about the corresponding enabled sensors. The default
 *   period is 200ms.
 *
 * - the HAL module sends "set-delay:<delay>", where <delay> is an integer
 *   corresponding to a time delay in milli-seconds. This corresponds to
 *   a new interval between sensor events sent by this code to the HAL
 *   module.
 *
 * - the HAL module can also send a "wake" command. This code should simply
 *   send the "wake" back to the module. This is used internally to wake a
 *   blocking read that happens in a different thread. This ping-pong makes
 *   the code in the HAL module very simple.
 *
 * - each timer tick, this code sends sensor reports in the following
 *   format (each line corresponds to a different line sent to the module):
 *
 *      acceleration:<x>:<y>:<z>
 *      magnetic-field:<x>:<y>:<z>
 *      orientation:<azimuth>:<pitch>:<roll>
 *      temperature:<celsius>
 *      light:<lux>
 *      pressure:<hpa>
 *      humidity:<percent>
 *      sync:<time_us>
 *
 *   Where each line before the sync:<time_us> is optional and will only
 *   appear if the corresponding sensor has been enabled by the HAL module.
 *
 *   Note that <time_us> is the VM time in micro-seconds when the report
 *   was "taken" by this code. This is adjusted by the HAL module to
 *   emulated system time (using the first sync: to compute an adjustment
 *   offset).
 */
#define HEADER_SIZE 4
#define BUFFER_SIZE 512

typedef struct HwSensorClient HwSensorClient;

typedef struct {
    QemudService* service;
    Sensor sensors[MAX_SENSORS];
    PhysicalModel* physical_model;
    HwSensorClient* clients;
    AndroidSensorsPort* sensors_port;
    int64_t time_offset_ns;
} HwSensors;

struct HwSensorClient {
    HwSensorClient* next;
    HwSensors* sensors;
    QemudClient* client;
    LoopTimer* timer;
    uint32_t enabledMask;
    int32_t delay_ms;
};

static android::base::LazyInstance<
        android::emulation::control::EventChangeSupport<void>>
        sSensorChangeCallbackReg = LAZY_INSTANCE_INIT;

class HwSensorChangedCallbackListener
    : public android::emulation::control::EventListener<void> {
public:
    explicit HwSensorChangedCallbackListener(HwSensorChangedCallback callback,
                                             void* opaque)
        : mCallback(callback), mOpaque(opaque) {
        sSensorChangeCallbackReg->addListener(this);
    }

    ~HwSensorChangedCallbackListener() override {
        sSensorChangeCallbackReg->removeListener(this);
    }

    void eventArrived() override { mCallback(mOpaque); }

private:
    HwSensorChangedCallback mCallback;
    void* mOpaque;
};

static std::unordered_map<void*,
                          std::unique_ptr<HwSensorChangedCallbackListener>>
        gSensorListeners;

static void _hwSensorClient_free(HwSensorClient* cl) {
    /* remove from sensors's list */
    if (cl->sensors) {
        HwSensorClient** pnode = &cl->sensors->clients;
        for (;;) {
            HwSensorClient* node = *pnode;
            if (node == NULL)
                break;
            if (node == cl) {
                *pnode = cl->next;
                break;
            }
            pnode = &node->next;
        }
        cl->next = NULL;
        cl->sensors = NULL;
    }

    /* close QEMUD client, if any */
    if (cl->client) {
        qemud_client_close(cl->client);
        cl->client = NULL;
    }
    /* remove timer, if any */
    if (cl->timer) {
        loopTimer_stop(cl->timer);
        loopTimer_free(cl->timer);
        cl->timer = NULL;
    }
    AFREE(cl);
}

/* forward */
static void _hwSensorClient_tick(void* opaque, LoopTimer* timer);

static HwSensorClient* _hwSensorClient_new(HwSensors* sensors) {
    HwSensorClient* cl;

    ANEW0(cl);

    cl->sensors = sensors;
    cl->enabledMask = 0;
    cl->delay_ms = 800;
    cl->timer =
            loopTimer_newWithClock(looper_getForThread(), _hwSensorClient_tick,
                                   cl, LOOPER_CLOCK_VIRTUAL);

    cl->next = sensors->clients;
    sensors->clients = cl;

    return cl;
}

/* forward */

static void _hwSensorClient_receive(HwSensorClient* cl,
                                    uint8_t* query,
                                    int querylen);

/* Qemud service management */

static void _hwSensorClient_recv(void* opaque,
                                 uint8_t* msg,
                                 int msglen,
                                 QemudClient* client) {
    auto cl = static_cast<HwSensorClient*>(opaque);

    _hwSensorClient_receive(cl, msg, msglen);
}

static void _hwSensorClient_close(void* opaque) {
    auto cl = static_cast<HwSensorClient*>(opaque);

    /* the client is already closed here */
    cl->client = NULL;
    _hwSensorClient_free(cl);
}

/* send a one-line message to the HAL module through a qemud channel */
static void _hwSensorClient_send(HwSensorClient* cl,
                                 const uint8_t* msg,
                                 int msglen) {
    D("%s: '%s'", __FUNCTION__, quote_bytes((const char*)msg, msglen));
    qemud_client_send(cl->client, msg, msglen);
}

static int _hwSensorClient_enabled(HwSensorClient* cl, int sensorId) {
    return (cl->enabledMask & (1 << sensorId)) != 0;
}

/* a helper function that replaces commas (,) with points (.).
 * Each sensor string must be processed this way before being
 * sent into the guest. This is because the system locale may
 * cause decimal values to be formatted with a comma instead of
 * a decimal point, but that would not be parsed correctly
 * within the guest.
 */
static void _hwSensorClient_sanitizeSensorString(char* string, int maxlen) {
    for (int i = 0; i < maxlen && string[i] != '\0'; i++) {
        if (string[i] == ',') {
            string[i] = '.';
        }
    }
}

static void serializeValue_float(Sensor* sensor,
                                 const char* format,
                                 float value,
                                 long measurement_id) {
    sensor->serialized.length =
            snprintf(sensor->serialized.value, sizeof(sensor->serialized.value),
                     format, value, measurement_id);
}

static void serializeValue_vec3(Sensor* sensor,
                                const char* format,
                                vec3 value,
                                long measurement_id) {
    sensor->serialized.length =
            snprintf(sensor->serialized.value, sizeof(sensor->serialized.value),
                     format, value.x, value.y, value.z, measurement_id);
}

static void serializeValue_vec4(Sensor* sensor,
                                const char* format,
                                vec4 value,
                                long measurement_id) {
    sensor->serialized.length = snprintf(
            sensor->serialized.value, sizeof(sensor->serialized.value), format,
            value.x, value.y, value.z, value.w, measurement_id);
}

// a function to serialize the sensor value based on its ID
static void serializeSensorValue(PhysicalModel* physical_model,
                                 Sensor* sensor,
                                 AndroidSensor sensor_id) {
    long measurement_id = -1L;

    switch (sensor_id) {
#define ENUM_NAME(x) ANDROID_SENSOR_##x
#define GET_FUNCTION_NAME(x) physicalModel_get##x
#define SERIALIZE_VALUE_NAME(x) serializeValue_##x
#define SENSOR_(x, y, z, v, w)                                         \
    case ENUM_NAME(x): {                                               \
        const v current_value =                                        \
                GET_FUNCTION_NAME(z)(physical_model, &measurement_id); \
        if (measurement_id != sensor->serialized.measurement_id) {     \
            SERIALIZE_VALUE_NAME(v)                                    \
            (sensor, w ":%ld", current_value, measurement_id);         \
        }                                                              \
        break;                                                         \
    }
        SENSORS_LIST
#undef SENSOR_
#undef SERIALIZE_VALUE_NAME
#undef GET_FUNCTION_NAME
#undef ENUM_NAME
        default:
            assert(false);  // should never happen
            return;
    }
    assert(sensor->serialized.length < sizeof(sensor->serialized.value));

    if (measurement_id != sensor->serialized.measurement_id) {
        _hwSensorClient_sanitizeSensorString(sensor->serialized.value,
                                             sensor->serialized.length);
    }
    sensor->serialized.measurement_id = measurement_id;
}

/* this function is called periodically to send sensor reports
 * to the HAL module, and re-arm the timer if necessary
 */
static void _hwSensorClient_tick(void* opaque, LoopTimer* unused) {
    auto cl = static_cast<HwSensorClient*>(opaque);
    int64_t delay_ms = cl->delay_ms;
    const uint32_t mask = cl->enabledMask;

    // Grab the guest time before sending any sensor data:
    // the android.hardware CTS requires sync times to be no greater than the
    // time of the sensor event arrival. Since the CTS enforces this property,
    // other code may also rely on it.
    const DurationNs now_ns =
            android::automation::AutomationController::get().advanceTime();

    for (size_t sensor_id = 0; sensor_id < MAX_SENSORS; ++sensor_id) {
        if (!_hwSensorClient_enabled(cl, sensor_id)) {
            continue;
        }
        Sensor* sensor = &cl->sensors->sensors[sensor_id];

        serializeSensorValue(cl->sensors->physical_model, sensor,
                             (AndroidSensor)sensor_id);
        _hwSensorClient_send(cl, (uint8_t*)sensor->serialized.value,
                             sensor->serialized.length);
    }

    char buffer[64];
    int buffer_len = snprintf(buffer, sizeof(buffer), "guest-sync:%" PRId64,
                              ((int64_t)now_ns) + cl->sensors->time_offset_ns);
    assert(buffer_len < sizeof(buffer));
    _hwSensorClient_send(cl, (uint8_t*)buffer, buffer_len);

    buffer_len =
            snprintf(buffer, sizeof(buffer), "sync:%" PRId64, now_ns / 1000);
    assert(buffer_len < sizeof(buffer));
    _hwSensorClient_send(cl, (uint8_t*)buffer, buffer_len);

    if (mask == 0)
        return;

    // Rearm the timer to fire a little bit early, so we can sustain the
    // requested frequency. Also make sure we have at least a minimal delay,
    // otherwise this timer would hijack the main loop thread and won't allow
    // guest to ever run.
    // Note: While there is some overhead in this code, it is signifcantly less
    //  than 1ms. Just delay by exactly (delay_ms) below to keep the actual rate
    //  as close to the desired rate as possible.
    // Note2: Let's cap the minimal tick interval to 10ms, to make sure:
    // - We never overload the main QEMU loop.
    // - Some CTS hardware test cases require a limit on the maximum update
    // rate,
    //   which has been known to be in the low 100's of Hz.
    if (delay_ms < 10) {
        delay_ms = 10;
    } else if (delay_ms > 60 * 60 * 1000) {
        delay_ms = 60 * 60 * 1000;
    }
    loopTimer_startRelative(cl->timer, delay_ms);
}

/* handle incoming messages from the HAL module */
static void _hwSensorClient_receive(HwSensorClient* cl,
                                    uint8_t* msg,
                                    int msglen) {
    HwSensors* hw = cl->sensors;

    D("%s: '%.*s'", __FUNCTION__, msglen, (char*) msg);

    /* "list-sensors" is used to get an integer bit map of
     * available emulated sensors. We compute the mask from the
     * current hardware configuration.
     */
    if (msglen == 12 && !memcmp(msg, "list-sensors", 12)) {
        char buff[12];
        int mask = 0;
        int nn;

        for (nn = 0; nn < MAX_SENSORS; nn++) {
            if (hw->sensors[nn].enabled) {
                mask |= (1 << nn);
            }
        }

        const int len = snprintf(buff, sizeof buff, "%d", mask);
        assert(len < sizeof(buff));
        _hwSensorClient_send(cl, (const uint8_t*)buff, len);
        return;
    }

    /* "wake" is a special message that must be sent back through
     * the channel. It is used to exit a blocking read.
     */
    if (msglen == 4 && !memcmp(msg, "wake", 4)) {
        _hwSensorClient_send(cl, (const uint8_t*)"wake", 4);
        return;
    }

    /* "set-delay:<delay>" is used to set the delay in milliseconds
     * between sensor events
     */
    if (msglen > 10 && !memcmp(msg, "set-delay:", 10)) {
        cl->delay_ms = atoi((const char*)msg + 10);
        if (cl->enabledMask != 0)
            _hwSensorClient_tick(cl, cl->timer);

        return;
    }

    /* "set:<name>:<state>" is used to enable/disable a given
     * sensor. <state> must be 0 or 1
     */
    if (msglen > 4 && !memcmp(msg, "set:", 4)) {
        char* q;
        int id, enabled, oldEnabledMask = cl->enabledMask;
        msg += 4;
        q = strchr((char*)msg, ':');
        if (q == NULL) { /* should not happen */
            D("%s: ignore bad 'set' command", __FUNCTION__);
            return;
        }
        *q++ = 0;

        id = _sensorIdFromName((const char*)msg);
        if (id < 0 || id >= MAX_SENSORS) {
            D("%s: ignore unknown sensor name '%s'", __FUNCTION__, (char*) msg);
            return;
        }

        if (!hw->sensors[id].enabled) {
            D("%s: trying to set disabled %s sensor", __FUNCTION__, (char*) msg);
            return;
        }
        enabled = (q[0] == '1');

        if (enabled)
            cl->enabledMask |= (1 << id);
        else
            cl->enabledMask &= ~(1 << id);

        if (cl->enabledMask != (uint32_t)oldEnabledMask) {
            D("%s %s sensor",
              (cl->enabledMask & (1 << id)) ? "enabling" : "disabling", (char*) msg);
        }

        /* If emulating device is connected update sensor state there too. */
        if (hw->sensors_port != NULL) {
            if (enabled) {
                sensors_port_enable_sensor(hw->sensors_port, (const char*)msg);
            } else {
                sensors_port_disable_sensor(hw->sensors_port, (const char*)msg);
            }
        }

        _hwSensorClient_tick(cl, cl->timer);
        return;
    }

    /* "time:time_ns" is used to set a synchronization point for sensor
     * data
     */
    if (msglen > 5 && !memcmp(msg, "time:", 5)) {
        msg += 5;
        int64_t guest_time_ns;
        if (sscanf((const char*)msg, "%" PRId64, &guest_time_ns) !=
            1) { /* should not happen */
            D("%s: ignore bad 'time' command", __FUNCTION__);
            return;
        }

        const DurationNs now_ns = looper_nowNsWithClock(looper_getForThread(),
                                                        LOOPER_CLOCK_VIRTUAL);

        hw->time_offset_ns = guest_time_ns - now_ns;
        return;
    }

    D("%s: ignoring unknown query", __FUNCTION__);
}

/* Saves sensor-specific client data to snapshot */
static void _hwSensorClient_save(Stream* f, QemudClient* client, void* opaque) {
    auto sc = static_cast<HwSensorClient*>(opaque);

    stream_put_be32(f, sc->delay_ms);
    stream_put_be32(f, sc->enabledMask);
    stream_put_timer(f, sc->timer);
}

/* Loads sensor-specific client data from snapshot */
static int _hwSensorClient_load(Stream* f, QemudClient* client, void* opaque) {
    auto sc = static_cast<HwSensorClient*>(opaque);

    sc->delay_ms = stream_get_be32(f);
    sc->enabledMask = stream_get_be32(f);
    stream_get_timer(f, sc->timer);

    return 0;
}

static QemudClient* _hwSensors_connect(void* opaque,
                                       QemudService* service,
                                       int channel,
                                       const char* client_param) {
    auto sensors = static_cast<HwSensors*>(opaque);
    HwSensorClient* cl = _hwSensorClient_new(sensors);
    QemudClient* client = qemud_client_new(
            service, channel, client_param, cl, _hwSensorClient_recv,
            _hwSensorClient_close, _hwSensorClient_save, _hwSensorClient_load);
    qemud_client_set_framing(client, 1);
    cl->client = client;

    return client;
}

vec3 get_vec3_value(const float* val, const size_t count) {
    return vec3{count > 0 ? val[0] : 0, count > 1 ? val[1] : 0,
                count > 2 ? val[2] : 0};
}

vec4 get_vec4_value(const float* val, const size_t count) {
    return vec4{count > 0 ? val[0] : 0, count > 1 ? val[1] : 0,
                count > 2 ? val[2] : 0, count > 3 ? val[3] : 0};
}

float get_float_value(const float* val, const size_t count) {
    return count > 0 ? val[0] : 0;
}

/* change the value of the emulated sensor vector */
static void _hwSensors_setSensorValue(HwSensors* h,
                                      int sensor_id,
                                      const float* val,
                                      const size_t count) {
    switch (sensor_id) {
#define OVERRIDE_FUNCTION_NAME(x) physicalModel_override##x
#define GET_TYPE_VALUE_FUNCTION_NAME(x) get_##x##_value
#define ENUM_NAME(x) ANDROID_SENSOR_##x
#define SENSOR_(x, y, z, v, w)                                            \
    case ENUM_NAME(x):                                                    \
        OVERRIDE_FUNCTION_NAME(z)                                         \
        (h->physical_model, GET_TYPE_VALUE_FUNCTION_NAME(v)(val, count)); \
        break;
        SENSORS_LIST
#undef SENSOR_
#undef ENUM_NAME
#undef GET_TYPE_VALUE_FUNCTION_NAME
#undef OVERRIDE_FUNCTION_NAME
        default:
            assert(false);  // should never happen
            break;
    }
}

void vec3_get_size(size_t* size) {
    if (size != nullptr) {
        *size = 3;
    }
}

void vec4_get_size(size_t* size) {
    if (size != nullptr) {
        *size = 4;
    }
}

void float_get_size(size_t* size) {
    if (size != nullptr) {
        *size = 1;
    }
}

void vec3_get_values(const vec3 value, float* const* out, const size_t count) {
    if (count > 0)
        *out[0] = value.x;
    if (count > 1)
        *out[1] = value.y;
    if (count > 2)
        *out[2] = value.z;
}

void vec4_get_values(const vec4 value, float* const* out, const size_t count) {
    if (count > 0) {
        *out[0] = value.x;
    }
    if (count > 1) {
        *out[1] = value.y;
    }
    if (count > 2) {
        *out[2] = value.z;
    }
    if (count > 3) {
        *out[3] = value.w;
    }
}

void float_get_values(const float value,
                      float* const* out,
                      const size_t count) {
    if (count > 0) {
        *out[0] = value;
    }
}

/* get the value of the emulated sensor vector */
static void _hwSensors_getSensorValue(HwSensors* h,
                                      int sensor_id,
                                      float* const* out,
                                      const size_t count) {
    long measurement_id;
    switch (sensor_id) {
#define GET_FUNCTION_NAME(x) physicalModel_get##x
#define TYPE_GET_VALUES_FUNCTION_NAME(x) x##_get_values
#define ENUM_NAME(x) ANDROID_SENSOR_##x
#define SENSOR_(x, y, z, v, w)                                          \
    case ENUM_NAME(x):                                                  \
        TYPE_GET_VALUES_FUNCTION_NAME(v)                                \
        (GET_FUNCTION_NAME(z)(h->physical_model, &measurement_id), out, \
         count);                                                        \
        break;
        SENSORS_LIST
#undef SENSOR_
#undef ENUM_NAME
#undef TYPE_GET_VALUES_FUNCTION_NAME
#undef GET_FUNCTION_NAME
        default:
            assert(false);  // should never happen
            break;
    }
}

/* get the size of the emulated sensor vector */
static void _hwSensors_getSensorValueSize(HwSensors* h,
                                          int sensor_id,
                                          size_t* size) {
    long measurement_id;
    switch (sensor_id) {
#define GET_FUNCTION_NAME(x) physicalModel_get##x
#define TYPE_GET_VALUES_FUNCTION_NAME(x) x##_get_size
#define ENUM_NAME(x) ANDROID_SENSOR_##x
#define SENSOR_(x, y, z, v, w)           \
    case ENUM_NAME(x):                   \
        TYPE_GET_VALUES_FUNCTION_NAME(v) \
        (size);                          \
        break;
        SENSORS_LIST
#undef SENSOR_
#undef ENUM_NAME
#undef TYPE_GET_VALUES_FUNCTION_NAME
#undef GET_FUNCTION_NAME
        default:
            assert(false);  // should never happen
            break;
    }
}

/* change the value of the physical parameter */
static void _hwSensors_setPhysicalParameterValue(HwSensors* h,
                                                 int parameter_id,
                                                 const float* val,
                                                 const size_t count,
                                                 int interpolation_mode) {
    switch (parameter_id) {
#define ENUM_NAME(x) PHYSICAL_PARAMETER_##x
#define GET_TYPE_VALUE_FUNCTION_NAME(x) get_##x##_value
#define SET_TARGET_FUNCTION_NAME(x) physicalModel_setTarget##x
#define PHYSICAL_PARAMETER_(x, y, z, w)                                  \
    case ENUM_NAME(x):                                                   \
        SET_TARGET_FUNCTION_NAME(z)                                      \
        (h->physical_model, GET_TYPE_VALUE_FUNCTION_NAME(w)(val, count), \
         (PhysicalInterpolation)interpolation_mode);                     \
        break;
        PHYSICAL_PARAMETERS_LIST
#undef PHYSICAL_PARAMETER_
#undef SET_TARGET_FUNCTION_NAME
#undef GET_TYPE_VALUE_FUNCTION_NAME
#undef ENUM_NAME
        default:
            assert(false);  // should never happen
            break;
    }

    sSensorChangeCallbackReg->fireEvent();
}

/* get the value of the physical parameter */
static void _hwSensors_getPhysicalParameterValue(
        HwSensors* h,
        int parameter_id,
        float* const* out,
        const size_t count,
        ParameterValueType parameter_value_type) {
    switch (parameter_id) {
#define ENUM_NAME(x) PHYSICAL_PARAMETER_##x
#define TYPE_GET_VALUES_FUNCTION_NAME(x) x##_get_values
#define GET_PARAMETER_FUNCTION_NAME(x) physicalModel_getParameter##x
#define PHYSICAL_PARAMETER_(x, y, z, w)                        \
    case ENUM_NAME(x):                                         \
        TYPE_GET_VALUES_FUNCTION_NAME(w)                       \
        (GET_PARAMETER_FUNCTION_NAME(z)(h->physical_model,     \
                                        parameter_value_type), \
         out, count);                                          \
        break;
        PHYSICAL_PARAMETERS_LIST
#undef GET_PARAMETER_FUNCTION_NAME
#undef TYPE_GET_VALUES_FUNCTION_NAME
#undef ENUM_NAME
#undef PHYSICAL_PARAMETER_
        default:
            assert(false);  // should never happen
            break;
    }
}

/* get the value of the physical parameter */
static void _hwSensors_getPhysicalParameterValueSize(HwSensors* h,
                                                     int parameter_id,
                                                     size_t* size) {
    switch (parameter_id) {
#define ENUM_NAME(x) PHYSICAL_PARAMETER_##x
#define TYPE_GET_VALUES_FUNCTION_NAME(x) x##_get_size
#define GET_PARAMETER_FUNCTION_NAME(x) physicalModel_getParameter##x
#define PHYSICAL_PARAMETER_(x, y, z, w)  \
    case ENUM_NAME(x):                   \
        TYPE_GET_VALUES_FUNCTION_NAME(w) \
        (size);                          \
        break;
        PHYSICAL_PARAMETERS_LIST
#undef GET_PARAMETER_FUNCTION_NAME
#undef TYPE_GET_VALUES_FUNCTION_NAME
#undef ENUM_NAME
#undef PHYSICAL_PARAMETER_
        default:
            assert(false);  // should never happen
            break;
    }
}

/*
 * Saves available sensors and physical state to allow checking availability
 * when loaded.
 */
static void _hwSensors_save(Stream* f, QemudService* sv, void* opaque) {
    auto h = static_cast<HwSensors*>(opaque);

    /* save a version number for compatibility. */
    stream_put_be32(f, SENSOR_FILE_VERSION);

    /* number of sensors. */
    stream_put_be32(f, MAX_SENSORS);
    for (size_t sensor_id = 0; sensor_id < MAX_SENSORS; sensor_id++) {
        Sensor* s = &h->sensors[sensor_id];
        stream_put_be32(f, s->enabled);
    }

    physicalModel_snapshotSave(h->physical_model, f);
}

static int _hwSensors_load(Stream* f, QemudService* s, void* opaque) {
    auto h = static_cast<HwSensors*>(opaque);

    /* load version number to check compatibility. */
    const int32_t version_number = stream_get_be32(f);
    if (version_number != SENSOR_FILE_VERSION) {
        D("%s: cannot load: snapshot requires file version %d.  Saved file is "
          "version %d",
          __FUNCTION__, SENSOR_FILE_VERSION, version_number);
        return -EIO;
    }

    /* check number of sensors */
    const int32_t num_sensors = stream_get_be32(f);
    if (num_sensors > MAX_SENSORS) {
        D("%s: cannot load: snapshot requires %d sensors, %d available",
          __FUNCTION__, num_sensors, MAX_SENSORS);
        return -EIO;
    }

    /* load sensor state, but ignore legacy values*/
    size_t sensor_id;
    for (sensor_id = 0; sensor_id < num_sensors; sensor_id++) {
        Sensor* s = &h->sensors[sensor_id];
        s->enabled = stream_get_be32(f);
    }

    /* The following is necessary when we resume a snaphost
     * created by an older version of the emulator that provided
     * less hardware sensors.
     */
    for (; sensor_id < MAX_SENSORS; sensor_id++) {
        h->sensors[sensor_id].enabled = false;
    }

    const int loadResult = physicalModel_snapshotLoad(h->physical_model, f);
    android::automation::AutomationController::get().reset();
    return loadResult;
}

/* change the coarse orientation (landscape/portrait) of the emulated device */
static void _hwSensors_setCoarseOrientation(HwSensors* h,
                                            AndroidCoarseOrientation orient,
                                            float tilt_degrees) {
    /* The Android framework computes the orientation by looking at
     * the accelerometer sensor (*not* the orientation sensor !)
     *
     * That's because the gravity is a constant 9.81 vector that
     * can be determined quite easily.
     *
     * Also, for some reason, the framework code considers that the phone should
     * be inclined by 30 degrees along the phone's X axis to be considered
     * in its ideal "vertical" position
     *
     * If the phone is completely vertical, rotating it will not do anything !
     */
    VERBOSE_INFO(rotation, "setCoarseOrientation - HwSensors %p", h);
    std::vector<float> rotation;
    switch (orient) {
        case ANDROID_COARSE_PORTRAIT:
            VERBOSE_INFO(rotation, "Setting coarse orientation to portrait");
            rotation = {-tilt_degrees, 0.f, 0.f};
            break;

        case ANDROID_COARSE_REVERSE_LANDSCAPE:
            VERBOSE_INFO(rotation,
                         "Setting coarse orientation to reverse landscape");
            rotation = {0.f, -tilt_degrees, -90.f};
            break;

        case ANDROID_COARSE_REVERSE_PORTRAIT:
            VERBOSE_INFO(rotation,
                         "Setting coarse orientation to reverse portrait");
            rotation = {tilt_degrees, 0.f, 180.f};
            break;

        case ANDROID_COARSE_LANDSCAPE:
            VERBOSE_INFO(rotation, "Setting coarse orientation to landscape");
            rotation = {0.f, tilt_degrees, 90.f};
            break;
        default:
            VERBOSE_INFO(rotation, "Invalid orientation");
            return;
    }

    _hwSensors_setPhysicalParameterValue(h, PHYSICAL_PARAMETER_ROTATION,
                                         rotation.data(), rotation.size(),
                                         PHYSICAL_INTERPOLATION_STEP);
}

/* initialize the sensors state */
static void _hwSensors_init(HwSensors* h) {
    h->sensors_port = NULL;

    h->service = qemud_service_register("sensors", 0, h, _hwSensors_connect,
                                        _hwSensors_save, _hwSensors_load);

    if (h->physical_model != NULL) {
        physicalModel_free(h->physical_model);
        h->physical_model = NULL;
    }
    h->physical_model = physicalModel_new();

    if (getConsoleAgents()->settings->hw()->hw_accelerometer) {
        h->sensors[ANDROID_SENSOR_ACCELERATION].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_accelerometer_uncalibrated) {
        h->sensors[ANDROID_SENSOR_ACCELERATION_UNCALIBRATED].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_gyroscope) {
        h->sensors[ANDROID_SENSOR_GYROSCOPE].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_proximity) {
        h->sensors[ANDROID_SENSOR_PROXIMITY].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_magnetic_field) {
        h->sensors[ANDROID_SENSOR_MAGNETIC_FIELD].enabled = true;
    }

    if (getConsoleAgents()
                ->settings->hw()
                ->hw_sensors_magnetic_field_uncalibrated) {
        h->sensors[ANDROID_SENSOR_MAGNETIC_FIELD_UNCALIBRATED].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_gyroscope_uncalibrated) {
        h->sensors[ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_orientation) {
        h->sensors[ANDROID_SENSOR_ORIENTATION].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_temperature) {
        h->sensors[ANDROID_SENSOR_TEMPERATURE].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_light) {
        h->sensors[ANDROID_SENSOR_LIGHT].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_pressure) {
        h->sensors[ANDROID_SENSOR_PRESSURE].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_humidity) {
        h->sensors[ANDROID_SENSOR_HUMIDITY].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_rgbclight) {
        h->sensors[ANDROID_SENSOR_RGBC_LIGHT].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensor_hinge) {
        h->sensors[ANDROID_SENSOR_HINGE_ANGLE0].enabled = true;
        switch (getConsoleAgents()->settings->hw()->hw_sensor_hinge_count) {
            case 3:
                h->sensors[ANDROID_SENSOR_HINGE_ANGLE2].enabled = true;
            case 2:
                h->sensors[ANDROID_SENSOR_HINGE_ANGLE1].enabled = true;
            default:;
        }
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_heart_rate ||
        (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) ==
                 AVD_WEAR &&
         avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()) >= 28)) {
        h->sensors[ANDROID_SENSOR_HEART_RATE].enabled = true;
    }

    if (getConsoleAgents()->settings->hw()->hw_sensors_wrist_tilt ||
        (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) ==
                 AVD_WEAR &&
         avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()) >= 28)) {
        h->sensors[ANDROID_SENSOR_WRIST_TILT].enabled = true;
    }

    /* XXX: TODO: Add other tests when we add the corresponding
     * properties to hardware-properties.ini et al. */

    const float kPressure = 1013.25F;
    _hwSensors_setPhysicalParameterValue(
            h, PHYSICAL_PARAMETER_PRESSURE, &kPressure, 1u,
            PHYSICAL_INTERPOLATION_SMOOTH);  // One "standard atmosphere"

    const float kProximity = 1.F;
    _hwSensors_setPhysicalParameterValue(h, PHYSICAL_PARAMETER_PROXIMITY,
                                         &kProximity, 1u,
                                         PHYSICAL_INTERPOLATION_STEP);
}

static HwSensors _sensorsState[1] = {};
static const QAndroidEmulatorWindowAgent* _windowAgent = NULL;

void android_hw_sensors_init(const QAndroidEmulatorWindowAgent* windowAgent) {
    HwSensors* hw = _sensorsState;
    if (windowAgent != NULL) {
        _windowAgent = windowAgent;
    }

    if (hw->service == NULL) {
        _hwSensors_init(hw);
        D("%s: sensors qemud service initialized", __FUNCTION__);
    }
}

const QAndroidEmulatorWindowAgent* android_hw_sensors_get_window_agent() {
    return _windowAgent;
}

void android_hw_sensors_init_remote_controller(void) {
    HwSensors* hw = _sensorsState;

    if (!hw->sensors_port) {
        /* Try to see if there is a device attached that can be used for
         * sensor emulation. */
        hw->sensors_port = sensors_port_create(hw);
        if (hw->sensors_port == NULL) {
            V("Realistic sensor emulation is not available, since the remote "
              "controller is not accessible: %s",
              strerror(errno));
        }
    }
}

/* change the coarse orientation value */
extern int android_sensors_set_coarse_orientation(
        AndroidCoarseOrientation orient,
        float tilt_degrees) {
    android_hw_sensors_init(NULL);
    _hwSensors_setCoarseOrientation(_sensorsState, orient, tilt_degrees);
    return 0;
}

/* Get the coarse orientation value */
extern AndroidCoarseOrientation android_sensors_get_coarse_orientation() {
    android_hw_sensors_init(NULL);
    float x = 0;
    float y = 0;
    float z = 0;
    float* rotation[] = {&x, &y, &z};
    android_sensors_get(ANDROID_SENSOR_ACCELERATION, rotation, 3);

    bool needRotateImage = false;
    if (fabs(x) < fabs(y)) {
        if (y < 0) {
            return ANDROID_COARSE_REVERSE_PORTRAIT;
        } else {
            return ANDROID_COARSE_PORTRAIT;
        }
    } else {
        if (x < 0) {
            return ANDROID_COARSE_REVERSE_LANDSCAPE;
        } else {
            return ANDROID_COARSE_LANDSCAPE;
        }
    }
}

/* Get sensor name from sensor id */
extern const char* android_sensors_get_name_from_id(int sensor_id) {
    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return NULL;

    return _sensorNameFromId(sensor_id);
}

/* Get sensor id from sensor name */
extern int android_sensors_get_id_from_name(const char* sensorname) {
    HwSensors* hw = _sensorsState;

    if (sensorname == NULL)
        return SENSOR_STATUS_UNKNOWN;

    int id = _sensorIdFromName(sensorname);

    if (id < 0 || id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    if (hw->service != NULL) {
        if (!hw->sensors[id].enabled)
            return SENSOR_STATUS_DISABLED;
    } else
        return SENSOR_STATUS_NO_SERVICE;

    return id;
}

/* Interface of reading the data for all sensors */
extern int android_sensors_get(int sensor_id,
                               float* const* out,
                               const size_t count) {
    HwSensors* hw = _sensorsState;

    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    Sensor* sensor = &hw->sensors[sensor_id];
    if (hw->service != NULL) {
        if (!sensor->enabled)
            return SENSOR_STATUS_DISABLED;
    } else {
        return SENSOR_STATUS_NO_SERVICE;
    }

    _hwSensors_getSensorValue(hw, sensor_id, out, count);

    return SENSOR_STATUS_OK;
}

extern int android_sensors_get_size(int sensor_id, size_t* size) {
    HwSensors* hw = _sensorsState;

    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    Sensor* sensor = &hw->sensors[sensor_id];
    if (hw->service != NULL) {
        if (!sensor->enabled)
            return SENSOR_STATUS_DISABLED;
    } else {
        return SENSOR_STATUS_NO_SERVICE;
    }

    _hwSensors_getSensorValueSize(hw, sensor_id, size);

    return SENSOR_STATUS_OK;
}

/* Interface of setting the data for all sensors */
extern int android_sensors_override_set(int sensor_id,
                                        const float* val,
                                        const size_t count) {
    HwSensors* hw = _sensorsState;

    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    if (hw->service != NULL) {
        if (!hw->sensors[sensor_id].enabled) {
            return SENSOR_STATUS_DISABLED;
        }
    } else {
        return SENSOR_STATUS_NO_SERVICE;
    }

    _hwSensors_setSensorValue(hw, sensor_id, val, count);

    return SENSOR_STATUS_OK;
}

/* Get Sensor from sensor id */
extern uint8_t android_sensors_get_sensor_status(int sensor_id) {
    HwSensors* hw = _sensorsState;

    if (sensor_id < 0 || sensor_id >= MAX_SENSORS)
        return SENSOR_STATUS_UNKNOWN;

    return hw->sensors[sensor_id].enabled;
}

/* Get guest time offset for sensors */
extern int64_t android_sensors_get_time_offset() {
    HwSensors* hw = _sensorsState;
    return hw->time_offset_ns;
}

/* Get the current sensor delay (determines update rate) */
extern int32_t android_sensors_get_delay_ms() {
    int min_delay = INT32_MAX;
    HwSensors* hw = _sensorsState;
    HwSensorClient* client = hw->clients;
    while (client) {
        if (client->delay_ms < min_delay) {
            min_delay = client->delay_ms;
        }
        client = client->next;
    }

    return min_delay;
}

/* Get the physical model instance. */
extern PhysicalModel* android_physical_model_instance() {
    HwSensors* hw = _sensorsState;

    return hw->physical_model;
}

/* Get a physical model parameter target value*/
extern int android_physical_model_get(int physical_parameter,
                                      float* const* out,
                                      const size_t count,
                                      ParameterValueType parameter_value_type) {
    HwSensors* hw = _sensorsState;

    if (physical_parameter < 0 || physical_parameter >= MAX_PHYSICAL_PARAMETERS)
        return PHYSICAL_PARAMETER_STATUS_UNKNOWN;

    if (hw->physical_model == NULL) {
        return PHYSICAL_PARAMETER_STATUS_NO_SERVICE;
    }

    _hwSensors_getPhysicalParameterValue(hw, physical_parameter, out, count,
                                         parameter_value_type);

    return PHYSICAL_PARAMETER_STATUS_OK;
}

/* Get a physical model parameter target value*/
extern int android_physical_model_get_size(int physical_parameter,
                                           size_t* size) {
    HwSensors* hw = _sensorsState;

    if (physical_parameter < 0 || physical_parameter >= MAX_PHYSICAL_PARAMETERS)
        return PHYSICAL_PARAMETER_STATUS_UNKNOWN;

    if (hw->physical_model == NULL) {
        return PHYSICAL_PARAMETER_STATUS_NO_SERVICE;
    }

    _hwSensors_getPhysicalParameterValueSize(hw, physical_parameter, size);

    return PHYSICAL_PARAMETER_STATUS_OK;
}

/* Set a physical model parameter */
extern int android_physical_model_set(int physical_parameter,
                                      const float* val,
                                      const size_t count,
                                      int interpolation_mode) {
    HwSensors* hw = _sensorsState;

    if (physical_parameter < 0 || physical_parameter >= MAX_PHYSICAL_PARAMETERS)
        return PHYSICAL_PARAMETER_STATUS_UNKNOWN;

    if (hw->physical_model == NULL) {
        return PHYSICAL_PARAMETER_STATUS_NO_SERVICE;
    }

    _hwSensors_setPhysicalParameterValue(hw, physical_parameter, val, count,
                                         interpolation_mode);
    return PHYSICAL_PARAMETER_STATUS_OK;
}

/* Set agent to receive physical state change callbacks */
extern int android_physical_agent_set(
        const struct QAndroidPhysicalStateAgent* agent) {
    HwSensors* hw = _sensorsState;

    physicalModel_setPhysicalStateAgent(hw->physical_model, agent);
    return SENSOR_STATUS_OK;
}

/* Get physical parameter id from name */
extern int android_physical_model_get_parameter_id_from_name(
        const char* physical_parameter_name) {
    if (physical_parameter_name == NULL)
        return PHYSICAL_PARAMETER_STATUS_UNKNOWN;

    const int id = _physicalParamIdFromName(physical_parameter_name);

    if (id < 0 || id >= MAX_PHYSICAL_PARAMETERS)
        return PHYSICAL_PARAMETER_STATUS_UNKNOWN;

    return id;
}

/* Get physical parameter name from id */
extern const char* android_physical_model_get_parameter_name_from_id(
        int physical_parameter_id) {
    if (physical_parameter_id < 0 ||
        physical_parameter_id >= MAX_PHYSICAL_PARAMETERS)
        return NULL;

    return _physicalParamNameFromId(physical_parameter_id);
}

// Start recording ground truth to the specified file.
extern int android_physical_model_record_ground_truth(const char* file_name) {
    HwSensors* hw = _sensorsState;

    return physicalModel_recordGroundTruth(hw->physical_model, file_name);
}

// Stop ground truth recording.
extern int android_physical_model_stop_recording() {
    HwSensors* hw = _sensorsState;

    return physicalModel_stopRecording(hw->physical_model);
}

int android_foldable_get_posture() {
    struct FoldableState state;
    constexpr int unknown_posture = static_cast<int>(POSTURE_UNKNOWN);
    if (android_foldable_get_state(&state) >= 0) {
        const auto posture = state.currentPosture;
        if (posture > POSTURE_UNKNOWN && posture < POSTURE_MAX) {
            return static_cast<int>(posture);
        }
    }
    return unknown_posture;
}

int android_foldable_get_state(struct FoldableState* state) {
    return physicalModel_getFoldableState(android_physical_model_instance(),
                                          state);
}

bool android_foldable_hinge_configured() {
    return (getConsoleAgents()->settings->hw()->hw_sensor_hinge &&
            getConsoleAgents()->settings->hw()->hw_sensor_hinge_count > 0);
}

bool android_foldable_is_pixel_fold() {
    if (resizableEnabled34()) {
        return true;
    }
    const auto devname = getConsoleAgents()->settings->hw()->hw_device_name;
    return (devname && "pixel_fold" == std::string(devname) &&
            fc::isEnabled(fc::SupportPixelFold));
}

int android_foldable_pixel_fold_second_display_id() {
    constexpr int PIXEL_FOLD_SECOND_DISPLAY = 6;
    return PIXEL_FOLD_SECOND_DISPLAY;
}

// We still need to discuss how to support foldable for secondary displays
bool android_foldable_hinge_enabled() {
    return ((android_foldable_hinge_configured() ||
             android_foldable_folded_area_configured(0) ||
             android_foldable_rollable_configured()) &&
            (!resizableEnabled() ||
             getResizableActiveConfigId() == PRESET_SIZE_UNFOLDED));
}

bool android_foldable_any_folded_area_configured() {
    for (int i = 0; i < ANDROID_FOLDABLE_MAX_DISPLAY_REGIONS; i++) {
        if (android_foldable_folded_area_configured(i)) {
            return true;
        }
    }
    return false;
}

bool android_foldable_folded_area_configured(int area) {
    if (area < 0 || area >= ANDROID_FOLDABLE_MAX_DISPLAY_REGIONS) {
        return false;
    }
    int xOffset, yOffset, width, height;
    switch (area) {
        case 0:
            xOffset = getConsoleAgents()
                              ->settings->hw()
                              ->hw_displayRegion_0_1_xOffset;
            yOffset = getConsoleAgents()
                              ->settings->hw()
                              ->hw_displayRegion_0_1_yOffset;
            width = getConsoleAgents()
                            ->settings->hw()
                            ->hw_displayRegion_0_1_width;
            height = getConsoleAgents()
                             ->settings->hw()
                             ->hw_displayRegion_0_1_height;
            break;
        case 1:
            xOffset = getConsoleAgents()
                              ->settings->hw()
                              ->hw_displayRegion_0_2_xOffset;
            yOffset = getConsoleAgents()
                              ->settings->hw()
                              ->hw_displayRegion_0_2_yOffset;
            width = getConsoleAgents()
                            ->settings->hw()
                            ->hw_displayRegion_0_2_width;
            height = getConsoleAgents()
                             ->settings->hw()
                             ->hw_displayRegion_0_2_height;
            break;
        case 2:
            xOffset = getConsoleAgents()
                              ->settings->hw()
                              ->hw_displayRegion_0_3_xOffset;
            yOffset = getConsoleAgents()
                              ->settings->hw()
                              ->hw_displayRegion_0_3_yOffset;
            width = getConsoleAgents()
                            ->settings->hw()
                            ->hw_displayRegion_0_3_width;
            height = getConsoleAgents()
                             ->settings->hw()
                             ->hw_displayRegion_0_3_height;
            break;
        default:
            E("Invalid area %d", area);
    }
    bool enableFoldableByDefault = !(
            xOffset < 0 || xOffset > 9999 || yOffset < 0 || yOffset > 9999 ||
            width < 1 || width > 9999 || height < 1 || height > 9999 ||
            avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()) < 28);

    return enableFoldableByDefault;
}

bool android_foldable_is_folded() {
    return physicalModel_foldableisFolded(android_physical_model_instance());
}

bool android_foldable_fold() {
    if (!android_foldable_folded_area_configured(0)) {
        return false;
    }
    if (android_foldable_rollable_configured()) {
        // "fold" not clear for rollable
        return false;
    }
    struct FoldableState state;
    if (android_foldable_get_state(&state) < 0) {
        return false;
    }
    auto posture = {(float)state.config.foldAtPosture};
    return android_physical_model_set(PHYSICAL_PARAMETER_POSTURE,
                                      posture.begin(), posture.size(),
                                      PHYSICAL_INTERPOLATION_SMOOTH) >= 0;
}

bool android_foldable_unfold() {
    if (!android_foldable_folded_area_configured(0)) {
        return false;
    }
    if (android_foldable_rollable_configured()) {
        // "unfold" not clear for rollable
        return false;
    }
    auto posture = {(float)POSTURE_OPENED};
    return android_physical_model_set(PHYSICAL_PARAMETER_POSTURE,
                                      posture.begin(), posture.size(),
                                      PHYSICAL_INTERPOLATION_SMOOTH) >= 0;
}

bool android_foldable_set_posture(int posture) {
    if (!android_foldable_hinge_configured()) {
        return false;
    }
    struct FoldableState state;
    if (android_foldable_get_state(&state) < 0) {
        return false;
    }
    if (posture <= POSTURE_UNKNOWN || posture >= POSTURE_MAX) {
        return false;
    }
    if (!state.config.supportedFoldablePostures[posture]) {
        return false;
    }
    auto f_posture = static_cast<float>(posture);
    return static_cast<bool>(
            android_physical_model_set(PHYSICAL_PARAMETER_POSTURE, &f_posture,
                                       1, PHYSICAL_INTERPOLATION_SMOOTH) >= 0);
}

bool android_foldable_posture_name(int posture, char* name) {
    if (!name) {
        return false;
    }
    switch (posture) {
        case 0:
            strcpy(name, "UNKNOWN");
            return true;
        case 1:
            strcpy(name, "CLOSED");
            return true;
        case 2:
            strcpy(name, "HALF_OPENED");
            return true;
        case 3:
            strcpy(name, "OPENED");
            return true;
        case 4:
            strcpy(name, "FLIPPED");
            return true;
        case 5:
            strcpy(name, "TENT");
            return true;
        default:
            return false;
    }
}

bool android_foldable_get_folded_area(int* x, int* y, int* w, int* h) {
    return physicalModel_getFoldedArea(android_physical_model_instance(), x, y,
                                       w, h);
}

bool android_foldable_rollable_configured() {
    return (getConsoleAgents()->settings->hw()->hw_sensor_roll &&
            getConsoleAgents()->settings->hw()->hw_sensor_roll_count > 0);
}

bool android_is_automotive() {
    if (getConsoleAgents()->settings->android_qemu_mode()) {
        AvdFlavor flavor =
                avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo());
        return flavor == AVD_ANDROID_AUTO;
    }
    return false;
}

bool android_hw_sensors_is_loading_snapshot() {
    return physicalModel_isLoadingSnapshot(android_physical_model_instance());
}

bool android_heart_rate_sensor_configured() {
    return getConsoleAgents()->settings->hw()->hw_sensors_heart_rate;
}

void* android_get_posture_listener() {
    return physicalModel_getPostureListener(android_physical_model_instance());
}

void android_hw_sensors_register_callback(HwSensorChangedCallback callback,
                                          void* opaque) {
    auto oldSize = gSensorListeners.size();
    gSensorListeners[opaque] =
            std::make_unique<HwSensorChangedCallbackListener>(callback, opaque);
    assert(oldSize == gSensorListeners.size() - 1);
}

void android_hw_sensors_unregister_callback(void* opaque) {
    auto oldSize = gSensorListeners.size();
    gSensorListeners.erase(opaque);
    assert(oldSize == gSensorListeners.size() + 1);
}

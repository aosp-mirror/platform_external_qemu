/*
 * Goldfish 'sensors' device model
 *
 * This provides a android pipe based device to report sensor data to
 * the android goldfish emulation platform. It is a very much cut-down
 * version of the original "hw-sensors.c" from the Android Emulator.
 * The main difference is it only supports the accelerometer and
 * directly hooks into the AndroidPipe mechanism without the qemud
 * machinery.
 *
 * Copyright (c) 2014 Linaro Limited
 * Copyright (c) 2009 The Android Open Source Project
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "android/emulation/android_pipe_host.h"
#include "hw/input/goldfish_sensors.h"
#include "hw/misc/android_qemud.h"
#include "qemu/error-report.h"
#include "qemu/timer.h"
#include "sysemu/char.h"
#include "trace.h"

#include <math.h>

/* #define DEBUG_SENSORS */

#ifdef DEBUG_SENSORS
#define DPRINTF(fmt, ...) \
    do { fprintf(stderr, "%s: " fmt , __func__, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while (0)
#endif

enum {
    ANDROID_SENSOR_ACCELERATION
} SensorID_t;

/* The sensor configuration is global to whole driver */
typedef struct SensorsConfig {
    /* Timer tick stuff */
    int         delay_ms;

    /* Sensor data */
    int         enabled_mask;
    struct {
        struct {
            float   x, y, z;
        } acceleration;
    } sensors;
} SensorsConfig;

struct SensorsConfig sensor_config = {
    800,
    0,
    {
        .acceleration = { 0.0, 0.0, 0.0 }
    }
};

/* There will be one pipe connection per open connection with the
 * guest. Generally there will be a control path used to interrogate
 * the list of sensors and a data path which will trigger the periodic
 * updating of the the data
 */
typedef struct SensorsPipe {
    /* Android Pipe structs */
    void      *hwpipe;
    /* Sensor Ticks */
    QEMUTimer *periodic_tick;
    /* Arrays containing framed strings, outstanding is used for
     * staging the frames as they come in/go out.
     */
    GPtrArray *send_data;
    GString   *send_outstanding;
    GPtrArray *recv_data;
    GString   *recv_outstanding;
} SensorsPipe;

#define RADIANS_PER_DEGREE (M_PI / 180.0)

void goldfish_sensors_set_rotation(int rotation)
{
    /* The Android framework computes the orientation by looking at
     * the accelerometer sensor (*not* the orientation sensor !)
     *
     * That's because the gravity is a constant 9.81 vector that
     * can be determined quite easily.
     *
     * Also, for some reason, the framework code considers that the phone should
     * be inclined by 30 degrees along the phone's X axis to be considered
     * in its ideal "vertical" position.
     *
     * If the phone is completely vertical, rotating it will not do anything !
     */
    const double  g      = 9.81;
    const double  angle  = 20.0;
    const double  cos_angle = cos(angle * RADIANS_PER_DEGREE);
    const double  sin_angle = sin(angle * RADIANS_PER_DEGREE);

    switch (rotation) {
    case 0:
        /* Natural layout */
        sensor_config.sensors.acceleration.x = 0.0;
        sensor_config.sensors.acceleration.y = g*cos_angle;
        sensor_config.sensors.acceleration.z = g*sin_angle;
        break;
    case 1:
        /* Rotate 90 degrees clockwise */
        sensor_config.sensors.acceleration.x = g*cos_angle;
        sensor_config.sensors.acceleration.y = 0.0;
        sensor_config.sensors.acceleration.z = g*sin_angle;
        break;
    case 2:
        /* Rotate 180 degrees clockwise */
        sensor_config.sensors.acceleration.x = 0.0;
        sensor_config.sensors.acceleration.y = -1.0 * g*cos_angle;
        sensor_config.sensors.acceleration.z = g*sin_angle;
        break;
    case 3:
        /* Rotate 270 degrees clockwise */
        sensor_config.sensors.acceleration.x = -1.0 * g*cos_angle;
        sensor_config.sensors.acceleration.y = 0.0;
        sensor_config.sensors.acceleration.z = g*sin_angle;
        break;
    default:
        g_assert_not_reached();
        break;
    }
}

/* I'll just append to a GString holding our data here, which we
 * truncate as we page back out....
 */
static void sensor_send_data(SensorsPipe *sp, const uint8_t *buf, int len)
{
    gchar *msg = g_strndup((gchar *) buf, len);
    g_ptr_array_add(sp->send_data, msg);
    android_pipe_host_signal_wake(sp->hwpipe, PIPE_WAKE_READ);
}


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

/* this function is called periodically to send sensor reports
 * to the HAL module, and re-arm the timer if necessary
 */
static void
goldfish_sensor_tick(void *opaque)
{
    SensorsPipe *sp = opaque;
    int64_t          delay = sensor_config.delay_ms;
    int64_t          now_ms;
    uint32_t         mask  = sensor_config.enabled_mask;
    char             buffer[128];

    if (sensor_config.enabled_mask & (1<<ANDROID_SENSOR_ACCELERATION)) {
        snprintf(buffer, sizeof buffer, "acceleration:%g:%g:%g",
                 sensor_config.sensors.acceleration.x,
                 sensor_config.sensors.acceleration.y,
                 sensor_config.sensors.acceleration.z);
        sensor_send_data(sp, (uint8_t *)buffer, strlen(buffer));
    }

    now_ms = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);

    snprintf(buffer, sizeof buffer, "sync:%" PRId64, now_ms * 1000);
    sensor_send_data(sp, (uint8_t *)buffer, strlen(buffer));

    /* rearm timer, use a minimum delay of 20 ms, just to
     * be safe.
     */
    if (mask == 0) {
        return;
    }

    if (delay < 20) {
        delay = 20;
    }

    timer_mod(sp->periodic_tick, now_ms + delay);
}

/* Incoming command from the guest */
static ssize_t goldfish_sensors_have_data(SensorsPipe *sp, const gchar *buf)
{
    int len = strlen(buf);
    /* "list-sensors" is used to get an integer bit map of
     * available emulated sensors. We compute the mask from the
     * current hardware configuration.
     */
    if (len == 12 && g_str_has_prefix(buf, "list-sensors")) {
        sensor_send_data(sp, (const uint8_t *)"1", 1);
        return len;
    }

    /* "wake" is a special message that must be sent back through
     * the channel. It is used to exit a blocking read.
     */
    if (len == 4 && g_str_has_prefix(buf, "wake")) {
        sensor_send_data(sp, (const uint8_t *)"wake", 4);
        return len;
    }

    /* "set-delay:<delay>" is used to set the delay in milliseconds
     * between sensor events
     */
    if (len > 10 && g_str_has_prefix(buf, "set-delay:")) {
        sensor_config.delay_ms = atoi((const char *)buf+10);
        if (sensor_config.enabled_mask != 0) {
            goldfish_sensor_tick(sp);
        }
        return len;
    }

    /* "set:<name>:<state>" is used to enable/disable a given
     * sensor. <state> must be 0 or 1
     */
    if (len > 4 && g_str_has_prefix(buf, "set:")) {
        int mask = 0;
        gchar *state = g_strrstr_len(buf, len, ":");
        if (g_str_has_prefix(buf, "set:acceleration")) {
            mask = (1<<ANDROID_SENSOR_ACCELERATION);
        }
        if (state && mask) {
            switch (state[1]) {
            case '0':
                sensor_config.enabled_mask &= ~mask;
                break;
            case '1':
                sensor_config.enabled_mask |= mask;
                goldfish_sensor_tick(sp);
                break;
            default:
                DPRINTF("bad set: command (%s)", buf);
                break;
            }
        }
        return len;
    }

    return len;
}

static void *sensors_pipe_init(void *hwpipe, void *opaque, const char *args)
{
    SensorsPipe *pipe;

    DPRINTF("hwpipe=%p\n", hwpipe);
    pipe = g_malloc0(sizeof(SensorsPipe));
    pipe->hwpipe = hwpipe;
    pipe->periodic_tick = timer_new_ms(QEMU_CLOCK_VIRTUAL,
                                       goldfish_sensor_tick, pipe);
    pipe->send_data = g_ptr_array_new_full(8, g_free);
    pipe->recv_data = g_ptr_array_new_full(8, g_free);
    pipe->send_outstanding = g_string_sized_new(128);
    pipe->recv_outstanding = g_string_sized_new(128);
    return pipe;
}

static void sensors_pipe_close(void *opaque)
{
    SensorsPipe *pipe = opaque;
    DPRINTF("pipe %p, hwpipe %p\n", pipe, pipe->hwpipe);
    timer_del(pipe->periodic_tick);
    timer_free(pipe->periodic_tick);
    pipe->periodic_tick = NULL;
    g_ptr_array_free(pipe->send_data, TRUE);
    g_ptr_array_free(pipe->recv_data, TRUE);
    g_string_free(pipe->send_outstanding, TRUE);
    g_string_free(pipe->recv_outstanding, TRUE);
    g_free(pipe);
}

static void sensors_pipe_wake(void *opaque, int flags)
{
    SensorsPipe *pipe = opaque;

    /* we have data for the guest to read */
    if (flags & PIPE_WAKE_READ
        && (pipe->send_outstanding->len > 0 || pipe->send_data->len > 0)) {
        DPRINTF("0x%x:PIPE_WAKE_READ we have %d bytes, %d msgs\n", flags,
                (int) pipe->send_outstanding->len,
                pipe->send_data->len);
        android_pipe_host_signal_wake(pipe->hwpipe, PIPE_WAKE_READ);
    }

    /* we can always be written to... */
    if (flags & PIPE_WAKE_WRITE) {
        DPRINTF("0x%x:PIPE_WAKE_WRITE\n", flags);
        android_pipe_host_signal_wake(pipe->hwpipe, PIPE_WAKE_WRITE);
    }
}

static int sensors_pipe_recv(void *opaque, AndroidPipeBuffer *buffers,
                             int cnt)
{
    SensorsPipe *pipe = opaque;

    DPRINTF("%d outstanding msgs\n", pipe->send_data->len);
    if (pipe->send_outstanding->len > 0 || pipe->send_data->len > 0) {
        return qemud_pipe_write_buffers(pipe->send_data, pipe->send_outstanding,
                                        buffers, cnt);
    } else {
        return PIPE_ERROR_AGAIN;
    }
}

/*
 * We use the qemud helper functions to de-serialise the qemud framed
 * messages into a series of strings. We then just process each entry
 * in the array and free the strings when done.
 */
static int sensors_pipe_send(void *opaque, const AndroidPipeBuffer* buffers,
                             int cnt)
{
    SensorsPipe *pipe = opaque;
    int consumed = qemud_pipe_read_buffers(buffers, cnt,
                                           pipe->recv_data,
                                           pipe->recv_outstanding);

    DPRINTF("pipe %p, messages: %d\n", pipe, pipe->recv_data->len);

    while (pipe->recv_data->len > 0) {
        gchar *msg = g_ptr_array_index(pipe->recv_data, 0);
        goldfish_sensors_have_data(pipe, msg);
        g_ptr_array_remove(pipe->recv_data, msg);
    }

    return consumed;
}

static unsigned sensors_pipe_poll(void *opaque)
{
    SensorsPipe *pipe = opaque;
    unsigned flags = 0;

    if (pipe->send_outstanding->len > 0 || pipe->send_data->len > 0) {
        flags |= PIPE_POLL_IN;
    }
    flags |= PIPE_POLL_OUT;
    DPRINTF("pipe %p, flags 0x%x", pipe, flags);
    return flags;
}

static const AndroidPipeFuncs sensors_pipe_funcs = {
    sensors_pipe_init,
    sensors_pipe_close,
    sensors_pipe_send,
    sensors_pipe_recv,
    sensors_pipe_poll,
    sensors_pipe_wake,
};

void android_sensors_init(void)
{
    goldfish_sensors_set_rotation(0);
    android_pipe_add_type("qemud:sensors", NULL, &sensors_pipe_funcs);
}

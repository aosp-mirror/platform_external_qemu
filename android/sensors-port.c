/*
 * Copyright (C) 2011 The Android Open Source Project
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

#include "android/sensors-port.h"
#include "android/hw-sensors.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(sensors_port,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(sensors_port)

/* Maximum number of sensors supported. */
#define ASP_MAX_SENSOR          12

/* Maximum length of a sensor message. */
#define ASP_MAX_SENSOR_MSG      1024

/* Maximum length of a sensor event. */
#define ASP_MAX_SENSOR_EVENT    256

/* Query timeout in milliseconds. */
#define ASP_QUERY_TIMEOUT       3000

/* Sensors port descriptor. */
struct AndroidSensorsPort {
    /* Caller identifier. */
    void*           opaque;
    /* Connected android device. */
    AndroidDevice*  device;
    /* String containing list of all available sensors. */
    char            sensors[ASP_MAX_SENSOR * 64];
    /* Array of available sensor names. Note that each string in this array
     * points inside the 'sensors' buffer. */
    const char*     sensor_list[ASP_MAX_SENSOR];
    /* Number of available sensors. */
    int             sensors_num;
    /* Connection status: 1 connected, 0 - disconnected. */
    int             is_connected;
    /* Buffer where to receive sensor messages. */
    char            sensor_msg[ASP_MAX_SENSOR_MSG];
    /* Buffer where to receive sensor events. */
    char            events[ASP_MAX_SENSOR_EVENT];
};

/* Destroys and frees the descriptor. */
static void
_sensors_port_free(AndroidSensorsPort* asp)
{
    if (asp != NULL) {
        if (asp->device != NULL) {
            android_device_destroy(asp->device);
        }
        AFREE(asp);
    }
}

/********************************************************************************
 *                          Sensors port callbacks
 *******************************************************************************/

/* A callback that invoked on sensor events.
 * Param:
 *  opaque - AndroidSensorsPort instance.
 *  ad - Android device used by this sensors port.
 *  msg, msgsize - Sensor event message
 *  failure - Message receiving status.
 */
static void
_on_sensor_received(void* opaque, AndroidDevice* ad, char* msg, int msgsize)
{
    float fvalues[3] = {0, 0, 0};
    char sensor[ASP_MAX_SENSOR_MSG];
    char* value;
    int id;
    AndroidSensorsPort* asp = (AndroidSensorsPort*)opaque;

    if (errno) {
        D("Sensors notification has failed on sensors port: %s", strerror(errno));
        return;
    }

    /* Parse notification, separating sensor name from parameters. */
    memcpy(sensor, msg, msgsize);
    value = strchr(sensor, ':');
    if (value == NULL) {
        W("Bad format for sensor notification: %s", msg);
        return;
    }
    sensor[value-sensor] = '\0';
    value++;

    id = android_sensors_get_id_from_name(sensor);
    if (id >= 0) {
        /* Parse the value part to get the sensor values(a, b, c) */
        int i;
        char* pnext;
        char* pend = value + strlen(value);
        for (i = 0; i < 3; i++, value = pnext + 1) {
            pnext=strchr( value, ':' );
            if (pnext) {
                *pnext = 0;
            } else {
                pnext = pend;
            }

            if (pnext > value) {
                if (1 != sscanf( value,"%g", &fvalues[i] )) {
                    W("Bad parameters in sensor notification %s", msg);
                    return;
                }
            }
        }
        android_sensors_set(id, fvalues[0], fvalues[1], fvalues[2]);
    } else {
        W("Unknown sensor name '%s' in '%s'", sensor, msg);
    }

    /* Listen to the next event. */
    android_device_listen(ad, asp->events, sizeof(asp->events), _on_sensor_received);
}

/* A callback that is invoked when android device is connected (i.e. both, command
 * and event channels have been stablished.
 * Param:
 *  opaque - AndroidSensorsPort instance.
 *  ad - Android device used by this sensors port.
 *  failure - Connections status.
 */
static void
_on_device_connected(void* opaque, AndroidDevice* ad, int failure)
{
    if (!failure) {
        AndroidSensorsPort* asp = (AndroidSensorsPort*)opaque;
        asp->is_connected = 1;
        D("Sensor emulation has started");
        /* Initialize sensors on device. */
        sensors_port_init_sensors(asp);
    }
}

/* Invoked when an I/O failure occurs on a socket.
 * Note that this callback will not be invoked on connection failures.
 * Param:
 *  opaque - AndroidSensorsPort instance.
 *  ad - Android device instance
 *  ads - Connection socket where failure has occured.
 *  failure - Contains 'errno' indicating the reason for failure.
 */
static void
_on_io_failure(void* opaque, AndroidDevice* ad, int failure)
{
    AndroidSensorsPort* asp = (AndroidSensorsPort*)opaque;
    E("Sensors port got disconnected: %s", strerror(failure));
    asp->is_connected = false;
    android_device_disconnect(ad);
    android_device_connect_async(ad, _on_device_connected);
}

/********************************************************************************
 *                          Sensors port API
 *******************************************************************************/

#include "android/sdk-controller-socket.h"

static AsyncIOAction
_on_sdkctl_connection(void* client_opaque, SDKCtlSocket* sdkctl, AsyncIOState status)
{
    if (status == ASIO_STATE_FAILED) {
        sdkctl_socket_reconnect(sdkctl, 1970, 20);
    }
    return ASIO_ACTION_DONE;
}

void on_sdkctl_handshake(void* client_opaque,
                         SDKCtlSocket* sdkctl,
                         void* handshake,
                         uint32_t handshake_size,
                         AsyncIOState status)
{
    if (status == ASIO_STATE_SUCCEEDED) {
        printf("---------- Handshake %d bytes received.\n", handshake_size);
    } else {
        printf("!!!!!!!!!! Handshake failed with status %d: %d -> %s\n",
               status, errno, strerror(errno));
        sdkctl_socket_reconnect(sdkctl, 1970, 20);
    }
}

void on_sdkctl_message(void* client_opaque,
                       SDKCtlSocket* sdkctl,
                       SDKCtlPacket* message,
                       int msg_type,
                       void* msg_data,
                       int msg_size)
{
    printf("########################################################\n");
}

AndroidSensorsPort*
sensors_port_create(void* opaque)
{
    AndroidSensorsPort* asp;
    char* wrk;
    int res;

    SDKCtlSocket* sdkctl = sdkctl_socket_new(20, "test", _on_sdkctl_connection,
                                             on_sdkctl_handshake, on_sdkctl_message,
                                             NULL);
//    sdkctl_init_recycler(sdkctl, 64, 8);
    sdkctl_socket_connect(sdkctl, 1970, 20);

    ANEW0(asp);
    asp->opaque = opaque;
    asp->is_connected = 0;

    asp->device = android_device_init(asp, AD_SENSOR_PORT, _on_io_failure);
    if (asp->device == NULL) {
        _sensors_port_free(asp);
        return NULL;
    }

    res = android_device_connect_sync(asp->device, ASP_QUERY_TIMEOUT);
    if (res != 0) {
        sensors_port_destroy(asp);
        return NULL;
    }

    res = android_device_query(asp->device, "list",
                               asp->sensors, sizeof(asp->sensors),
                               ASP_QUERY_TIMEOUT);
    if (res != 0) {
        sensors_port_destroy(asp);
        return NULL;
    }

    /* Parse sensor list. */
    asp->sensors_num = 0;
    wrk = asp->sensors;

    while (wrk != NULL && *wrk != '\0' && *wrk != '\n') {
        asp->sensor_list[asp->sensors_num] = wrk;
        asp->sensors_num++;
        wrk = strchr(wrk, '\n');
        if (wrk != NULL) {
            *wrk = '\0'; wrk++;
        }
    }

    android_device_listen(asp->device, asp->events, sizeof(asp->events),
                          _on_sensor_received);
    return asp;
}

int
sensors_port_init_sensors(AndroidSensorsPort* asp)
{
    int res, id;

    /* Disable all sensors for now. Reenable only those that are emulated. */
    res = sensors_port_disable_sensor(asp, "all");
    if (res) {
        return res;
    }

    /* Start listening on sensor events. */
    res = android_device_listen(asp->device, asp->events, sizeof(asp->events),
                                _on_sensor_received);
    if (res) {
        return res;
    }

    /* Walk throuh the list of enabled sensors enabling them on the device. */
    for (id = 0; id < MAX_SENSORS; id++) {
        if (android_sensors_get_sensor_status(id) == 1) {
            res = sensors_port_enable_sensor(asp, android_sensors_get_name_from_id(id));
            if (res == 0) {
                D("Sensor '%s' is enabled on the device.",
                  android_sensors_get_name_from_id(id));
            }
        }
    }

    /* Start sensor events. */
    return sensors_port_start(asp);
}

void
sensors_port_destroy(AndroidSensorsPort* asp)
{
    _sensors_port_free(asp);
}

int
sensors_port_is_connected(AndroidSensorsPort* asp)
{
    return asp->is_connected;
}

int
sensors_port_enable_sensor(AndroidSensorsPort* asp, const char* name)
{
    char query[1024];
    char qresp[1024];
    snprintf(query, sizeof(query), "enable:%s", name);
    const int res =
        android_device_query(asp->device, query, qresp, sizeof(qresp),
                             ASP_QUERY_TIMEOUT);
    if (res) {
        if (errno) {
            D("Query '%s' failed on I/O: %s", query, strerror(errno));
        } else {
            D("Query '%s' failed on device: %s", query, qresp);
        }
    }
    return res;
}

int
sensors_port_disable_sensor(AndroidSensorsPort* asp, const char* name)
{
    char query[1024];
    char qresp[1024];
    snprintf(query, sizeof(query), "disable:%s", name);
    const int res =
        android_device_query(asp->device, query, qresp, sizeof(qresp),
                             ASP_QUERY_TIMEOUT);
    if (res) {
        if (errno) {
            D("Query '%s' failed on I/O: %s", query, strerror(errno));
        } else {
            D("Query '%s' failed on device: %s", query, qresp);
        }
    }
    return res;
}

int
sensors_port_start(AndroidSensorsPort* asp)
{
    char qresp[ASP_MAX_SENSOR_MSG];
    const int res =
        android_device_query(asp->device, "start", qresp, sizeof(qresp),
                             ASP_QUERY_TIMEOUT);
    if (res) {
        if (errno) {
            D("Query 'start' failed on I/O: %s", strerror(errno));
        } else {
            D("Query 'start' failed on device: %s", qresp);
        }
    }
    return res;
}

int
sensors_port_stop(AndroidSensorsPort* asp)
{
    char qresp[ASP_MAX_SENSOR_MSG];
    const int res =
        android_device_query(asp->device, "stop", qresp, sizeof(qresp),
                             ASP_QUERY_TIMEOUT);
    if (res) {
        if (errno) {
            D("Query 'stop' failed on I/O: %s", strerror(errno));
        } else {
            D("Query 'stop' failed on device: %s", qresp);
        }
    }

    return res;
}

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

#include "qemu-common.h"
#include "utils/panic.h"
#include "android/hw-events.h"
#include "android/charmap.h"
#include "android/multitouch-screen.h"
#include "android/multitouch-port.h"
#include "android/globals.h"  /* for android_hw */
#include "android/utils/misc.h"
#include "android/utils/jpeg-compress.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(mtport,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(mtport)

/* Query timeout in milliseconds. */
#define MTSP_QUERY_TIMEOUT       3000
#define MTSP_MAX_MSG             2048
#define MTSP_MAX_EVENT           2048

/* Multi-touch port descriptor. */
struct AndroidMTSPort {
    /* Caller identifier. */
    void*           opaque;
    /* Connected android device. */
    AndroidDevice*  device;
    /* Initialized JPEG compressor instance. */
    AJPEGDesc*      jpeg_compressor;
    /* Connection status: 1 connected, 0 - disconnected. */
    int             is_connected;
    /* Buffer where to receive multitouch messages. */
    char            mts_msg[MTSP_MAX_MSG];
    /* Buffer where to receive multitouch events. */
    char            events[MTSP_MAX_EVENT];
};

/* Destroys and frees the descriptor. */
static void
_mts_port_free(AndroidMTSPort* mtsp)
{
    if (mtsp != NULL) {
        if (mtsp->jpeg_compressor != NULL) {
            jpeg_compressor_destroy(mtsp->jpeg_compressor);
        }
        if (mtsp->device != NULL) {
            android_device_destroy(mtsp->device);
        }
        AFREE(mtsp);
    }
}

/********************************************************************************
 *                          Multi-touch action handlers
 *******************************************************************************/

/*
 * Although there are a lot of similarities in the way the handlers below are
 * implemented, for the sake of tracing / debugging it's better to have a
 * separate handler for each distinctive action.
 */

/* First pointer down event handler. */
static void
_on_action_down(int tracking_id, int x, int y, int pressure)
{
    multitouch_update_pointer(MTES_DEVICE, tracking_id, x, y, pressure);
}

/* Last pointer up event handler. */
static void
_on_action_up(int tracking_id)
{
    multitouch_update_pointer(MTES_DEVICE, tracking_id, 0, 0, 0);
}

/* Pointer down event handler. */
static void
_on_action_pointer_down(int tracking_id, int x, int y, int pressure)
{
    multitouch_update_pointer(MTES_DEVICE, tracking_id, x, y, pressure);
}

/* Pointer up event handler. */
static void
_on_action_pointer_up(int tracking_id)
{
    multitouch_update_pointer(MTES_DEVICE, tracking_id, 0, 0, 0);
}

/* Pointer move event handler. */
static void
_on_action_move(int tracking_id, int x, int y, int pressure)
{
    multitouch_update_pointer(MTES_DEVICE, tracking_id, x, y, pressure);
}

/********************************************************************************
 *                          Multi-touch event handlers
 *******************************************************************************/

/* Handles "pointer move" event. */
static void
_on_move(const char* param)
{
    const char* pid = param;
    D(">>> MOVE: %s", param);
    while (pid && *pid) {
        int pid_val, x, y, pressure = 0;
        if (!get_token_value_int(pid, "pid", &pid_val) &&
            !get_token_value_int(pid, "x", &x) &&
            !get_token_value_int(pid, "y", &y)) {
            get_token_value_int(pid, "pressure", &pressure);
            _on_action_move(pid_val, x, y, pressure);
            pid = strstr(pid + 1, "pid");
        } else {
            break;
        }
    }
}

/* Handles "first pointer down" event. */
static void
_on_down(const char* param)
{
    int pid_val, x, y, pressure = 0;
    D(">>> 1-ST DOWN: %s", param);
    if (!get_token_value_int(param, "pid", &pid_val) &&
        !get_token_value_int(param, "x", &x) &&
        !get_token_value_int(param, "y", &y)) {
        get_token_value_int(param, "pressure", &pressure);
        _on_action_down(pid_val, x, y, pressure);
    } else {
        W("Invalid parameters '%s' for MTS 'down' event", param);
    }
}

/* Handles "last pointer up" event. */
static void
_on_up(const char* param)
{
    int pid_val;
    D(">>> LAST UP: %s", param);
    if (!get_token_value_int(param, "pid", &pid_val)) {
        _on_action_up(pid_val);
    } else {
        W("Invalid parameters '%s' for MTS 'up' event", param);
    }
}

/* Handles "next pointer down" event. */
static void
_on_pdown(const char* param)
{
    int pid_val, x, y, pressure = 0;
    D(">>> DOWN: %s", param);
    if (!get_token_value_int(param, "pid", &pid_val) &&
        !get_token_value_int(param, "x", &x) &&
        !get_token_value_int(param, "y", &y)) {
        get_token_value_int(param, "pressure", &pressure);
        _on_action_pointer_down(pid_val, x, y, pressure);
    } else {
        W("Invalid parameters '%s' for MTS 'pointer down' event", param);
    }
}

/* Handles "next pointer up" event. */
static void
_on_pup(const char* param)
{
    int pid_val;
    D(">>> UP: %s", param);
    if (!get_token_value_int(param, "pid", &pid_val)) {
        _on_action_pointer_up(pid_val);
    } else {
        W("Invalid parameters '%s' for MTS 'up' event", param);
    }
}

/********************************************************************************
 *                      Device communication callbacks
 *******************************************************************************/

/* Main event handler.
 * This routine is invoked when an event message has been received from the
 * device.
 */
static void
_on_event_received(void* opaque, AndroidDevice* ad, char* msg, int msgsize)
{
    char* action;
    int res;
    AndroidMTSPort* mtsp = (AndroidMTSPort*)opaque;

    if (errno) {
        D("Multi-touch notification has failed: %s", strerror(errno));
        return;
    }

    /* Dispatch the event to an appropriate handler. */
    res = get_token_value_alloc(msg, "action", &action);
    if (!res) {
        const char* param = strchr(msg, ' ');
        if (param) {
            param++;
        }
        if (!strcmp(action, "move")) {
            _on_move(param);
        } else if (!strcmp(action, "down")) {
            _on_down(param);
        } else if (!strcmp(action, "up")) {
            _on_up(param);
        } else if (!strcmp(action, "pdown")) {
            _on_pdown(param);
        } else if (!strcmp(action, "pup")) {
            _on_pup(param);
        } else {
            D("Unknown multi-touch event action '%s'", action);
        }
        free(action);
    }

    /* Listen to the next event. */
    android_device_listen(ad, mtsp->events, sizeof(mtsp->events),
                          _on_event_received);
}

/* A callback that is invoked when android device is connected (i.e. both,
 * command and event channels have been established).
 * Param:
 *  opaque - AndroidMTSPort instance.
 *  ad - Android device used by this port.
 *  failure - Connections status.
 */
static void
_on_device_connected(void* opaque, AndroidDevice* ad, int failure)
{
    if (!failure) {
        AndroidMTSPort* mtsp = (AndroidMTSPort*)opaque;
        mtsp->is_connected = 1;
        D("Multi-touch emulation has started");
        android_device_listen(mtsp->device, mtsp->events, sizeof(mtsp->events),
                              _on_event_received);
        mts_port_start(mtsp);
    }
}

/* Invoked when an I/O failure occurs on a socket.
 * Note that this callback will not be invoked on connection failures.
 * Param:
 *  opaque - AndroidMTSPort instance.
 *  ad - Android device instance
 *  ads - Connection socket where failure has occured.
 *  failure - Contains 'errno' indicating the reason for failure.
 */
static void
_on_io_failure(void* opaque, AndroidDevice* ad, int failure)
{
    AndroidMTSPort* mtsp = (AndroidMTSPort*)opaque;
    E("Multi-touch port got disconnected: %s", strerror(failure));
    mtsp->is_connected = 0;
    android_device_disconnect(ad);

    /* Try to reconnect again. */
    android_device_connect_async(ad, _on_device_connected);
}

/********************************************************************************
 *                          MTS port API
 *******************************************************************************/

AndroidMTSPort*
mts_port_create(void* opaque)
{
    AndroidMTSPort* mtsp;
    int res;

    ANEW0(mtsp);
    mtsp->opaque = opaque;
    mtsp->is_connected = 0;

    /* Initialize default MTS descriptor. */
    multitouch_init(mtsp);

    /* Create JPEG compressor. Put "$BLOB:%09d\0" + MTFrameHeader header in front
     * of the compressed data. this way we will have entire query ready to be
     * transmitted to the device. */
    mtsp->jpeg_compressor = jpeg_compressor_create(16 + sizeof(MTFrameHeader), 4096);

    mtsp->device = android_device_init(mtsp, AD_MULTITOUCH_PORT, _on_io_failure);
    if (mtsp->device == NULL) {
        _mts_port_free(mtsp);
        return NULL;
    }

    res = android_device_connect_async(mtsp->device, _on_device_connected);
    if (res != 0) {
        mts_port_destroy(mtsp);
        return NULL;
    }

    return mtsp;
}

void
mts_port_destroy(AndroidMTSPort* mtsp)
{
    _mts_port_free(mtsp);
}

int
mts_port_is_connected(AndroidMTSPort* mtsp)
{
    return mtsp->is_connected;
}

int
mts_port_start(AndroidMTSPort* mtsp)
{
    char qresp[MTSP_MAX_MSG];
    char query[256];
    AndroidHwConfig* config = android_hw;

    /* Query the device to start capturing multi-touch events, also providing
     * the device with width / height of the emulator's screen. This is required
     * so device can properly adjust multi-touch event coordinates, and display
     * emulator's framebuffer. */
    snprintf(query, sizeof(query), "start:%dx%d",
             config->hw_lcd_width, config->hw_lcd_height);
    int res = android_device_query(mtsp->device, query, qresp, sizeof(qresp),
                                   MTSP_QUERY_TIMEOUT);
    if (!res) {
        /* By protocol device should reply with its view dimensions. */
        if (*qresp) {
            int width, height;
            if (sscanf(qresp, "%dx%d", &width, &height) == 2) {
                multitouch_set_device_screen_size(width, height);
                D("Multi-touch emulation has started. Device dims: %dx%d",
                  width, height);
            } else {
                E("Unexpected reply to MTS 'start' query: %s", qresp);
                android_device_query(mtsp->device, "stop", qresp, sizeof(qresp),
                                     MTSP_QUERY_TIMEOUT);
                res = -1;
            }
        } else {
            E("MTS protocol error: no reply to query 'start'");
            android_device_query(mtsp->device, "stop", qresp, sizeof(qresp),
                                 MTSP_QUERY_TIMEOUT);
            res = -1;
        }
    } else {
        if (errno) {
            D("Query 'start' failed on I/O: %s", strerror(errno));
        } else {
            D("Query 'start' failed on device: %s", qresp);
        }
    }
    return res;
}

int
mts_port_stop(AndroidMTSPort* mtsp)
{
    char qresp[MTSP_MAX_MSG];
    const int res =
        android_device_query(mtsp->device, "stop", qresp, sizeof(qresp),
                             MTSP_QUERY_TIMEOUT);
    if (res) {
        if (errno) {
            D("Query 'stop' failed on I/O: %s", strerror(errno));
        } else {
            D("Query 'stop' failed on device: %s", qresp);
        }
    }

    return res;
}

/********************************************************************************
 *                       Handling framebuffer updates
 *******************************************************************************/

/* Compresses a framebuffer region into JPEG image.
 * Param:
 *  mtsp - Multi-touch port descriptor with initialized JPEG compressor.
 *  fmt Descriptor for framebuffer region to compress.
 *  fb Beginning of the framebuffer.
 *  jpeg_quality JPEG compression quality. A number from 1 to 100. Note that
 *      value 10 provides pretty decent image for the purpose of multi-touch
 *      emulation.
 */
static void
_fb_compress(const AndroidMTSPort* mtsp,
             const MTFrameHeader* fmt,
             const uint8_t* fb,
             int jpeg_quality,
             int ydir)
{
    jpeg_compressor_compress_fb(mtsp->jpeg_compressor, fmt->x, fmt->y, fmt->w,
                                fmt->h, fmt->disp_height, fmt->bpp, fmt->bpl,
                                fb, jpeg_quality, ydir);
}

int
mts_port_send_frame(AndroidMTSPort* mtsp,
                    MTFrameHeader* fmt,
                    const uint8_t* fb,
                    async_send_cb cb,
                    void* cb_opaque,
                    int ydir)
{
    char* query;
    int blob_size, off;

    /* Make sure that port is connected. */
    if (!mts_port_is_connected(mtsp)) {
        return -1;
    }

    /* Compress framebuffer region. 10% quality seems to be sufficient. */
    fmt->format = MTFB_JPEG;
    _fb_compress(mtsp, fmt, fb, 10, ydir);

    /* Total size of the blob: header + JPEG image. */
    blob_size = sizeof(MTFrameHeader) +
                jpeg_compressor_get_jpeg_size(mtsp->jpeg_compressor);

    /* Query starts at the beginning of the buffer allocated by the compressor's
     * destination manager. */
    query = (char*)jpeg_compressor_get_buffer(mtsp->jpeg_compressor);

    /* Build the $BLOB query to transfer to the device. */
    snprintf(query, jpeg_compressor_get_header_size(mtsp->jpeg_compressor),
             "$BLOB:%09d", blob_size);
    off = strlen(query) + 1;

    /* Copy framebuffer update header to the query. */
    memcpy(query + off, fmt, sizeof(MTFrameHeader));

    /* Zeroing the rectangle in the update header we indicate that it contains
     * no updates. */
    fmt->x = fmt->y = fmt->w = fmt->h = 0;

    /* Initiate asynchronous transfer of the updated framebuffer rectangle. */
    if (android_device_send_async(mtsp->device, query, off + blob_size, 0, cb, cb_opaque)) {
        D("Unable to send query '%s': %s", query, strerror(errno));
        return -1;
    }

    return 0;
}

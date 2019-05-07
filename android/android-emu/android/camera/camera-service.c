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

/*
 * Contains emulated camera service implementation.
 */

#include "android/camera/camera-service.h"

#include "android/boot-properties.h"
#include "android/camera/camera-capture.h"
#include "android/camera/camera-format-converters.h"
#include "android/camera/camera-metrics.h"
#include "android/camera/camera-videoplayback.h"
#include "android/camera/camera-virtualscene.h"
#include "android/emulation/android_qemud.h"
#include "android/featurecontrol/feature_control.h"
#include "android/globals.h" /* for android_hw */
#include "android/hw-sensors.h"
#include "android/utils/debug.h"
#include "android/utils/looper.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"

#include <stdio.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(camera)

/* the T(...) macro is used to dump traffic */
#define  T_ACTIVE   0

#if T_ACTIVE
#define  T(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#else
#define  T(...)    ((void)0)
#endif

/* Defines name of the camera service. */
#define SERVICE_NAME    "camera"

/* Maximum number of supported emulated cameras. */
#define MAX_CAMERA      8

/* the query from guest should not be too long, so 8092 is more than enough */
#define MAX_QUERY_MESSAGE_SIZE 8092

/* Camera sevice descriptor. */
typedef struct CameraServiceDesc CameraServiceDesc;
struct CameraServiceDesc {
    /* Information about camera devices connected to the host.
     * Note that once initialized, entries in this array are considered to be
     * constant. */
    CameraInfo  camera_info[MAX_CAMERA];
    /* Number of camera devices connected to the host. */
    int         camera_count;
};

/* One and only one camera service. */
static CameraServiceDesc    _camera_service_desc;

/********************************************************************************
 * Helper routines
 *******************************************************************************/

/* Extracts query name, and (optionally) query parameters from the query string.
 * Param:
 *  query - Query string. Query string in the camera service are formatted as such:
 *          "<query name>[ <parameters>]",
 *      where parameters are optional, and if present, must be separated from the
 *      query name with a single ' '. See comments to get_token_value routine
 *      for the format of the parameters string.
 *  query_name - Upon success contains query name extracted from the query
 *      string.
 *  query_name_size - Buffer size for 'query_name' string.
 *  query_param - Upon success contains a pointer to the beginning of the query
 *      parameters. If query has no parameters, NULL will be passed back with
 *      this parameter. This parameter is optional and can be NULL.
 * Return:
 *  0 on success, or number of bytes required for query name if 'query_name'
 *  string buffer was too small to contain it.
 */
static int
_parse_query(const char* query,
             char* query_name,
             int query_name_size,
             const char** query_param)
{
    /* Extract query name. */
    const char* qend = strchr(query, ' ');
    if (qend == NULL) {
        qend = query + strlen(query);
    }
    if ((qend - query) >= query_name_size) {
        return qend - query + 1;
    }
    memcpy(query_name, query, qend - query);
    query_name[qend - query] = '\0';

    /* Calculate query parameters pointer (if needed) */
    if (query_param != NULL) {
        if (*qend == ' ') {
            qend++;
        }
        *query_param = (*qend == '\0') ? NULL : qend;
    }

    return 0;
}

/* Appends one string to another, growing the destination string buffer if
 * needed.
 * Param:
 *  str_buffer - Contains pointer to the destination string buffer. Content of
 *      this parameter can be NULL. Note that content of this parameter will
 *      change if string buffer has been reallocated.
 *  str_buf_size - Contains current buffer size of the string, addressed by
 *      'str_buffer' parameter. Note that content of this parameter will change
 *      if string buffer has been reallocated.
 *  str - String to append.
 * Return:
 *  0 on success, or -1 on failure (memory allocation).
 */
static int
_append_string(char** str_buf, size_t* str_buf_size, const char* str)
{
    const size_t offset = (*str_buf != NULL) ? strlen(*str_buf) : 0;
    const size_t append_bytes = strlen(str) + 1;

    /* Make sure these two match. */
    if (*str_buf == NULL) {
        *str_buf_size = 0;
    }

    if ((offset + append_bytes) > *str_buf_size) {
        /* Reallocate string, so it can fit what's being append to it. Note that
         * we reallocate a bit bigger buffer than is needed in order to minimize
         * number of memory allocation calls in case there are more "appends"
         * coming. */
        const size_t required_mem = offset + append_bytes + 256;
        char* new_buf = (char*)realloc(*str_buf, required_mem);
        if (new_buf == NULL) {
            E("%s: Unable to allocate %d bytes for a string",
              __FUNCTION__, required_mem);
            return -1;
        }
        *str_buf = new_buf;
        *str_buf_size = required_mem;
    }
    memcpy(*str_buf + offset, str, append_bytes);

    return 0;
}

/* Represents camera information as a string formatted as follows:
 *  'name=<devname> channel=<num> pix=<format> facing=<direction> framedims=<widh1xheight1,...>\n'
 * Param:
 *  ci - Camera information descriptor to convert into a string.
 *  str - Pointer to the string buffer where to save the converted camera
 *      information descriptor. On entry, content of this parameter can be NULL.
 *      Note that string buffer addressed with this parameter may be reallocated
 *      in this routine, so (if not NULL) it must contain a buffer allocated with
 *      malloc.  The caller is responsible for freeing string buffer returned in
 *      this parameter.
 *  str_size - Contains byte size of the buffer addressed by 'str' parameter.
 * Return:
 *  0 on success, or != 0 on failure.
 */
static int
_camera_info_to_string(const CameraInfo* ci, char** str, size_t* str_size) {
    int res;
    int n;
    char tmp[128];

    /* Append device name. */
    snprintf(tmp, sizeof(tmp), "name=%s ", ci->device_name);
    res = _append_string(str, str_size, tmp);
    if (res) {
        return res;
    }
    /* Append input channel. */
    snprintf(tmp, sizeof(tmp), "channel=%d ", ci->inp_channel);
    res = _append_string(str, str_size, tmp);
    if (res) {
        return res;
    }
    /* Append pixel format. */
    snprintf(tmp, sizeof(tmp), "pix=%d ", ci->pixel_format);
    res = _append_string(str, str_size, tmp);
    if (res) {
        return res;
    }
    /* Append direction. */
    snprintf(tmp, sizeof(tmp), "dir=%s ", ci->direction);
    res = _append_string(str, str_size, tmp);
    if (res) {
        return res;
    }
    /* Append supported frame sizes. */
    snprintf(tmp, sizeof(tmp), "framedims=%dx%d",
             ci->frame_sizes[0].width, ci->frame_sizes[0].height);
    res = _append_string(str, str_size, tmp);
    if (res) {
        return res;
    }
    for (n = 1; n < ci->frame_sizes_num; n++) {
        if (ci->frame_sizes[n].width > 1280 || ci->frame_sizes[n].height > 1280) {
            /* Guest cannot handle large pictures due to memory allocation problem
               bug:30835259
             */
            continue;
        }
        snprintf(tmp, sizeof(tmp), ",%dx%d",
                 ci->frame_sizes[n].width, ci->frame_sizes[n].height);
        res = _append_string(str, str_size, tmp);
        if (res) {
            return res;
        }
    }

    /* Stringified camera properties should end with EOL. */
    return _append_string(str, str_size, "\n");
}

/* Gets camera information matching a display name.
 * Param:
 *  disp_name - Display name to match.
 *  arr - Array of camera informations.
 *  num - Number of elements in the array.
 * Return:
 *  Matching camera information, or NULL if matching camera information for the
 *  given display name has not been found in the array.
 */
static CameraInfo* _camera_info_get_by_display_name(const char* disp_name,
                                                    CameraInfo* arr,
                                                    int num) {
    int n;
    for (n = 0; n < num; n++) {
        if (!arr[n].in_use && arr[n].display_name != NULL &&
            !strcmp(arr[n].display_name, disp_name)) {
            return &arr[n];
        }
    }
    return NULL;
}

/* Gets camera information matching a device name.
 * Param:
 *  device_name - Device name to match.
 *  arr - Array of camera informations.
 *  num - Number of elements in the array.
 * Return:
 *  Matching camera information, or NULL if matching camera information for the
 *  given device name has not been found in the array.
 */
static CameraInfo*
_camera_info_get_by_device_name(const char* device_name, CameraInfo* arr, int num)
{
    int n;
    for (n = 0; n < num; n++) {
        if (arr[n].device_name != NULL && !strcmp(arr[n].device_name, device_name)) {
            return &arr[n];
        }
    }
    return NULL;
}

/********************************************************************************
 * CameraServiceDesc API
 *******************************************************************************/

/* Initialize virtual scene camera record in camera service descriptor.
 * Param:
 *  csd - Camera service descriptor to initialize a record in.
 */
static void _virtualscenecamera_setup(CameraServiceDesc* csd) {
    /* Array containing emulated camera frame dimensions
     * expected by framework. */
    static const CameraFrameDim kEmulateDims[] = {
            {640, 480},
            /* The following dimensions are required by the camera framework. */
            {352, 288},
            {320, 240},
            {176, 144},
            {1280, 720},
            /* Additional resolutions. */
            {1280, 960}};

    csd->camera_info[csd->camera_count].frame_sizes =
            (CameraFrameDim*)malloc(sizeof(kEmulateDims));
    if (csd->camera_info[csd->camera_count].frame_sizes != NULL) {
        csd->camera_info[csd->camera_count].frame_sizes_num =
                sizeof(kEmulateDims) / sizeof(*kEmulateDims);
        memcpy(csd->camera_info[csd->camera_count].frame_sizes, kEmulateDims,
               sizeof(kEmulateDims));

        csd->camera_info[csd->camera_count].display_name =
                ASTRDUP("virtualscene");
        csd->camera_info[csd->camera_count].device_name =
                ASTRDUP("virtualscene");
        csd->camera_info[csd->camera_count].camera_source = kVirtualScene;
        csd->camera_info[csd->camera_count].inp_channel = 0;

        csd->camera_info[csd->camera_count].pixel_format =
                camera_virtualscene_preferred_format();
        csd->camera_info[csd->camera_count].direction = ASTRDUP("back");
        csd->camera_info[csd->camera_count].in_use = 0;
    }

    csd->camera_count++;
}

/* Initialize video playback camera record in camera service descriptor.
 * Param:
 *  csd - Camera service descriptor to initialize a record in.
 */
static void _videoplaybackcamera_setup(CameraServiceDesc* csd) {
    /* Array containing emulated camera frame dimensions
     * expected by framework. */
    static const CameraFrameDim kEmulateDims[] = {
            {640, 480},
            /* The following dimensions are required by the camera framework. */
            {352, 288},
            {320, 240},
            {176, 144},
            {1280, 720},
            /* Additional resolutions. */
            {1280, 960}};

    csd->camera_info[csd->camera_count].frame_sizes =
            (CameraFrameDim*)malloc(sizeof(kEmulateDims));
    if (csd->camera_info[csd->camera_count].frame_sizes != NULL) {
        csd->camera_info[csd->camera_count].frame_sizes_num =
                sizeof(kEmulateDims) / sizeof(*kEmulateDims);
        memcpy(csd->camera_info[csd->camera_count].frame_sizes, kEmulateDims,
               sizeof(kEmulateDims));

        csd->camera_info[csd->camera_count].display_name =
                ASTRDUP("videoplayback");
        csd->camera_info[csd->camera_count].device_name =
                ASTRDUP("videoplayback");
        csd->camera_info[csd->camera_count].camera_source = kVideoPlayback;
        csd->camera_info[csd->camera_count].inp_channel = 0;

        csd->camera_info[csd->camera_count].pixel_format =
                camera_videoplayback_preferred_format();
        csd->camera_info[csd->camera_count].direction = ASTRDUP("back");
        csd->camera_info[csd->camera_count].in_use = 0;
    }

    csd->camera_count++;
}

/* Initialized webcam emulation record in camera service descriptor.
 * Param:
 *  csd - Camera service descriptor to initialize a record in.
 *  disp_name - Display name of a web camera ('webcam<N>') to use for emulation.
 *  dir - Direction ('back', or 'front') that emulated camera is facing.
 *  ci, ci_cnt - Array of webcam information for enumerated web cameras connected
 *      to the host.
 */
static void _webcam_setup(CameraServiceDesc* csd,
                          const char* disp_name,
                          const char* dir,
                          CameraInfo* ci,
                          int ci_cnt) {
    /* Find webcam record in the list of enumerated web cameras. */
    CameraInfo* found = _camera_info_get_by_display_name(disp_name, ci, ci_cnt);
    if (found == NULL) {
        W("Camera name '%s' is not found in the list of connected cameras.\n"
          "Use '-webcam-list' emulator option to obtain the list of connected camera names.\n",
          disp_name);
        return;
    }

    /* Save to the camera info array that will be used by the service. */
    camera_info_copy(&csd->camera_info[csd->camera_count], found);
    csd->camera_info[csd->camera_count].camera_source = kWebcam;
    /* This camera is taken. */
    found->in_use = 1;
    /* Update direction parameter. */
    if (csd->camera_info[csd->camera_count].direction != NULL) {
        free(csd->camera_info[csd->camera_count].direction);
    }
    csd->camera_info[csd->camera_count].direction = ASTRDUP(dir);
    D("Camera %d '%s' connected to '%s' facing %s using %.4s pixel format",
      csd->camera_count, csd->camera_info[csd->camera_count].display_name,
      csd->camera_info[csd->camera_count].device_name,
      csd->camera_info[csd->camera_count].direction,
      (const char*)(&csd->camera_info[csd->camera_count].pixel_format));

    csd->camera_count++;
}

/* Initializes camera service descriptor.
 */
static void
_camera_service_init(CameraServiceDesc* csd)
{
    /* Enumerate camera devices connected to the host. */
    memset(csd->camera_info, 0, sizeof(CameraInfo) * MAX_CAMERA);
    csd->camera_count = 0;

    if (androidHwConfig_hasVirtualSceneCamera(android_hw)) {
        /* Set up virtual scene camera emulation. */
        _virtualscenecamera_setup(csd);
    }

    if (androidHwConfig_hasVideoPlaybackCamera(android_hw)) {
        _videoplaybackcamera_setup(csd);
    }

    /* Lets see if HW config uses emulated cameras. */
    if (!strncmp(android_hw->hw_camera_back, "webcam", 6) ||
        !strncmp(android_hw->hw_camera_front, "webcam", 6)) {
        int connected_cnt = 0;
        CameraInfo ci[MAX_CAMERA] = {};

        /* Enumerate web cameras connected to the host. */
        connected_cnt = camera_enumerate_devices(ci, MAX_CAMERA);
        if (connected_cnt <= 0) {
            /* Nothing is connected - nothing to emulate. */
            return;
        }

        /* Set up back camera emulation. */
        if (!strncmp(android_hw->hw_camera_back, "webcam", 6)) {
            _webcam_setup(csd, android_hw->hw_camera_back, "back", ci,
                          connected_cnt);
        }

        /* Set up front camera emulation. */
        if (!strncmp(android_hw->hw_camera_front, "webcam", 6)) {
            _webcam_setup(csd, android_hw->hw_camera_front, "front", ci,
                          connected_cnt);
        }

        int i;
        for (i = 0; i < connected_cnt; ++i) {
            camera_info_done(&ci[i]);
        }
    }
}

/* Gets camera information for the given camera device name.
 * Param:
 *  cs - Initialized camera service descriptor.
 *  device_name - Camera's device name to look up the information for.
 * Return:
 *  Camera information pointer on success, or NULL if no camera information has
 *  been found for the given device name.
 */
static CameraInfo*
_camera_service_get_camera_info_by_device_name(CameraServiceDesc* cs,
                                               const char* device_name)
{
    return _camera_info_get_by_device_name(device_name, cs->camera_info,
                                           cs->camera_count);
}

/********************************************************************************
 * Helpers for handling camera client queries
 *******************************************************************************/

/* Formats paload size according to the protocol, and sends it to the client.
 * To simplify endianess handling we convert payload size to an eight characters
 * string, representing payload size value in hexadecimal format.
 * Param:
 *  qc - Qemu client to send the payload size to.
 *  payload_size - Payload size to report to the client.
 */
static void
_qemu_client_reply_payload(QemudClient* qc, size_t payload_size)
{
    char payload_size_str[9];
    snprintf(payload_size_str, sizeof(payload_size_str), "%08x", (uint32_t)payload_size);
    qemud_client_send(qc, (const uint8_t*)payload_size_str, 8);
}

/*
 * Prefixes for replies to camera client queries.
 */

/* Success, no data to send in reply. */
#define OK_REPLY        "ok"
/* Failure, no data to send in reply. */
#define KO_REPLY        "ko"
/* Success, there are data to send in reply. */
#define OK_REPLY_DATA   OK_REPLY ":"
/* Failure, there are data to send in reply. */
#define KO_REPLY_DATA   KO_REPLY ":"

/* Builds and sends a reply to a query.
 * All replies to a query in camera service have a prefix indicating whether the
 * query has succeeded ("ok"), or failed ("ko"). The prefix can be followed by
 * extra data, containing response to the query. In case there are extra data,
 * they are separated from the prefix with a ':' character.
 * Param:
 *  qc - Qemu client to send the reply to.
 *  ok_ko - An "ok", or "ko" selector, where 0 is for "ko", and !0 is for "ok".
 *  extra - Optional extra query data. Can be NULL.
 *  extra_size - Extra data size.
 */
static void
_qemu_client_query_reply(QemudClient* qc,
                         int ok_ko,
                         const void* extra,
                         size_t extra_size)
{
    const char* ok_ko_str;
    size_t payload_size;

    /* Make sure extra_size is 0 if extra is NULL. */
    if (extra == NULL && extra_size != 0) {
        W("%s: 'extra' = NULL, while 'extra_size' = %d",
          __FUNCTION__, (int)extra_size);
        extra_size = 0;
    }

    /* Calculate total payload size, and select appropriate 'ok'/'ko' prefix */
    if (extra_size) {
        /* 'extra' size + 2 'ok'/'ko' bytes + 1 ':' separator byte. */
        payload_size = extra_size + 3;
        ok_ko_str = ok_ko ? OK_REPLY_DATA : KO_REPLY_DATA;
    } else {
        /* No extra data: just zero-terminated 'ok'/'ko'. */
        payload_size = 3;
        ok_ko_str = ok_ko ? OK_REPLY : KO_REPLY;
    }

    /* Send payload size first. */
    _qemu_client_reply_payload(qc, payload_size);
    /* Send 'ok[:]'/'ko[:]' next. Note that if there is no extra data, we still
     * need to send a zero-terminator for 'ok'/'ko' string instead of the ':'
     * separator. So, one way or another, the prefix is always 3 bytes. */
    qemud_client_send(qc, (const uint8_t*)ok_ko_str, 3);
    /* Send extra data (if present). */
    if (extra != NULL) {
        qemud_client_send(qc, (const uint8_t*)extra, extra_size);
    }
}

/* Replies query success ("OK") back to the client.
 * Param:
 *  qc - Qemu client to send the reply to.
 *  ok_str - An optional string containing query results. Can be NULL.
 */
static void
_qemu_client_reply_ok(QemudClient* qc, const char* ok_str)
{
    _qemu_client_query_reply(qc, 1, ok_str,
                             (ok_str != NULL) ? (strlen(ok_str) + 1) : 0);
}

/* Replies query failure ("KO") back to the client.
 * Param:
 *  qc - Qemu client to send the reply to.
 *  ko_str - An optional string containing reason for failure. Can be NULL.
 */
static void
_qemu_client_reply_ko(QemudClient* qc, const char* ko_str)
{
    _qemu_client_query_reply(qc, 0, ko_str,
                             (ko_str != NULL) ? (strlen(ko_str) + 1) : 0);
}

/********************************************************************************
 * Camera Factory API
 *******************************************************************************/

/* Handles 'list' query received from the Factory client.
 * Response to this query is a string that represents each connected camera in
 * this format: 'name=devname framedims=widh1xheight1,widh2xheight2,widhNxheightN\n'
 * Strings, representing each camera are separated with EOL symbol.
 * Param:
 *  csd, client - Factory serivice, and client.
 * Return:
 *  0 on success, or != 0 on failure.
 */
static int
_factory_client_list_cameras(CameraServiceDesc* csd, QemudClient* client)
{
    int n;
    size_t reply_size = 0;
    char* reply = NULL;

    /* Lets see if there was anything found... */
    if (csd->camera_count == 0) {
        /* No cameras connected to the host. Reply with "\n" */
        _qemu_client_reply_ok(client, "\n");
        return 0;
    }

    /* "Stringify" each camera information into the reply string. */
    for (n = 0; n < csd->camera_count; n++) {
        const int res =
            _camera_info_to_string(csd->camera_info + n, &reply, &reply_size);
        if (res) {
            if (reply != NULL) {
                free(reply);
            }
            _qemu_client_reply_ko(client, "Memory allocation error");
            return res;
        }
    }

    D("%s Replied: %s", __FUNCTION__, reply);
    _qemu_client_reply_ok(client, reply);
    free(reply);

    return 0;
}

/* Handles a message received from the emulated camera factory client.
 * Queries received here are represented as strings:
 *  'list' - Queries list of cameras connected to the host.
 * Param:
 *  opaque - Camera service descriptor.
 *  msg, msglen - Message received from the camera factory client.
 *  client - Camera factory client pipe.
 */
static void
_factory_client_recv(void*         opaque,
                     uint8_t*      msg,
                     int           msglen,
                     QemudClient*  client)
{
    /*
     * Emulated camera factory client queries.
     */

    /* List cameras connected to the host. */
    static const char _query_list[]     = "list";

    CameraServiceDesc* csd = (CameraServiceDesc*)opaque;
    char query_name[64];
    const char* query_param = NULL;

    /* Parse the query, extracting query name and parameters. */
    if (_parse_query((const char*)msg, query_name, sizeof(query_name),
                     &query_param)) {
        E("%s: Invalid format in query '%s'", __FUNCTION__, (const char*)msg);
        _qemu_client_reply_ko(client, "Invalid query format");
        return;
    }

    D("%s Camera factory query '%s'", __FUNCTION__, query_name);

    /* Dispatch the query to an appropriate handler. */
    if (!strcmp(query_name, _query_list)) {
        /* This is a "list" query. */
        _factory_client_list_cameras(csd, client);
    } else {
        E("%s: Unknown camera factory query name in '%s'",
          __FUNCTION__, (const char*)msg);
        _qemu_client_reply_ko(client, "Unknown query name");
    }
}

/* Emulated camera factory client has been disconnected from the service. */
static void
_factory_client_close(void*  opaque)
{
    /* There is nothing to clean up here: factory service is just an alias for
     * the "root" camera service, that doesn't require anything more, than camera
     * dervice descriptor already provides. */
}

/********************************************************************************
 * Camera client API
 *******************************************************************************/

/* Describes an emulated camera client.
 */
typedef struct CameraClient CameraClient;
struct CameraClient
{
    /* Client name.
     *  On Linux this is the name of the camera device.
     *  On Windows this is the name of capturing window.
     */
    char*               device_name;
    /* Input channel to use to connect to the camera. */
    int                 inp_channel;
    /* Camera information. */
    const CameraInfo*   camera_info;
    /* Emulated camera device descriptor. */
    CameraDevice*       camera;

    CameraDevice* (*open)(const char* name, int inp_channel);
    int (*start_capturing)(CameraDevice* cd,
                           uint32_t pixel_format,
                           int frame_width,
                           int frame_height);
    int (*stop_capturing)(CameraDevice* cd);
    int (*read_frame)(CameraDevice* cd,
                      ClientFrame* frame,
                      float r_scale,
                      float g_scale,
                      float b_scale,
                      float exp_comp);
    void (*close)(CameraDevice* cd);

    /* Buffer allocated for video frames.
     * Note that memory allocated for this buffer also contains preview
     * framebuffer and i420 staging framebuffer. */
    uint8_t*            video_frame;
    /* Preview frame buffer.
     * This address points inside the 'video_frame' buffer. */
    uint8_t*            preview_frame;
    /* Byte size of the videoframe buffer. */
    size_t              video_frame_size;
    /* Byte size of the preview frame buffer. */
    size_t              preview_frame_size;
    /* Staging framebuffer, used as an intermediate buffer for libyuv. */
    uint8_t*            staging_framebuffer;
    /* Staging framebuffer size. */
    size_t              staging_framebuffer_size;
    /* Pixel format required by the guest. */
    uint32_t            pixel_format;
    /* Frame width. */
    int                 width;
    /* Frame height. */
    int                 height;
    /* Number of pixels in a frame buffer. */
    int                 pixel_num;
    /* Status of preview frame cache. */
    int                 frames_cached;
    /* Queries being sent from the guest can be interrupted, resulting in the camera receiving
       the partial text of a query.  (This can be detected by the query not ending with a
       terminating 0 character.)  In that case, the partial command is stored in command_buffer,
       and command_buffer_offset records the length of the messages received so far, and thus where
       the next segment should be written.
       */
    char command_buffer[MAX_QUERY_MESSAGE_SIZE];
    int  command_buffer_offset;
    /* Total number of frames rendered, used for metrics. */
    uint64_t            frame_count;
};

/* Frees emulated camera client descriptor. */
static void
_camera_client_free(CameraClient* cc)
{
    /* The only exception to the "read only" rule: we have to mark the camera
     * as being not used when we destroy a service for it. */
    if (cc->camera_info != NULL) {
        ((CameraInfo*)cc->camera_info)->in_use = 0;
    }
    if (cc->camera != NULL) {
        cc->close(cc->camera);
    }
    if (cc->video_frame != NULL) {
        free(cc->video_frame);
    }
    if (cc->device_name != NULL) {
        free(cc->device_name);
    }

    AFREE(cc);
}

/* Creates descriptor for a connecting emulated camera client.
 * Param:
 *  csd - Camera service descriptor.
 *  param - Client parameters. Must be formatted as described in comments to
 *      get_token_value routine, and must contain at least 'name' parameter,
 *      identifiying the camera device to create the service for. Also parameters
 *      may contain a decimal 'inp_channel' parameter, selecting the input
 *      channel to use when communicating with the camera device.
 * Return:
 *  Emulated camera client descriptor on success, or NULL on failure.
 */
static CameraClient*
_camera_client_create(CameraServiceDesc* csd, const char* param)
{
    CameraClient* cc;
    CameraInfo* ci;
    int res;
    ANEW0(cc);

    /*
     * Parse parameter string, containing camera client properties.
     */

    /* Pull required device name. */
    if (get_token_value_alloc(param, "name", &cc->device_name)) {
        E("%s: Allocation failure, or required 'name' parameter is missing, or misformed in '%s'",
          __FUNCTION__, param);
        return NULL;
    }

    /* Pull optional input channel. */
    res = get_token_value_int(param, "inp_channel", &cc->inp_channel);
    if (res != 0) {
        if (res == -1) {
            /* 'inp_channel' parameter has been ommited. Use default input
             * channel, which is zero. */
            cc->inp_channel = 0;
        } else {
            E("%s: 'inp_channel' parameter is misformed in '%s'",
              __FUNCTION__, param);
            return NULL;
        }
    }

    /* Get camera info for the emulated camera represented with this service.
     * Array of camera information records has been created when the camera
     * service was enumerating camera devices during the service initialization.
     * By the camera service protocol, camera service clients must first obtain
     * list of enumerated cameras via the 'list' query to the camera service, and
     * then use device name reported in the list to connect to an emulated camera
     * service. So, if camera information for the given device name is not found
     * in the array, we fail this connection due to protocol violation. */
    ci = _camera_service_get_camera_info_by_device_name(csd, cc->device_name);
    if (ci == NULL) {
        E("%s: Cannot find camera info for device '%s'",
          __FUNCTION__, cc->device_name);
        _camera_client_free(cc);
        return NULL;
    }

    /* We can't allow multiple camera services for a single camera device, Lets
     * make sure that there is no client created for this camera. */
    if (ci->in_use) {
        E("%s: Camera device '%s' is in use", __FUNCTION__, cc->device_name);
        _camera_client_free(cc);
        return NULL;
    }

    /* We're done. Set camera in use, and succeed the connection. */
    ci->in_use = 1;
    cc->camera_info = ci;

    if (ci->camera_source == kVirtualScene) {
        cc->open = camera_virtualscene_open;
        cc->start_capturing = camera_virtualscene_start_capturing;
        cc->stop_capturing = camera_virtualscene_stop_capturing;
        cc->read_frame = camera_virtualscene_read_frame;
        cc->close = camera_virtualscene_close;
    } else if (ci->camera_source == kVideoPlayback) {
        cc->open = camera_videoplayback_open;
        cc->start_capturing = camera_videoplayback_start_capturing;
        cc->stop_capturing = camera_videoplayback_stop_capturing;
        cc->read_frame = camera_videoplayback_read_frame;
        cc->close = camera_videoplayback_close;
    } else {
        cc->open = camera_device_open;
        cc->start_capturing = camera_device_start_capturing;
        cc->stop_capturing = camera_device_stop_capturing;
        cc->read_frame = camera_device_read_frame;
        cc->close = camera_device_close;
    }

    D("%s: Camera service is created for device '%s' using input channel %d",
      __FUNCTION__, cc->device_name, cc->inp_channel);

    return cc;
}

/********************************************************************************
 * Camera client queries
 *******************************************************************************/

/* Returns current time in microseconds. */
static __inline__ uint64_t _get_timestamp(void) {
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    return (uint64_t)t.tv_sec * 1000000LL + t.tv_usec;
}

/* Sleeps for the given amount of milliseconds */
static __inline__ void _camera_sleep(int millisec) {
    struct timeval t;
    const uint64_t wake_at = _get_timestamp() + (uint64_t)millisec * 1000;
    do {
        const uint64_t stamp = _get_timestamp();
        if ((stamp / 1000) >= (wake_at / 1000)) {
            break;
        }
        t.tv_sec = (wake_at - stamp) / 1000000;
        t.tv_usec = (wake_at - stamp) - (uint64_t)t.tv_sec * 1000000;
    } while (select(0, NULL, NULL, NULL, &t) < 0 && errno == EINTR);
}

/* Client has queried conection to the camera.
 * Param:
 *  cc - Queried camera client descriptor.
 *  qc - Qemu client for the emulated camera.
 *  param - Query parameters. There are no parameters expected for this query.
 */
static void
_camera_client_query_connect(CameraClient* cc, QemudClient* qc, const char* param)
{
    if (cc->camera != NULL) {
        /* Already connected. */
        W("%s: Camera '%s' is already connected", __FUNCTION__, cc->device_name);
        _qemu_client_reply_ok(qc, "Camera is already connected");
        return;
    }

    /* Open camera device. */
    cc->camera = cc->open(cc->device_name, cc->inp_channel);

    if (cc->camera == NULL) {
        E("%s: Unable to open camera device '%s'", __FUNCTION__, cc->device_name);
        _qemu_client_reply_ko(qc, "Unable to open camera device.");
        return;
    }

    D("%s: Camera device '%s' is now connected", __FUNCTION__, cc->device_name);

    _qemu_client_reply_ok(qc, NULL);
}

/* Client has queried disconection from the camera.
 * Param:
 *  cc - Queried camera client descriptor.
 *  qc - Qemu client for the emulated camera.
 *  param - Query parameters. There are no parameters expected for this query.
 */
static void
_camera_client_query_disconnect(CameraClient* cc,
                                QemudClient* qc,
                                const char* param)
{
    if (cc->camera == NULL) {
        /* Already disconnected. */
        W("%s: Camera '%s' is already disconnected", __FUNCTION__, cc->device_name);
        _qemu_client_reply_ok(qc, "Camera is not connected");
        return;
    }

    /* Before we can go ahead and disconnect, we must make sure that camera is
     * not capturing frames. */
    if (cc->video_frame != NULL) {
        E("%s: Cannot disconnect camera '%s' while it is not stopped",
          __FUNCTION__, cc->device_name);
        _qemu_client_reply_ko(qc, "Camera is not stopped");
        return;
    }

    /* Close camera device. */
    cc->close(cc->camera);
    cc->camera = NULL;

    D("Camera device '%s' is now disconnected", cc->device_name);

    _qemu_client_reply_ok(qc, NULL);
}

/* Start capturing video with the given frame params
 * Param:
 *  cc - Queried camera client descriptor.
 *  width - Requested frame width.
 *  height - Requested frame height.
 *  pix_format - Requested pixel format.
 * Returns:
 *  The result of the client start attempt as a ClientStartResult.  Note that
 *  success codes are > 0, error codes are less than 0.
 */
static ClientStartResult
_camera_client_start(CameraClient* cc, int width, int height, int pix_format) {
    camera_metrics_report_start_session(cc->camera_info->camera_source,
                                        cc->camera_info->direction, width,
                                        height, pix_format);

    /* After collecting capture parameters lets see if camera has already
     * started, and if so, lets see if parameters match. */
    if (cc->video_frame != NULL) {
        /* Already started. Match capture parameters. */
        if (cc->pixel_format != (uint32_t)pix_format ||cc->width != width ||
            cc->height != height) {
            W("%s: Camera '%s' is already started", __FUNCTION__, cc->device_name);
            return CLIENT_START_RESULT_ALREADY_STARTED;
        } else {
            /* Parameters don't match. Fail the query. */
            E("%s: Camera '%s' is already started, and parameters don't match:\n"
              "Current %.4s[%dx%d] != requested %.4s[%dx%d]",
              __FUNCTION__, cc->device_name, (const char*)&cc->pixel_format,
              cc->width, cc->height, (const char*)&pix_format, width, height);
            return CLIENT_START_RESULT_PARAMETER_MISMATCH;
        }
    }

    /*
     * Start the camera.
     */

    /* Save capturing parameters. */
    cc->pixel_format = pix_format;
    cc->width = width;
    cc->height = height;
    cc->pixel_num = cc->width * cc->height;
    cc->frames_cached = 0;
    cc->frame_count = 0;
    cc->staging_framebuffer = NULL;
    cc->staging_framebuffer_size = 0;

    /* Make sure that pixel format is known, and calculate video/preview
     * framebuffer size along the lines. */
    if (!calculate_framebuffer_size(cc->pixel_format, cc->width, cc->height,
                                    &cc->video_frame_size)) {
        E("%s: Unknown pixel format %.4s",
          __FUNCTION__, (char*)&cc->pixel_format);
        return CLIENT_START_RESULT_UNKNOWN_PIXEL_FORMAT;
    }

    /* TODO: At the moment camera framework in the emulator requires RGB32 pixel
     * format for preview window. So, we need to keep two framebuffers here: one
     * for the video, and another for the preview window. Watch out when this
     * changes (if changes). */
    cc->preview_frame_size = cc->width * cc->height * 4;

    /* Make sure that we have a converters between the original camera pixel
     * format and the one that the client expects. Also a converter must exist
     * for the preview window pixel format (RGB32) */
    if (!has_converter(cc->camera_info->pixel_format, cc->pixel_format) ||
        !has_converter(cc->camera_info->pixel_format, V4L2_PIX_FMT_RGB32)) {
        E("%s: No conversion exist between %.4s and %.4s (or RGB32) pixel formats",
          __FUNCTION__, (char*)&cc->camera_info->pixel_format, (char*)&cc->pixel_format);
        return CLIENT_START_RESULT_NO_PIXEL_CONVERSION;
    }

    /* Allocate buffer large enough to contain both, video and preview
     * framebuffers. */
    cc->video_frame =
            (uint8_t*)malloc(cc->video_frame_size + cc->preview_frame_size);
    if (cc->video_frame == NULL) {
        E("%s: Not enough memory for framebuffers %d + %d", __FUNCTION__,
          cc->video_frame_size, cc->preview_frame_size);
        return CLIENT_START_RESULT_OUT_OF_MEMORY;
    }

    /* Set framebuffer pointers. */
    cc->preview_frame = cc->video_frame + cc->video_frame_size;

    /* Start the camera. */
    if (cc->start_capturing(cc->camera, cc->camera_info->pixel_format,
                            cc->width, cc->height)) {
        E("%s: Cannot start camera '%s' for %.4s[%dx%d]: %s",
          __FUNCTION__, cc->device_name, (const char*)&cc->pixel_format,
          cc->width, cc->height, strerror(errno));
        free(cc->video_frame);
        cc->video_frame = NULL;
        return CLIENT_START_RESULT_FAILED;
    }

    D("%s: Camera '%s' is now started for %.4s[%dx%d]",
      __FUNCTION__, cc->device_name, (char*)&cc->pixel_format, cc->width,
      cc->height);

    return CLIENT_START_RESULT_SUCCESS;
}

/* Client has queried the client to start capturing video.
 * Param:
 *  cc - Queried camera client descriptor.
 *  qc - Qemu client for the emulated camera.
 *  param - Query parameters. Parameters for this query must contain a 'dim', and
 *      a 'pix' parameters, where 'dim' must be "dim=<width>x<height>", and 'pix'
 *      must be "pix=<format>", where 'width' and 'height' must be numerical
 *      values for the capturing video frame width, and height, and 'format' must
 *      be a numerical value for the pixel format of the video frames expected by
 *      the client. 'format' must be one of the V4L2_PIX_FMT_XXX values.
 */
static void
_camera_client_query_start(CameraClient* cc, QemudClient* qc, const char* param)
{
    char* w;
    char dim[64];
    int width, height, pix_format;

    /* Sanity check. */
    if (cc->camera == NULL) {
        /* Not connected. */
        E("%s: Camera '%s' is not connected", __FUNCTION__, cc->device_name);
        _qemu_client_reply_ko(qc, "Camera is not connected");
        return;
    }

    /*
     * Parse parameters.
     */

    if (param == NULL) {
        E("%s: Missing parameters for the query", __FUNCTION__);
        _qemu_client_reply_ko(qc, "Missing parameters for the query");
        return;
    }

    /* Pull required 'dim' parameter. */
    if (get_token_value(param, "dim", dim, sizeof(dim))) {
        E("%s: Invalid or missing 'dim' parameter in '%s'", __FUNCTION__, param);
        _qemu_client_reply_ko(qc, "Invalid or missing 'dim' parameter");
        return;
    }

    /* Pull required 'pix' parameter. */
    if (get_token_value_int(param, "pix", &pix_format)) {
        E("%s: Invalid or missing 'pix' parameter in '%s'", __FUNCTION__, param);
        _qemu_client_reply_ko(qc, "Invalid or missing 'pix' parameter");
        return;
    }

    /* Parse 'dim' parameter, and get requested frame width and height. */
    w = strchr(dim, 'x');
    if (w == NULL || w[1] == '\0') {
        E("%s: Invalid 'dim' parameter in '%s'", __FUNCTION__, param);
        _qemu_client_reply_ko(qc, "Invalid 'dim' parameter");
        return;
    }
    *w = '\0'; w++;
    errno = 0;
    width = strtoi(dim, NULL, 10);
    height = strtoi(w, NULL, 10);
    if (errno) {
        E("%s: Invalid 'dim' parameter in '%s'", __FUNCTION__, param);
        _qemu_client_reply_ko(qc, "Invalid 'dim' parameter");
        return;
    }

    ClientStartResult result =
            _camera_client_start(cc, width, height, pix_format);
    camera_metrics_report_start_result(result);

    if (result < 0) {
        camera_metrics_report_stop_session(0);
    }

    switch (result) {
        case CLIENT_START_RESULT_SUCCESS:
            _qemu_client_reply_ok(qc, NULL);
            break;
        case CLIENT_START_RESULT_ALREADY_STARTED:
            _qemu_client_reply_ok(qc, "Camera is already started");
            break;
        case CLIENT_START_RESULT_PARAMETER_MISMATCH:
            _qemu_client_reply_ko(qc, "Camera is already started with different capturing parameters");
            break;
        case CLIENT_START_RESULT_UNKNOWN_PIXEL_FORMAT:
            _qemu_client_reply_ko(qc, "Pixel format is unknown");
            break;
        case CLIENT_START_RESULT_NO_PIXEL_CONVERSION:
            _qemu_client_reply_ko(qc, "No conversion exist for the requested pixel format");
            break;
        case CLIENT_START_RESULT_OUT_OF_MEMORY:
            _qemu_client_reply_ko(qc, "Out of memory");
            break;
        default:
            E("%s: Unexpected capture result '%d'", __FUNCTION__, result);
            // Intentional fallthrough.
        case CLIENT_START_RESULT_FAILED:
            _qemu_client_reply_ko(qc, "Cannot start the camera");
            break;
    }
}

/* Client has queried the client to stop capturing video.
 * Param:
 *  cc - Queried camera client descriptor.
 *  qc - Qemu client for the emulated camera.
 *  param - Query parameters. There are no parameters expected for this query.
 */
static void
_camera_client_query_stop(CameraClient* cc, QemudClient* qc, const char* param)
{
    if (cc->video_frame == NULL) {
        /* Not started. */
        W("%s: Camera '%s' is not started", __FUNCTION__, cc->device_name);
        _qemu_client_reply_ok(qc, "Camera is not started");
        return;
    }

    /* Stop the camera. */
    if (cc->stop_capturing(cc->camera)) {
        E("%s: Cannot stop camera device '%s': %s",
          __FUNCTION__, cc->device_name, strerror(errno));
        _qemu_client_reply_ko(qc, "Cannot stop camera device");
        return;
    }

    free(cc->video_frame);
    cc->video_frame = NULL;

    free(cc->staging_framebuffer);
    cc->staging_framebuffer = NULL;

    camera_metrics_report_stop_session(cc->frame_count);

    D("%s: Camera device '%s' is now stopped.", __FUNCTION__, cc->device_name);
    _qemu_client_reply_ok(qc, NULL);
}

/* Client has queried next frame.
 * Param:
 *  cc - Queried camera client descriptor.
 *  qc - Qemu client for the emulated camera.
 *  param - Query parameters. Parameters for this query are formatted as such:
 *          video=<size> preview=<size> whiteb=<red>,<green>,<blue> expcomp=<comp> time=<1,0>
 *      where:
 *       - 'video', and 'preview' both must be decimal values, defining size of
 *         requested video, and preview frames respectively. Zero value for any
 *         of these parameters means that this particular frame is not requested.
 *       - whiteb contains float values required to calculate whilte balance.
 *       - expcomp contains a float value required to calculate exposure
 *         compensation.
 *       - time=1 is passed when the image is requesting a response with the
 *         frame time included.
 */
static void
_camera_client_query_frame(CameraClient* cc, QemudClient* qc, const char* param)
{
    int video_size = 0;
    int preview_size = 0;
    int repeat;
    ClientFrameBuffer fbs[2];
    int fbs_num = 0;
    size_t payload_size;
    uint64_t tick;
    float r_scale = 1.0f, g_scale = 1.0f, b_scale = 1.0f, exp_comp = 1.0f;
    char tmp[256];
    int send_frame_time = 0;
    ClientFrame frame = {};

    /* Sanity check. */
    if (cc->video_frame == NULL) {
        /* Not started. */
        E("%s: Camera '%s' is not started", __FUNCTION__, cc->device_name);
        _qemu_client_reply_ko(qc, "Camera is not started");
        return;
    }

    /* Pull required parameters. */
    if (get_token_value_int(param, "video", &video_size) ||
        get_token_value_int(param, "preview", &preview_size)) {
        E("%s: Invalid or missing 'video', or 'preview' parameter in '%s'",
          __FUNCTION__, param);
        _qemu_client_reply_ko(qc,
            "Invalid or missing 'video', or 'preview' parameter");
        return;
    }

    /* Pull white balance values. */
    if (!get_token_value(param, "whiteb", tmp, sizeof(tmp))) {
        if (sscanf(tmp, "%g,%g,%g", &r_scale, &g_scale, &b_scale) != 3) {
            D("Invalid value '%s' for parameter 'whiteb'", tmp);
            r_scale = g_scale = b_scale = 1.0f;
        }
    }

    /* Pull exposure compensation. */
    if (!get_token_value(param, "expcomp", tmp, sizeof(tmp))) {
        if (sscanf(tmp, "%g", &exp_comp) != 1) {
            D("Invalid value '%s' for parameter 'whiteb'", tmp);
            exp_comp = 1.0f;
        }
    }

    if (get_token_value_int(param, "time", &send_frame_time) < 0) {
        send_frame_time = 0;
    }

    /* Verify that framebuffer sizes match the ones that the started camera
     * operates with. */
    if ((video_size != 0 && cc->video_frame_size != (size_t)video_size) ||
        (preview_size != 0 && cc->preview_frame_size != (size_t)preview_size)) {
        E("%s: Frame sizes don't match for camera '%s':\n"
          "Expected %d for video, and %d for preview. Requested %d, and %d",
          __FUNCTION__, cc->device_name, cc->video_frame_size,
          cc->preview_frame_size, video_size, preview_size);
        _qemu_client_reply_ko(qc, "Frame size mismatch");
        return;
    }

    /*
     * Initialize framebuffer array for frame read.
     */

    if (video_size) {
        fbs[fbs_num].pixel_format = cc->pixel_format;
        fbs[fbs_num].framebuffer = cc->video_frame;
        fbs_num++;
    }
    if (preview_size) {
        /* TODO: Watch out for preview format changes! */
        fbs[fbs_num].pixel_format = V4L2_PIX_FMT_RGB32;
        fbs[fbs_num].framebuffer = cc->preview_frame;
        fbs_num++;
    }

    /* Capture new frame. */
    tick = _get_timestamp();

    frame.framebuffers_count = fbs_num;
    frame.framebuffers = fbs;
    frame.staging_framebuffer = &cc->staging_framebuffer;
    frame.staging_framebuffer_size = &cc->staging_framebuffer_size;
    frame.frame_time =
            looper_nowNsWithClock(looper_getForThread(), LOOPER_CLOCK_VIRTUAL);

    repeat = cc->read_frame(cc->camera, &frame, r_scale, g_scale, b_scale,
                            exp_comp);

    /* Note that there is no (known) way how to wait on next frame being
     * available, so we could dequeue frame buffer from the device only when we
     * know it's available. Instead we're shooting in the dark, and quite often
     * device will response with EAGAIN, indicating that it doesn't have frame
     * ready. In turn, it means that the last frame we have obtained from the
     * device is still good, and we can reply with the cached frames. The only
     * case when we need to keep trying to obtain a new frame is when frame cache
     * is empty. To prevent ourselves from an indefinite loop in case device got
     * stuck on something (observed with some Microsoft devices) we will limit
     * the loop by 2 second time period (which is more than enough to obtain
     * something from the device) */
    while (repeat == 1 && !cc->frames_cached &&
           (_get_timestamp() - tick) < 2000000LL) {
        /* Sleep for 10 millisec before repeating the attempt. */
        _camera_sleep(10);
        repeat = cc->read_frame(cc->camera, &frame, r_scale, g_scale, b_scale,
                                exp_comp);
    }
    if (repeat == 1 && !cc->frames_cached) {
        /* Waited too long for the first frame. */
        E("%s: Unable to obtain first video frame from the camera '%s' in %d milliseconds: %s.",
          __FUNCTION__, cc->device_name,
          (uint32_t)(_get_timestamp() - tick) / 1000, strerror(errno));
        _qemu_client_reply_ko(qc, "Unable to obtain video frame from the camera");
        return;
    } else if (repeat < 0) {
        /* An I/O error. */
        E("%s: Unable to obtain video frame from the camera '%s': %s.",
          __FUNCTION__, cc->device_name, strerror(errno));
        _qemu_client_reply_ko(qc, strerror(errno));
        return;
    }

    if (video_size && repeat == 1 && cc->frames_cached) {
        // Device has no frame update reported. Use cached preview frame.
        // Convert preview frame format to video frame format.
        if (convert_frame(cc->preview_frame, V4L2_PIX_FMT_RGB32,
                      cc->preview_frame_size,
                      cc->width, cc->height,
                      &frame, 1.0f, 1.0f, 1.0f, 1.0f)) {
            E("%s: Unable to obtain first video frame from the camera '%s'",
                __FUNCTION__, cc->device_name);
            _qemu_client_reply_ko(qc, "Unable to obtain video frame from the camera");
            return;
        }
    }

    /* We have cached something... */
    if (preview_size) {
        cc->frames_cached = 1;
    }
    ++cc->frame_count;

    /*
     * Build the reply.
     */

    /* Payload includes "ok:" + requested video and preview frames + an int64
     * frame timestamp if requested. */
    payload_size = 3 + (send_frame_time ? 8 : 0) + video_size + preview_size;

    /* Send payload size first. */
    _qemu_client_reply_payload(qc, payload_size);

    /* After that send the 'ok:'. Note that if there is no frames sent, we should
     * use prefix "ok" instead of "ok:" */
    if (video_size || preview_size) {
        qemud_client_send(qc, (const uint8_t*)"ok:", 3);
    } else {
        /* Still 3 bytes: zero terminator is required in this case. */
        qemud_client_send(qc, (const uint8_t*)"ok", 3);
    }

    /* After that send video frame (if requested). */
    if (video_size) {
        qemud_client_send(qc, cc->video_frame, video_size);
    }

    /* After that send preview frame (if requested). */
    if (preview_size) {
        qemud_client_send(qc, cc->preview_frame, preview_size);
    }

    // after that send the frame time (if requested). */
    if (send_frame_time) {
        int64_t adjusted_time = frame.frame_time +
                android_sensors_get_time_offset();

        qemud_client_send(qc, (const uint8_t*) &adjusted_time, 8);
    }
}

/* Handles a message received from the emulated camera client.
 * Queries received here are represented as strings:
 * - 'connect' - Connects to the camera device (opens it).
 * - 'disconnect' - Disconnexts from the camera device (closes it).
 * - 'start' - Starts capturing video from the connected camera device.
 * - 'stop' - Stop capturing video from the connected camera device.
 * - 'frame' - Queries video and preview frames captured from the camera.
 * Param:
 *  opaque - Camera service descriptor.
 *  msg, msglen - Message received from the camera factory client.
 *  client - Camera factory client pipe.
 */
static void
_camera_client_recv(void*         opaque,
                    uint8_t*      msg,
                    int           msglen,
                    QemudClient*  client)
{
    /*
     * Emulated camera client queries.
     */
    CameraClient* cc = (CameraClient*)opaque;

    if (msglen <= 0) {
        D("%s: query message has invalid length %d", __func__, msglen);
        return;
    }

    if (cc->command_buffer_offset + msglen >= MAX_QUERY_MESSAGE_SIZE) {
        E("%s: command buffer overflowed: existing buffer-size %d needed %d\n",
                MAX_QUERY_MESSAGE_SIZE, cc->command_buffer_offset + msglen);
        cc->command_buffer_offset = 0;
        _qemu_client_reply_ko(client, "query too long");
        return;
    }
    memcpy(cc->command_buffer + cc->command_buffer_offset, msg, msglen);
    cc->command_buffer_offset += msglen;

    if (cc->command_buffer[cc->command_buffer_offset - 1] != '\0') {
        D("%s: imcomplete query", __func__);
        /* This is a partial command, and we expect _camera_client_recv to be invoked again
           with the next segment of the command */
        return;
    }
    msg = (uint8_t*)(cc->command_buffer);
    cc->command_buffer_offset = 0;
    /* E("%s: query is '%s'\n", __func__, msg); */
    /* Connect to the camera. */
    static const char _query_connect[]    = "connect";
    /* Disconnect from the camera. */
    static const char _query_disconnect[] = "disconnect";
    /* Start video capturing. */
    static const char _query_start[]      = "start";
    /* Stop video capturing. */
    static const char _query_stop[]       = "stop";
    /* Query frame(s). */
    static const char _query_frame[]      = "frame";

    char query_name[64];
    const char* query_param = NULL;

    /*
     * Emulated camera queries are formatted as such:
     *  "<query name> [<parameters>]"
     */

    T("%s: Camera client query: '%s'", __FUNCTION__, (char*)msg);
    if (_parse_query((const char*)msg, query_name, sizeof(query_name),
        &query_param)) {
        E("%s: Invalid query '%s'", __FUNCTION__, (char*)msg);
        _qemu_client_reply_ko(client, "Invalid query");
        return;
    }

    /* Dispatch the query to an appropriate handler. */
    if (!strcmp(query_name, _query_frame)) {
        /* A frame is queried. */
        _camera_client_query_frame(cc, client, query_param);
    } else if (!strcmp(query_name, _query_connect)) {
        /* Camera connection is queried. */
        _camera_client_query_connect(cc, client, query_param);
    } else if (!strcmp(query_name, _query_disconnect)) {
        /* Camera disnection is queried. */
        _camera_client_query_disconnect(cc, client, query_param);
    } else if (!strcmp(query_name, _query_start)) {
        /* Start capturing is queried. */
        _camera_client_query_start(cc, client, query_param);
    } else if (!strcmp(query_name, _query_stop)) {
        /* Stop capturing is queried. */
        _camera_client_query_stop(cc, client, query_param);
    } else {
        E("%s: Unknown query '%s'", __FUNCTION__, (char*)msg);
        _qemu_client_reply_ko(client, "Unknown query");
    }
}

/* Emulated camera client has been disconnected from the service. */
static void
_camera_client_close(void* opaque)
{
    CameraClient* cc = (CameraClient*)opaque;

    D("%s: Camera client for device '%s' on input channel %d is now closed",
      __FUNCTION__, cc->device_name, cc->inp_channel);

    _camera_client_free(cc);
}

/* Saves the state of the camera client.
 *
 * This simply saves whether the camera is currently connected, so that it can
 * reconnect on load.
 */
static void _camera_client_save(Stream* f, QemudClient* client, void* opaque) {
    CameraClient* cc = (CameraClient*)opaque;

    stream_put_be32(f, cc->camera != NULL ? 1 : 0);
    stream_put_be32(f, cc->video_frame != NULL ? 1 : 0);
    if (cc->video_frame != NULL) {
        stream_put_be32(f, cc->pixel_format);
        stream_put_be32(f, cc->width);
        stream_put_be32(f, cc->height);
    }
}

static int _camera_client_load(Stream* f, QemudClient* client, void* opaque) {
    CameraClient* cc = (CameraClient*)opaque;

    int is_camera_connected = stream_get_be32(f);
    if (is_camera_connected && cc->camera == NULL) {
        cc->camera = cc->open(cc->device_name, cc->inp_channel);
        if (cc->camera == NULL) {
            D("%s: failed to start camera service required in snapshot.\n",
              __FUNCTION__);
            return -EIO;
        }
    }

    // Try to stop the camera if it is already started in order to avoid a frame
    // size or format mismatch.
    if (cc->video_frame != NULL) {
        if (cc->stop_capturing(cc->camera) == 0) {
            free(cc->video_frame);
            cc->video_frame = NULL;
        } else {
            D("%s: failed to stop running camera stream.\n",
              __FUNCTION__);
            return -EIO;
        }
    }

    int is_camera_started = stream_get_be32(f);
    if (is_camera_started) {
        int pixel_format = stream_get_be32(f);
        int width = stream_get_be32(f);
        int height = stream_get_be32(f);

        ClientStartResult result =
                _camera_client_start(cc, width, height, pixel_format);
        camera_metrics_report_start_result(result);
        if (result < 0) {
            D("%s: failed to start camera service required in snapshot.\n",
              __FUNCTION__);
            camera_metrics_report_stop_session(0);
            return -EIO;
        }
    }

    return 0;
}

/********************************************************************************
 * Camera service API
 *******************************************************************************/

/* Connects a client to the camera service.
 * There are two classes of the client that can connect to the service:
 *  - Camera factory that is interested only in listing camera devices attached
 *    to the host.
 *  - Camera device emulators that attach to the actual camera devices.
 * The distinction between these two classes is made by looking at extra
 * parameters passed in client_param variable. If it's NULL, or empty, the
 * client connects to a camera factory. Otherwise, parameters describe the
 * camera device the client wants to connect to.
 */
static QemudClient*
_camera_service_connect(void*          opaque,
                        QemudService*  serv,
                        int            channel,
                        const char*    client_param)
{
    QemudClient*  client = NULL;
    CameraServiceDesc* csd = (CameraServiceDesc*)opaque;

    D("%s: Connecting camera client '%s'", __FUNCTION__,
      client_param ? client_param : "Factory");
    if (client_param == NULL || *client_param == '\0') {
        /* This is an emulated camera factory client. */
        client = qemud_client_new(serv, channel, client_param, csd,
                                  _factory_client_recv, _factory_client_close,
                                  NULL, NULL);
    } else {
        /* This is an emulated camera client. */
        CameraClient* cc = _camera_client_create(csd, client_param);
        if (cc != NULL) {
            client = qemud_client_new(serv, channel, client_param, cc,
                                      _camera_client_recv, _camera_client_close,
                                      _camera_client_save, _camera_client_load);
        }
    }

    return client;
}

void android_camera_service_init(void) {
    static int _inited = 0;

    if (!_inited) {
        _camera_service_init(&_camera_service_desc);
        QemudService*  serv = qemud_service_register( SERVICE_NAME, 0,
                &_camera_service_desc,
                _camera_service_connect,
                NULL,
                NULL);
        if (serv == NULL) {
            derror("%s: Could not register '%s' service",
                    __FUNCTION__, SERVICE_NAME);
            return;
        }

        if (strcmp(android_hw->hw_camera_back, "emulated") &&
                strcmp(android_hw->hw_camera_front, "emulated")) {
            /* Fake camera is not used for camera emulation. */
            boot_property_add("qemu.sf.fake_camera", "none");
        } else {
            if(!strcmp(android_hw->hw_camera_back, "emulated") &&
                    !strcmp(android_hw->hw_camera_front, "emulated")) {
                /* Fake camera is used for both, front and back camera emulation. */
                boot_property_add("qemu.sf.fake_camera", "both");
            } else if (!strcmp(android_hw->hw_camera_back, "emulated")) {
                boot_property_add("qemu.sf.fake_camera", "back");
            } else {
                boot_property_add("qemu.sf.fake_camera", "front");
            }
        }

        D("%s: Registered '%s' qemud service", __FUNCTION__, SERVICE_NAME);
    }
}

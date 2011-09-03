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

#include "qemu-common.h"
#include "android/hw-qemud.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#include "android/utils/debug.h"
#include "android/camera/camera-capture.h"
#include "android/camera/camera-format-converters.h"
#include "android/camera/camera-service.h"

#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  W(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  E(...)    VERBOSE_PRINT(camera,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(camera)

#define SERVICE_NAME  "camera"

/* Camera sevice descriptor. */
typedef struct CameraServiceDesc CameraServiceDesc;
struct CameraServiceDesc {
    /* Number of camera devices connected to the host. */
    int camera_count;
};

/* One and only one camera service. */
static CameraServiceDesc    _camera_service_desc;

/********************************************************************************
 * CameraServiceDesc API
 *******************************************************************************/

/* Initializes camera service descriptor.
 */
static void
_csDesc_init(CameraServiceDesc* csd)
{
    csd->camera_count = 0;
}

/********************************************************************************
 * Camera Factory API
 *******************************************************************************/

/* Handles a message received from the emulated camera factory client.
 */
static void
_factory_client_recv(void*         opaque,
                     uint8_t*      msg,
                     int           msglen,
                     QemudClient*  client)
{
    // TODO: implement.
}

/* Emulated camera factory client has been disconnected from the service.
 */
static void
_factory_client_close(void*  opaque)
{
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
    char*   name;

    /* Input channel to use to connect to the camera. */
    int     inp_channel;

    /* Extra parameters passed to the client. */
    char*   remaining_param;
};

/* Frees emulated camera client descriptor.
 */
static void
_camera_client_free(CameraClient* cc)
{
    if (cc->remaining_param != NULL) {
        free(cc->remaining_param);
    }
    if (cc->name != NULL) {
        free(cc->name);
    }

    AFREE(cc);
}

/* Parses emulated camera client parameters.
 * Param:
 *  param - Parameters to parse. This string contains multiple parameters,
 *      separated by a ':' character. The format of the parameters string is as
 *      follows:
 *          <device name>[:<input channel #>][:<extra param>],
 *      where 'device name' is a required parameter defining name of the camera
 *      device, 'input channel' is an optional parameter (positive integer),
 *      defining input channel to use on the camera device. Format of the
 *      extra parameters is not defined at this point.
 *  device_name - Upon success contains camera device name. The caller is
 *      responsible for freeing string buffer returned here.
 *  inp_channel - Upon success contains the input channel to use when connecting
 *      to the device. If this parameter is missing, a 0 will be returned here.
 *  remainder - Contains copy of the string containing remander of the parameters
 *      following device name and input channel. If there are no remainding
 *      parameters, a NULL will be returned here. The caller is responsible for
 *      freeing string buffer returned here.
 * Return:
 *  0 on success, or !0 on failure.
 */
static int
_parse_camera_client_param(const char* param,
                           char** device_name,
                           int* inp_channel,
                           char** remainder)
{
    const char* wrk = param;
    const char* sep;

    *device_name = *remainder = NULL;
    *inp_channel = 0;

    /* Sanity check. */
    if (param == NULL || *param == '\0') {
        E("%s: Parameters must contain device name", __FUNCTION__);
        return -1;
    }

    /* Must contain device name. */
    sep = strchr(wrk, ':');
    if (sep == NULL) {
        /* Contains only device name. */
        *device_name = ASTRDUP(param);
        return 0;
    }

    /* Save device name. */
    *device_name = (char*)malloc((sep - wrk) + 1);
    if (*device_name == NULL) {
        derror("%s: Not enough memory", __FUNCTION__);
        return -1;
    }
    memcpy(*device_name, wrk, sep - wrk);
    (*device_name)[sep - wrk] = '\0';

    /* Move on to the the input channel. */
    wrk = sep + 1;
    if (*wrk == '\0') {
        return 0;
    }
    sep = strchr(wrk, ':');
    if (sep == NULL) {
        sep = wrk + strlen(wrk);
    }
    errno = 0;  // strtol doesn't set it on success.
    *inp_channel = strtol(wrk, (char**)&sep, 10);
    if (errno != 0) {
        E("%s: Parameters %s contain invalid input channel",
          __FUNCTION__, param);
        free(*device_name);
        *device_name = NULL;
        return -1;
    }
    if (*sep == '\0') {
        return 0;
    }

    /* Move on to the the remaining string. */
    wrk = sep + 1;
    if (*wrk == '\0') {
        return 0;
    }
    *remainder = ASTRDUP(wrk);

    return 0;
}

/* Creates descriptor for a connecting emulated camera client.
 * Param:
 *  csd - Camera service descriptor.
 *  param - Client parameters. Must be formatted as follows:
 *      - Multiple parameters are separated by ':'
 *      - Must begin with camera device name
 *      - Can follow with an optional input channel number, wich must be an
 *        integer value
 * Return:
 *  Emulated camera client descriptor on success, or NULL on failure.
 */
static CameraClient*
_camera_client_create(CameraServiceDesc* csd, const char* param)
{
    CameraClient* cc;
    int res;

    ANEW0(cc);

    /* Parse parameters, and save them to the client. */
    res = _parse_camera_client_param(param, &cc->name, &cc->inp_channel,
                                     &cc->remaining_param);
    if (res) {
        _camera_client_free(cc);
        return NULL;
    }

    D("Camera client created: name=%s, inp_channel=%d",
      cc->name, cc->inp_channel);
    return cc;
}

/* Handles a message received from the emulated camera client.
 */
static void
_camera_client_recv(void*         opaque,
                    uint8_t*      msg,
                    int           msglen,
                    QemudClient*  client)
{
    CameraClient* cc = (CameraClient*)opaque;

    // TODO: implement!
}

/* Emulated camera client has been disconnected from the service.
 */
static void
_camera_client_close(void*  opaque)
{
    CameraClient* cc = (CameraClient*)opaque;

    D("Camera client closed: name=%s, inp_channel=%d",
      cc->name, cc->inp_channel);

    _camera_client_free(cc);
}

/********************************************************************************
 * Camera service API
 *******************************************************************************/

/* Connects a client to the camera service.
 * There are two classes of the client that can connect to the service:
 *  - Camera factory that is insterested only in listing camera devices attached
 *    to the host.
 *  - Camera device emulators that attach to the actual camera devices.
 * The distinction between these two classes is made by looking at extra
 * parameters passed in client_param variable. If it's NULL, or empty, the client
 * connects to a camera factory. Otherwise, parameters describe the camera device
 * the client wants to connect to.
 */
static QemudClient*
_camera_service_connect(void*          opaque,
                        QemudService*  serv,
                        int            channel,
                        const char*    client_param)
{
    QemudClient*  client = NULL;
    CameraServiceDesc* csd = (CameraServiceDesc*)opaque;

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
                                      NULL, NULL);
        }
    }

    return client;
}

void
android_camera_service_init(void)
{
    static int _inited = 0;

    if (!_inited) {
        _csDesc_init(&_camera_service_desc);
        QemudService*  serv = qemud_service_register( SERVICE_NAME, 0,
                                                      &_camera_service_desc,
                                                      _camera_service_connect,
                                                      NULL, NULL);
        if (serv == NULL) {
            derror("could not register '%s' service", SERVICE_NAME);
            return;
        }
        D("registered '%s' qemud service", SERVICE_NAME);
    }
}

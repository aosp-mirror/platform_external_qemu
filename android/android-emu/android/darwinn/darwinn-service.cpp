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
* Contains emulated darwinn service implementation.
*/

#include "android/darwinn/darwinn-service.h"

#include "android/emulation/android_qemud.h"
#include "android/featurecontrol/feature_control.h"
#include "android/globals.h"  /* for android_hw */
#include "android/hw-sensors.h"
#include "android/boot-properties.h"
#include "android/utils/debug.h"
#include "android/utils/looper.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#include "third_party/darwinn/api/driver.h"

#include <stdio.h>

#define SERVICE_NAME "darwinn"
#define MAX_QUERY_MESSAGE_SIZE 8092
#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(camera,__VA_ARGS__)

static platforms::darwinn::api::Driver* _darwinn_driver = nullptr;

typedef struct DarwinnClient DarwinnClient;
struct DarwinnClient {
  uint8_t command_buffer[MAX_QUERY_MESSAGE_SIZE];
  int command_buffer_offset;
};

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
  snprintf(payload_size_str, sizeof(payload_size_str), "%08zx", payload_size);
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

/* Handles a message received from the emulated darwinn client.
 * Queries received here are represented as strings:
 * - 'connect' - Connects to the darwinn device (opens it).
 * - 'disconnect' - Disconnexts from the darwinn device (closes it).
 * - 'submit' - Submit the executable reference.
 * - 'create' - Create a submit request.
 * Param:
 *  opaque - Darwinn service descriptor.
 *  msg, msglen - Message received from the camera factory client.
 *  client - Darwinn client pipe.
 */
static void
_darwinn_client_recv(void*         opaque,
                     uint8_t*      msg,
                     int           msglen,
                     QemudClient*  client)
{
  /*
   * Emulated camera client queries.
   */
  DarwinnClient* dc = (DarwinnClient*)opaque;

  if (msglen <= 0) {
    D("%s: query message has invalid length %d", __func__, msglen);
    return;
  }

  if (dc->command_buffer_offset + msglen >= MAX_QUERY_MESSAGE_SIZE) {
    E("%s: command buffer overflowed: existing buffer-size %d needed %d\n",
      MAX_QUERY_MESSAGE_SIZE, dc->command_buffer_offset + msglen);
    dc->command_buffer_offset = 0;
    _qemu_client_reply_ko(client, "query too long");
    return;
  }
  memcpy(dc->command_buffer + dc->command_buffer_offset, msg, msglen);
  dc->command_buffer_offset += msglen;

  if (dc->command_buffer[dc->command_buffer_offset - 1] != '\0') {
    D("%s: imcomplete query", __func__);
    /* This is a partial command, and we expect _camera_client_recv to be invoked again
       with the next segment of the command */
    return;
  }
  msg = (uint8_t*)(dc->command_buffer);
  dc->command_buffer_offset = 0;
  /* E("%s: query is '%s'\n", __func__, msg); */

  /* Register an executable that has been serialized into a string. */
  static const char _query_register[] = "register";
  /* Submit a request for execution. */
  static const char _query_submit[]   = "submit";
  /* Open the driver. */
  static const char _query_open[]     = "open";
  /* Close the driver. */
  static const char _query_close[]    = "close";

  char query_name[64];
  const char* query_param = NULL;

  /* Dispatch the query to an appropriate handler. */
  if (!memcmp(msg, _query_open, strlen(_query_open)) &&
      msg[strlen(_query_open)] == ' ') {
    D("Open driver is called");
  } else if (!memcmp(msg, _query_close, strlen(_query_close)) &&
      msg[strlen(_query_close)] == ' ') {
    D("Close driver is called");
  } else if (!memcmp(msg, _query_register, strlen(_query_register)) &&
      msg[strlen(_query_register)] == ' ') {
    D("Register executable is called");
  } else if (!memcmp(msg, _query_submit, strlen(_query_submit)) &&
      msg[strlen(_query_submit)] == ' ') {
    D("Submit request is called");
  } else {
    E("%s: Unknown query '%s'", __FUNCTION__, (char*)msg);
    _qemu_client_reply_ko(client, "Unknown query");
  }
}

/* Creates descriptor for a connecting emulated darwinn client.
 * Param:
 *  param - Client parameters. Must be formatted as described in comments to
 *      get_token_value routine, and must contain at least 'name' parameter,
 *      identifiying the camera device to create the service for. Also parameters
 *      may contain a decimal 'inp_channel' parameter, selecting the input
 *      channel to use when communicating with the camera device.
 * Return:
 *  Emulated camera client descriptor on success, or NULL on failure.
 */
static DarwinnClient*
_darwinn_client_create()
{
  DarwinnClient* dc;
  ANEW0(dc);

  return dc;
}

static void _darwinn_client_free(DarwinnClient* dc)
{
  AFREE(dc);
}

/* Emulated camera client has been disconnected from the service. */
static void
_darwinn_client_close(void* opaque)
{
  DarwinnClient* dc = (DarwinnClient*)opaque;

  _darwinn_client_free(dc);
}

static QemudClient*
_darwinn_service_connect(void*          opaque,
                         QemudService*  serv,
                         int            channel,
                         const char*    client_param)
{
  QemudClient*  client = NULL;

  D("%s: Connecting darwinn client", __FUNCTION__);

  /* This is an emulated camera client. */
  DarwinnClient* dc = _darwinn_client_create();
  if (dc != NULL) {
    client = qemud_client_new(serv, channel, client_param, dc,
                              _darwinn_client_recv, _darwinn_client_close,
                              NULL, NULL);
  }

  return client;
}

void android_darwinn_service_init(void) {
  static int _inited = 0;

  if (!_inited) {
    QemudService*  serv = qemud_service_register( SERVICE_NAME, 0, NULL,
                                                  _darwinn_service_connect,
                                                  NULL,
                                                  NULL);
    if (serv == NULL) {
      derror("%s: Could not register '%s' service",
             __FUNCTION__, SERVICE_NAME);
      return;
    }

//    if (strcmp(android_hw->hw_camera_back, "emulated") &&
//        strcmp(android_hw->hw_camera_front, "emulated")) {
//      /* Fake camera is not used for camera emulation. */
//      boot_property_add("qemu.sf.fake_camera", "none");
//    } else {
//      if(!strcmp(android_hw->hw_camera_back, "emulated") &&
//          !strcmp(android_hw->hw_camera_front, "emulated")) {
//        /* Fake camera is used for both, front and back camera emulation. */
//        boot_property_add("qemu.sf.fake_camera", "both");
//      } else if (!strcmp(android_hw->hw_camera_back, "emulated")) {
//        boot_property_add("qemu.sf.fake_camera", "back");
//      } else {
//        boot_property_add("qemu.sf.fake_camera", "front");
//      }
//    }

    D("%s: Registered '%s' qemud service", __FUNCTION__, SERVICE_NAME);
  }
}

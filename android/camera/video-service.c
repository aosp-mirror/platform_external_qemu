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
 * Contains emulated video service impementation.
 */

#include "android/utils/system.h"
#include "android/utils/debug.h"
#include "android/camera/service-common.h"
#include "android/camera/video-service.h"

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

#define SERVICE_NAME "video"

/* Video service descriptor. */
typedef struct VideoServiceDesc {
  // Some data;
}VideoServiceDesc;

/* One and only one video service. */
static VideoServiceDesc _video_service_desc;

static void
_video_client_close(void* opaque)
{
}

static void
_factory_client_close(void* opaque)
{
}

static void
_video_client_query_connect(VideoInfo* vc, QemudClient* qc, const char* param)
{
}
static void
_video_client_query_disconnect(VideoInfo* vc, QemudClient* qc, const char* param)
{
}
static void
_video_client_query_start(VideoInfo* vc, QemudClient* qc, const char* param)
{
}
static void
_video_client_query_stop(VideoInfo* vc, QemudClient* qc, const char* param)
{
}
static void
_video_client_query_frame(VideoInfo* vc, QemudClient* qc, const char* param)
{
}

/* Handle message received from the emulated camera factory client. */
static void
_factory_client_recv(void* opaque,
                     uint8_t* msg,
                     int msglen,
                     QemudClient* client)
{
  char* query = (char*)msg;
  char* query_name = malloc(5);
  if (!parse_query(query, query_name, 5, NULL)) {
    //crash E("fatal error, query size too big\n");
    qemu_client_reply_ko(client, "Unknown query name");
    return;
  }
  if (strcmp(query_name,"list")) {
    E("another fatal error\n");
    qemu_client_reply_ko(client, "Unknown query name");
    return;
  }
  //list vids.
  qemu_client_reply_ok(client, "done");

}

/* Handle messsage received from the emulated video client. */
static void
_video_client_recv(void* opaque,
                   uint8_t* msg,
                   int msglen,
                   QemudClient* client)
{
  char* query = (char*)msg;
  static const char query_connect[] = "connect";
  static const char query_disconnect[]="disconnect";
  static const char query_start[]="start";
  static const char query_stop[]="stop";
  static const char query_frame[]="frame";
  char query_name[64];
  const char* query_param = NULL;
  if (parse_query(query, query_name, sizeof(query_name), &query_param)) {
    E("invalid query");
    qemu_client_reply_ko(client, "invalid query");
    return;
  }
  VideoInfo* vc = (VideoInfo*)opaque;
  if (!strcmp(query_name, query_connect)) {
    _video_client_query_connect(vc,client,query_param);
  }else if(!strcmp(query_name, query_disconnect)) {
    _video_client_query_disconnect(vc,client,query_param);
  }else if(!strcmp(query_name, query_start)) {
    _video_client_query_start(vc,client,query_param);
  }else if(!strcmp(query_name, query_stop)) {
    _video_client_query_stop(vc,client,query_param);
  }else if(!strcmp(query_name, query_frame)) {
    _video_client_query_frame(vc,client,query_param);
  } else {
    E("invalid query kameene");
    qemu_client_reply_ko(client, "invalid query kameene");
  }
}

/*******************************************************************************
 * Video service API
 ******************************************************************************/

static QemudClient*
_video_service_connect(void*         opaque,
                       QemudService* serv,
                       int           channel,
                       const char*   client_param)
{
  QemudClient* client = NULL;
  VideoServiceDesc* vsd = (VideoServiceDesc*) opaque;
  if (client_param ==NULL || *client_param=='\0')
    client = qemud_client_new(serv, channel, client_param, vsd,
                              _factory_client_recv, _factory_client_close,
                              NULL, NULL);
  else {
    // Create video client.
    client = qemud_client_new(serv, channel, client_param, vsd,
                              _video_client_recv, _video_client_close,
                              NULL, NULL);
  }
  return client;

}

/*****************
 * Helper routines, if needed.
 *********************************/

void
android_video_service_init(void)
{

  static int _inited = 0;

  if (!_inited) {
    QemudService* serv = qemud_service_register(SERVICE_NAME, 0,
                                                &_video_service_desc,
                                                &_video_service_connect,
                                                NULL, NULL);
    if (serv==NULL) {
      E("%s: Could not register '%s' service", __FUNCTION__, SERVICE_NAME);
      return;
    }
    D("%s: Registered '%s' qemud service", __FUNCTION__, SERVICE_NAME);
  }

}

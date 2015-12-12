#include "android/aeb.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#define  V(...)  VERBOSE_PRINT(init,__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
// BEGIN AEB HACK
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct AEBClient AEBClient ;

typedef struct {
  QemudService* qemu_listen_service;
  AEBClient* aeb_clients;
  int pushing;
} AEBService;

struct AEBClient {
  AEBClient* next;
  AEBService* aeb;
  QemudClient* qemu_client;
};


static AEBService _aebService[1];

static void aebclientrecv(void* opaque, uint8_t* msg, int msglen, QemudClient* client) {
  fprintf(stderr, "got message from guest aebd: %s\n", msg);
}

static void aebclientclose(void* opaque) {
  fprintf(stderr, "aebclientclose: no-op\n");
}

static QemudClient* _aeb_connect(void* opaque, QemudService* service, int channel, const char* client_param) {
  fprintf(stderr, "%s: call\n", __FUNCTION__);
  AEBService* aeb = opaque;
  AEBClient* aebclient = NULL;
  ANEW0(aebclient);
  aebclient->aeb = aeb;
  aebclient->next = aeb->aeb_clients;
  aeb->aeb_clients = aebclient;

  QemudClient* ret = qemud_client_new(service, channel, client_param, aebclient, aebclientrecv, aebclientclose, NULL, NULL);
  qemud_client_set_framing(ret, 1);
  aebclient->qemu_client = ret;
  return ret;
}

static void _aeb_save(Stream* f, QemudService* sv, void* opaque) {
  fprintf(stderr, "aeb save: no-op\n");
}

static int _aeb_load(Stream* f, QemudService* sv, void* opaque) {
  fprintf(stderr, "aeb load: no-op\n");
  return 0;
}

static void _aeb_send() {
  fprintf(stderr, "%s: call\n", __FUNCTION__);
  AEBService* aeb = _aebService;
  AEBClient* aebclient = aeb->aeb_clients;
  if (!aebclient) {
    fprintf(stderr, "%s: aeb client doesnt not exist wtf?\n", __FUNCTION__);
  }

  char buffer[128];
  if (aeb->pushing) {
    snprintf(buffer, sizeof(buffer) - 1, "on:%d", 1);
  } else {
    snprintf(buffer, sizeof(buffer) - 1, "off");
  }

  qemud_client_send(aebclient->qemu_client, (const uint8_t*)buffer, strlen(buffer) + 1);
  fprintf(stderr, "%s: exit\n", __FUNCTION__);
}

void aeb_install() {
  AEBService* aeb = _aebService;
  aeb->pushing = 1;
  _aeb_send();
}

static void aeb_install_finish() {
  AEBService* aeb = _aebService;
  aeb->pushing = 0;
  _aeb_send();
}


void android_aeb_init() {
  AEBService* aeb = _aebService;
  if (aeb->qemu_listen_service == NULL) {
    aeb->qemu_listen_service = qemud_service_register("aeblisten", 0, aeb, _aeb_connect, _aeb_save, _aeb_load);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// END AEB HACK
////////////////////////////////////////////////////////////////////////////////

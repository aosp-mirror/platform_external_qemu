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

#include <sys/time.h>

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

static void _aeb_send(const char* what,uint64_t msglen) {
  AEBService* aeb = _aebService;
  AEBClient* aebclient = aeb->aeb_clients;

  if (!aebclient) {
    fprintf(stderr, "%s: aeb client doesnt not exist!\n", __FUNCTION__);
  }

  qemud_client_send(aebclient->qemu_client, (const uint8_t*)what, msglen);
}

void aeb_install(const char* what) {
  struct timeval tp;
  long int ms;

  gettimeofday(&tp, NULL);
  ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

  fprintf(stderr, "Installing APK through AEB.\n");
  AEBService* aeb = _aebService;
  aeb->pushing = 1;

  int my_size = strlen(what) + 1;
  fprintf(stderr, "Installing %s\n", what);
  char cmd[10] = "         \0";
  snprintf(cmd, sizeof(cmd), "%d", my_size);
  _aeb_send(cmd, 10);
  _aeb_send(what, my_size);

  FILE* f = fopen(what,"r");
  if (f == NULL) { 
    fprintf(stderr, "Cannot read file %s. abort\n", what); return; 
  }

  char* bin = (char*)malloc(sizeof(char) * 100000000);
  int i = 0;
  int c;
  while(1) {
    c = fgetc(f);
    if (c == EOF) break;
    bin[i] = c;
    i += 1;
  }

  int filesize = i;

  gettimeofday(&tp, NULL);
  fprintf(stderr, "Finished reading file (%f MB) from disk. Duration=%ld ms\n", (float)filesize/1048576.0, tp.tv_sec * 1000 + tp.tv_usec / 1000 - ms);
  ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  fprintf(stderr, "Tranferring %d bytes to virtual device.\n", filesize);

  int max_chunk_size = 256000000;

  int j = i > max_chunk_size ? max_chunk_size : i;

  while (i > 0) {

    if (i <= max_chunk_size) { j = i; }

    snprintf(cmd, sizeof(cmd), "%d",j);
    _aeb_send(cmd, 10);
    _aeb_send(bin, j);
    bin += j;
    i -= j;
  }

  // We are done sending. Send "stop" cmd, which is that the next payload is size 0.
  snprintf(cmd, sizeof(cmd), "%d", 0);
  _aeb_send(cmd, 10);

  // Compute transfer rate and speed.
  
  gettimeofday(&tp, NULL);
  int transfer_time = tp.tv_sec * 1000 + tp.tv_usec / 1000 - ms;
  float transfer_rate = (1.0 * filesize) / (1048576.0) / transfer_time * 1000;
  aeb->pushing = 0;
      
  fprintf(stderr, "Finished transferring file to virtual device. Duration: %d ms. Rate: %f MB/s\n", transfer_time, transfer_rate);
}

void android_aeb_init() {
  AEBService* aeb = _aebService;
  if (aeb->qemu_listen_service == NULL) {
    fprintf(stderr, "Registering AEB service.\n");
    aeb->qemu_listen_service = qemud_service_register("aeblisten", 0, aeb, _aeb_connect, _aeb_save, _aeb_load);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// END AEB HACK
////////////////////////////////////////////////////////////////////////////////

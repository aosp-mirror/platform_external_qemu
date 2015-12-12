#include "android/aeb.h"

#include "android/emulation/android_qemud.h"
#include "android/emulation/qemud/android_qemud_client.h"

#include "android/utils/looper.h"
#include "android/emulation/android_pipe.h"

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

  fprintf(stderr, "%s: call client_new. service=0x%lx\n", __FUNCTION__,service);
  QemudClient* ret = qemud_client_new(service, channel, client_param, aebclient, aebclientrecv, aebclientclose, NULL, NULL);
  qemud_client_set_framing(ret, 0);
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


void custom_pipe_send(QemudClient* client, const char* msg,  uint64_t msglen) {
    QemudPipeMessage* buf;
    QemudPipeMessage** ins_at = &client->ProtocolSelector.Pipe.messages;

    /* Allocate descriptor big enough to contain message as well. */
    buf = (QemudPipeMessage*) malloc(msglen + sizeof(QemudPipeMessage));
    if (buf != NULL) {
        /* Message starts right after the descriptor. */
        buf->message = (uint8_t*) buf + sizeof(QemudPipeMessage);
        buf->size = msglen;
        memcpy(buf->message, msg, msglen);
        buf->offset = 0;
        buf->next = NULL;
        while (*ins_at != NULL) {
            ins_at = &(*ins_at)->next;
        }
        *ins_at = buf;
        /* Notify the pipe that there is data to read. */
        fprintf(stderr, "%s: call android_pipe_wake. hwpipe=0x%lx\n", __FUNCTION__, client->ProtocolSelector.Pipe.qemud_pipe->hwpipe);
        android_pipe_wake(client->ProtocolSelector.Pipe.qemud_pipe->hwpipe,
                          PIPE_WAKE_READ);
    }
}

static void _aeb_send(const char* what,uint64_t msglen) {
  fprintf(stderr, "%s: msglen=%d\n", __FUNCTION__, msglen);
  AEBService* aeb = _aebService;
  AEBClient* aebclient = aeb->aeb_clients;

  if (!aebclient) {
    fprintf(stderr, "%s: aeb client doesnt not exist!\n", __FUNCTION__);
  }

  QemudClient* aebqemudclient = aebclient->qemu_client;
  custom_pipe_send(aebqemudclient, what, msglen);
}

#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

char* load_file(const char* fn, int* sz) {
  struct stat sb;
  int fd = open(fn, O_RDONLY);
  fstat(fd, &sb);
  char* res = NULL;
  fprintf(stderr, "%s: Size of %s: %lu\n", __FUNCTION__, fn, sb.st_size);
  *sz = sb.st_size;
  res = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (res == MAP_FAILED) {
    fprintf(stderr, "%s: mmap failed\n", __FUNCTION__);
    return NULL;
  }
  return res;
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
  char cmd[16] = "\0";
  snprintf(cmd, sizeof(cmd), "%x", my_size);
  fprintf(stderr, "First send %s\n", what);
  _aeb_send(cmd, 16);
  fprintf(stderr, "First send finish\n");
  _aeb_send(what, my_size);

  // FILE* f = fopen(what,"r");
  // if (f == NULL) { 
  //   fprintf(stderr, "Cannot read file %s. abort\n", what); return; 
  // }

  int i = 0;
  char* bin = load_file(what, &i);
  if (bin == NULL) {
    fprintf(stderr, "Cannot read file %s. abort\n", what); return; 
  }
  int filesize = i;
  // char* bin = (char*)malloc(sizeof(char) * 100000000);
  // int i = 0;
  // int c;
  // while(1) {
  //   c = fgetc(f);
  //   if (c == EOF) break;
  //   bin[i] = c;
  //   i += 1;
  // }

  // int filesize = i;

  gettimeofday(&tp, NULL);
  fprintf(stderr, "Finished reading file (%f MB) from disk. Duration=%ld ms\n", (float)filesize/1048576.0, tp.tv_sec * 1000 + tp.tv_usec / 1000 - ms);
  ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  fprintf(stderr, "Tranferring %d bytes to virtual device.\n", filesize);

  int max_chunk_size = 256000000;

  int j = i > max_chunk_size ? max_chunk_size : i;

  while (i > 0) {

    if (i <= max_chunk_size) { j = i; }

    snprintf(cmd, sizeof(cmd), "%x",j);
    _aeb_send(cmd, 16);
    _aeb_send(bin, j);
    bin += j;
    i -= j;
  }

  // We are done sending. Send "stop" cmd, which is that the next payload is size 0.
  snprintf(cmd, sizeof(cmd), "%x", 0);
  _aeb_send(cmd, 16);

  // Compute transfer rate and speed.
  
  gettimeofday(&tp, NULL);
  int transfer_time = tp.tv_sec * 1000 + tp.tv_usec / 1000 - ms;
  float transfer_rate = (1.0 * filesize) / (1048576.0) / transfer_time * 1000;
  aeb->pushing = 0;
      
  fprintf(stderr, "Finished transferring file to virtual device. Duration on host: %d ms. Host rate: %f MB/s\n", transfer_time, transfer_rate);
}

static void*
_aebPipe_init(void* hwpipe, void* _looper, const char* args) { return NULL; }

static void
_aebPipe_closeFromGuest(void* opaque) { }

static int
_aebPipe_sendBuffers(void* opaque, const AndroidPipeBuffer* buffers, int numBuffers) { 
  return 0;
}

static int
_aebPipe_recvBuffers(void* opaque, AndroidPipeBuffer* buffers, int numBuffers) {
  return 0;
}

static unsigned
_aebPipe_poll(void* opaque) { return 0; }

static void
_aebPipe_wakeOn(void* opaque, int flags) { }

static void _aebPipe_save(void* opaque, Stream* f) { }

static void* _aebPipe_load(void* hwpipe, void* pipeOpaque, const char* args, Stream* f) { return NULL; }

static const AndroidPipeFuncs _aebPipe_funcs = {
        _aebPipe_init,
        _aebPipe_closeFromGuest,
        _aebPipe_sendBuffers,
        _aebPipe_recvBuffers,
        _aebPipe_poll,
        _aebPipe_wakeOn,
        _aebPipe_save,
        _aebPipe_load,
};


void android_aeb_init() {
  fprintf(stderr, "%s: call\n", __FUNCTION__);
  android_pipe_add_type("android_emulator_bridge", looper_getForThread(), &_aebPipe_funcs);

  AEBService* aeb = _aebService;
  if (aeb->qemu_listen_service == NULL) {
    fprintf(stderr, "Registering AEB service.\n");
    aeb->qemu_listen_service = qemud_service_register("aeblisten", 0, aeb, _aeb_connect, _aeb_save, _aeb_load);
    fprintf(stderr, "AEB service registered.\n");
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// END AEB HACK
////////////////////////////////////////////////////////////////////////////////

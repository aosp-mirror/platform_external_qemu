#include "android/aeb.h"

#include "android/utils/looper.h"
#include "android/emulation/android_pipe.h"

#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  V(...)  VERBOSE_PRINT(init,__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
// BEGIN AEB HACK
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef struct AEBPipeMessage AEBPipeMessage;

struct AEBPipeMessage {
  char* msg;
  uint64_t msglen;
  uint64_t offset;
  AEBPipeMessage* next;
};

typedef struct {
  void* hwpipe;
  AEBPipeMessage* messages;
} AEBPipe;

typedef struct {
  AEBPipe* pipe;
} AEBService;


static AEBService _aeb[1];

static void enqueue_msg(const char* what, uint64_t msglen) {
  AEBPipeMessage** ins_at = &_aeb->pipe->messages;
  AEBPipeMessage* buf = (AEBPipeMessage*)malloc(sizeof(AEBPipeMessage));
  if (buf != NULL) {
    buf->msg = (char*)malloc(msglen);
    buf->msglen = msglen;

    memcpy(buf->msg, what, msglen);

    buf->offset = 0;
    buf->next = NULL;
    while (*ins_at != NULL) {
      ins_at = &(*ins_at)->next;
    }
    *ins_at = buf;
    android_pipe_wake(_aeb->pipe->hwpipe, PIPE_WAKE_READ);
  }
}

static void _aeb_send(const char* what, uint64_t msglen) {
  fprintf(stderr, "%s: msglen=%" PRIu64 "\n", __FUNCTION__, msglen);
  enqueue_msg(what, msglen);
  fprintf(stderr, "%s: woke pipe, exit\n", __FUNCTION__);
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
  fprintf(stderr, "%s: Size of %s: %lld\n", __FUNCTION__, fn, sb.st_size);
  *sz = sb.st_size;
  res = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
  if (res == MAP_FAILED) {
    fprintf(stderr, "%s: mmap failed\n", __FUNCTION__);
    return NULL;
  }
  return res;
}

#include <sys/time.h>

void aeb_install(const char* what) {
  struct timeval tp;
  long int ms;

  gettimeofday(&tp, NULL);
  ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

  fprintf(stderr, "Installing APK through AEB.\n");

  int i = 0;
  char* bin = load_file(what, &i);
  if (bin == NULL) {
    fprintf(stderr, "Cannot read file %s. abort\n", what); return;
  }
  int filesize = i;

  int my_size = strlen(what) + 1;
  fprintf(stderr, "Installing %s\n", what);
  char cmd[16] = "\0";
  snprintf(cmd, sizeof(cmd), "%x", my_size);
  fprintf(stderr, "First send %s\n", what);
  _aeb_send(cmd, 16);
  fprintf(stderr, "First send finish\n");
  _aeb_send(what, my_size);

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
      
  fprintf(stderr, "Finished transferring file to virtual device. Duration on host: %d ms. Host rate: %f MB/s\n", transfer_time, transfer_rate);
}

static void*
_aebPipe_init(void* hwpipe, void* _looper, const char* args) { 
  fprintf(stderr, "%s: call. args=%s\n", __FUNCTION__, args);
  AEBPipe* res = NULL;
  res = (AEBPipe*)malloc(sizeof(AEBPipe));
  res->hwpipe = hwpipe;
  res->messages = NULL;
  _aeb->pipe = res;
  return res;
}

static void
_aebPipe_closeFromGuest(void* opaque) { 
  fprintf(stderr, "%s: call\n", __FUNCTION__);
}

// void aeb_pipe_recv(void* opaque, char* msg, uint64_t msglen) {
//   fprintf(stderr, "%s: call. msglen=%" PRIu64 "\n", __FUNCTION__, msglen);
//   AEBPipe* pipe = (AEBPipe*)(opaque);
//   pipe->msg = msg;
//   pipe->msglen = msglen;
//   fprintf(stderr, "%s: exit\n", __FUNCTION__);
//   pipe->msg = NULL;
//   pipe->msglen = 0;
// }

static int
_aebPipe_sendBuffers(void* opaque, const AndroidPipeBuffer* buffers, int numBuffers) { 
  fprintf(stderr, "%s: call\n", __FUNCTION__);
  // AEBPipe* pipe = (AEBPipe*)(opaque);
  uint64_t transferred = 0;
  char* msg;
  char* wrk;
  int n = 0;

  for (n = 0; n < numBuffers; n++) {
    transferred += buffers[n].size;
  }
  msg = (char*)(malloc(transferred));
  wrk = msg;
  for (n = 0; n < numBuffers; n++) {
    memcpy(wrk, buffers[n].data, buffers[n].size);
    wrk += buffers[n].size;
  }
  fprintf(stderr, "%s: received msg: %s\n", __FUNCTION__, msg);
  return 0;
}

static int
_aebPipe_recvBuffers(void* opaque, AndroidPipeBuffer* buffers, int numBuffers) {
  AEBPipe* pipe = (AEBPipe*)(opaque);
  AndroidPipeBuffer* buff = buffers;
  AndroidPipeBuffer* endbuff = buffers + numBuffers;
  uint64_t sent_bytes = 0;
  uint64_t off_in_buff = 0;

  AEBPipeMessage** msg_list = &pipe->messages;

  if (*msg_list == NULL) {
    return PIPE_ERROR_AGAIN;
  } 

  // This also dequeues sent messages.
  while (buff != endbuff && *msg_list != NULL) {
    AEBPipeMessage* msg = *msg_list;
    uint64_t msg_remaining = msg->msglen - msg->offset;
    uint64_t buff_remaining = buff->size - off_in_buff;
    uint64_t to_copy = msg_remaining > buff_remaining ? buff_remaining : msg_remaining;
    memcpy(buff->data + off_in_buff, msg->msg + msg->offset, to_copy);

    off_in_buff += to_copy;
    msg->offset += to_copy;
    sent_bytes += to_copy;

    if (msg->msglen == msg->offset) {
      *msg_list = msg->next;
      free(msg->msg);
      free(msg);
    }
    if (off_in_buff == buff->size) {
      buff++;
      off_in_buff = 0;
    }
  }

  return sent_bytes;
}

static unsigned
_aebPipe_poll(void* opaque) { 
  fprintf(stderr, "%s: call\n", __FUNCTION__);

  return 0; 
}

static void
_aebPipe_wakeOn(void* opaque, int flags) { 
  AEBPipe* pipe = (AEBPipe*)(opaque);
  if (flags & PIPE_WAKE_READ) {
    if (pipe->messages != NULL) {
      fprintf(stderr, "%s: msgs not null. wakeup\n", __FUNCTION__);
      android_pipe_wake(pipe->hwpipe, PIPE_WAKE_READ);
    }
  }
}

static void _aebPipe_save(void* opaque, Stream* f) { 
  fprintf(stderr, "%s: call\n", __FUNCTION__);
}

static void* _aebPipe_load(void* hwpipe, void* pipeOpaque, const char* args, Stream* f) { 
  fprintf(stderr, "%s: call\n", __FUNCTION__);
  return NULL; 
}

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
  fprintf(stderr, "Registering AEB pipe.\n");
  android_pipe_add_type("android_emulator_bridge", looper_getForThread(), &_aebPipe_funcs);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// END AEB HACK
////////////////////////////////////////////////////////////////////////////////

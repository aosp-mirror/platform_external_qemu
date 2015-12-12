// Copyright 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.#include "android/android_emulator_bridge.h"

#include "android/android_emulator_bridge.h"

#include "android/emulation/android_pipe.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/looper.h"
#include "android/utils/misc.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#define D(...) fprintf(stderr, __VA_ARGS__)
#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define V(...) VERBOSE_PRINT(aeb, __VA_ARGS__)

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

typedef struct { AEBPipe* pipe; } AEBService;

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
    D("%s: msglen=%" PRIu64 "\n", __FUNCTION__, msglen);
    enqueue_msg(what, msglen);
    D("%s: woke pipe, exit\n", __FUNCTION__);
}

char* load_file(const char* fn, int* sz) {
    struct stat sb;
    int fd = open(fn, O_RDONLY);

    if (fd < 0) {
        D("%s: failed to open file: %s\n", __FUNCTION__, fn);
        return NULL;
    }

    fstat(fd, &sb);
    char* res = NULL;
    D("%s: Size of %s: %lld\n", __FUNCTION__, fn, sb.st_size);
    *sz = sb.st_size;
    res = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (res == MAP_FAILED) {
        D("%s: mmap failed\n", __FUNCTION__);
        return NULL;
    }
    return res;
}

void android_emulator_bridge_install(const char* what) {
    struct timeval tp;
    long int ms;

    gettimeofday(&tp, NULL);
    ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;

    D("Installing APK through AEB.\n");

    int i = 0;
    char* bin = load_file(what, &i);
    if (bin == NULL) {
        D("Cannot read file %s. abort\n", what);
        return;
    }
    int filesize = i;

    int my_size = strlen(what) + 1;
    D("Installing %s\n", what);
    char cmd[16] = {};
    snprintf(cmd, sizeof(cmd), "%x", my_size);
    D("First send %s\n", what);
    _aeb_send(cmd, 16);
    D("First send finish\n");
    _aeb_send(what, my_size);

    gettimeofday(&tp, NULL);
    D("Finished reading file (%f MB) from disk. Duration=%ld ms\n",
      (float)filesize / 1048576.0, tp.tv_sec * 1000 + tp.tv_usec / 1000 - ms);
    ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    D("Tranferring %d bytes to virtual device.\n", filesize);

    int max_chunk_size = 256000000;

    int j = i > max_chunk_size ? max_chunk_size : i;

    while (i > 0) {
        if (i <= max_chunk_size) {
            j = i;
        }

        snprintf(cmd, sizeof(cmd), "%x", j);
        _aeb_send(cmd, 16);
        _aeb_send(bin, j);
        bin += j;
        i -= j;
    }

    // We are done sending. Send "stop" cmd, which is that the next payload is
    // size 0.
    snprintf(cmd, sizeof(cmd), "%x", 0);
    _aeb_send(cmd, 16);

    // Compute transfer rate and speed.

    gettimeofday(&tp, NULL);
    int transfer_time = tp.tv_sec * 1000 + tp.tv_usec / 1000 - ms;
    float transfer_rate = (1.0 * filesize) / (1048576.0) / transfer_time * 1000;

    D("Finished transferring file to virtual device. Duration on host: %d ms. "
      "Host rate: %f MB/s\n",
      transfer_time, transfer_rate);
}

static void* _aebPipe_init(void* hwpipe, void* _looper, const char* args) {
    D("%s: call. args=%s\n", __FUNCTION__, args);
    AEBPipe* res = NULL;
    res = (AEBPipe*)malloc(sizeof(AEBPipe));
    res->hwpipe = hwpipe;
    res->messages = NULL;
    _aeb->pipe = res;
    return res;
}

static void _aebPipe_closeFromGuest(void* opaque) {
    D("%s: call\n", __FUNCTION__);
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

static int _aebPipe_sendBuffers(void* opaque,
                                const AndroidPipeBuffer* buffers,
                                int numBuffers) {
    D("%s: call\n", __FUNCTION__);
    // AEBPipe* pipe = (AEBPipe*)(opaque);
    uint64_t transferred = 0;
    char* msg;
    char* wrk;
    int n = 0;

    if (numBuffers == 1) {
      D("%s: received msg (numBuffers = 1): %s\n", __FUNCTION__, buffers->data);
      transferred = buffers->size;
    }  else {
      for (n = 0; n < numBuffers; n++) {
        transferred += buffers[n].size;
      }

      msg = (char*)(malloc(transferred));
      wrk = msg;
      for (n = 0; n < numBuffers; n++) {
        memcpy(wrk, buffers[n].data, buffers[n].size);
        wrk += buffers[n].size;
      }
      D("%s: received msg: %s\n", __FUNCTION__, msg);
    }
    return transferred;
}

static int _aebPipe_recvBuffers(void* opaque,
                                AndroidPipeBuffer* buffers,
                                int numBuffers) {
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
        uint64_t to_copy =
                msg_remaining > buff_remaining ? buff_remaining : msg_remaining;
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

static unsigned _aebPipe_poll(void* opaque) {
    D("%s: call\n", __FUNCTION__);

    return 0;
}

static void _aebPipe_wakeOn(void* opaque, int flags) {
    AEBPipe* pipe = (AEBPipe*)(opaque);
    if (flags & PIPE_WAKE_READ) {
        if (pipe->messages != NULL) {
            D("%s: msgs not null. wakeup\n", __FUNCTION__);
            android_pipe_wake(pipe->hwpipe, PIPE_WAKE_READ);
        }
    }
}

// TODO: Handle snapshots properly.
static void _aebPipe_save(void* opaque, Stream* f) {
    D("%s: call\n", __FUNCTION__);
}

// TODO: Handle snapshots properly.
static void* _aebPipe_load(void* hwpipe,
                           void* pipeOpaque,
                           const char* args,
                           Stream* f) {
    D("%s: call\n", __FUNCTION__);
    return NULL;
}

static const AndroidPipeFuncs _aebPipe_funcs = {
        _aebPipe_init,        _aebPipe_closeFromGuest, _aebPipe_sendBuffers,
        _aebPipe_recvBuffers, _aebPipe_poll,           _aebPipe_wakeOn,
        _aebPipe_save,        _aebPipe_load,
};

void android_emulator_bridge_init() {
    D("%s: call\n", __FUNCTION__);
    D("Registering AEB pipe.\n");
    android_pipe_add_type("android_emulator_bridge", looper_getForThread(),
                          &_aebPipe_funcs);
}

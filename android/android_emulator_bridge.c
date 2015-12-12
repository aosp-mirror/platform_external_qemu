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
#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/looper.h"
#include "android/utils/misc.h"
#include "android/utils/path.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#define USE_MMAP 0
#else
#define USE_MMAP 1
#endif

#if USE_MMAP
#include <sys/mman.h>
#endif

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

// Host-side emulator bridge state.
typedef enum AEBCmd {
    CMD_IDLE = 0,
    CMD_PUSH = 1,
    CMD_PULL = 2,
    CMD_PULL_TRANSFER = 3,

    // Debugging/Benchmarking
    CMD_PUSH_TIME = 4,
    CMD_PULL_TIME = 5,

    // Other cmds
    CMD_SHUTDOWN = 6

} AEBCmd;

typedef struct {
    // Protocol-level command received from device.
    char* aeb_command;

    // Lower-level state for command implementations
    AEBCmd cmd;

    // For holding transferred data (large)
    int comm_errors;
    int payload_size;
    char* buffer;

    // For processing push/pull: reading from/writing to host disk
    FILE* current_file_handle;
    int current_file_descriptor;
    char* current_file_name;
    char* current_system_command;

    int pull_step;
    uint64_t pulled_so_far;
    uint64_t current_transfer;

    // Tracking stats
    uint64_t current_ms;
    uint64_t host_ms;
    struct timeval tp;

} AEBState;

typedef struct {
    void* hwpipe;
    AEBPipeMessage* messages_to_device;
    AEBState state;
} AEBPipe;

typedef struct { AEBPipe* pipe; } AEBService;

static AEBService _aeb[1];

static void enqueue_msg(const char* what, uint64_t msglen) {
    AEBPipeMessage** ins_at = &_aeb->pipe->messages_to_device;
    AEBPipeMessage* buf = (AEBPipeMessage*)malloc(sizeof(AEBPipeMessage));
    assert(buf != NULL);
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

static void _aeb_send(const char* what, uint64_t msglen) {
    enqueue_msg(what, msglen);
}

static void send_aeb_command(const char* cmd) {
    D("%s: call. cmd=%s\n", __FUNCTION__, cmd);
    snprintf(_aeb->pipe->state.aeb_command, AEB_COMMAND_WIDTH, "%s", cmd);
    _aeb_send(_aeb->pipe->state.aeb_command, AEB_COMMAND_WIDTH);
}

static void send_aeb_payload(const char* msg, uint64_t msglen) {
    char cmd[AEB_NUMCONST_WIDTH] = {};

    // D("%s: call. payload size=%d\n", __FUNCTION__, msglen);
    snprintf(cmd, sizeof(cmd), "%016" PRIx64, msglen);
    _aeb_send(cmd, AEB_NUMCONST_WIDTH);
    _aeb_send(msg, msglen);
}

char* load_file(const char* fn, uint64_t* sz, int* fd) {
#if USE_MMAP
    struct stat sb;
    *fd = open(fn, O_RDONLY);

    if (*fd < 0) {
        D("%s: failed to open file: %s\n", __FUNCTION__, fn);
        return NULL;
    }

    fstat(*fd, &sb);
    char* res = NULL;
    D("%s: Size of %s: %" PRIu64 "\n", __FUNCTION__, fn, (uint64_t)sb.st_size);
    *sz = (uint64_t)(sb.st_size);
    res = mmap(NULL, sb.st_size, PROT_WRITE, MAP_PRIVATE, *fd, 0);
    if (res == MAP_FAILED) {
        D("%s: mmap failed\n", __FUNCTION__);
        close(*fd);
        *fd = -1;
        return NULL;
    }
    return res;
#else
    FILE* fh = fopen(fn, "rb");
    char* res = NULL;

    if (!fh) {
        D("%s: failed to open file: %s\n", __FUNCTION__, fn);
        return NULL;
    }

    fseek(fh, 0, SEEK_END);
    uint64_t fsize = (uint64_t)ftell(fh);
    uint64_t totalsize = fsize;
    fseek(fh, 0, SEEK_SET);

    *sz = totalsize;
    *fd = -1;

    res = malloc(totalsize);
    uint64_t ret = fread(res, totalsize, 1, fh);
    if (!ret) {
        D("%s: error reading file %s\n", __FUNCTION__, fn);
        return NULL;
    }

    fclose(fh);
    return res;
#endif
}

bool is_command(AEBPipe* pipe, const char* cmd) {
    return !strcmp(pipe->state.aeb_command, cmd);
}

// transfer size: bytes
// transfer_ms: time taken to transfer
float transfer_rate(uint64_t transfer_size, uint64_t transfer_ms) {
    return ((float)transfer_size) / ((float)(1024 * 1024)) / transfer_ms *
           1000.0;
}

void android_emulator_bridge_recv(AEBPipe* pipe, char* msg, uint64_t msglen) {
    uint64_t device_time;

    switch (pipe->state.cmd) {
        case CMD_PUSH:
            // If in CMD_PUSH, we should get an acknowledgement from the device
            // for
            // success/failure
            D("%s: call. msg=%s msglen=%" PRIu64 "\n", __FUNCTION__, msg,
              msglen);
            memcpy(pipe->state.aeb_command, msg, AEB_COMMAND_WIDTH);
            D("%s: aeb_command=%s\n", __FUNCTION__, pipe->state.aeb_command);
            if (is_command(pipe, "SUCCESS")) {
                D("%s: Finished transferring file to virtual device.\n",
                  __FUNCTION__);

                gettimeofday(&pipe->state.tp, NULL);
                pipe->state.current_ms = (uint64_t)(
                        pipe->state.tp.tv_sec * 1000 +
                        pipe->state.tp.tv_usec / 1000 - pipe->state.current_ms);
                D("%s: Time measured since beginning of transfer"
                  " (including pipe lag): %" PRIu64
                  " ms."
                  " Time on host: %" PRIu64 " ms\n",
                  __FUNCTION__, pipe->state.current_ms, pipe->state.host_ms);

                D("Transfer rate (end to end + lag in receiving ack): %f MiB/s "
                  "Setting to idle.\n",
                  transfer_rate(pipe->state.current_transfer,
                                pipe->state.current_ms));

                send_aeb_command("CMD_PULL_TIME");
                pipe->state.cmd = CMD_PULL_TIME;
            } else if (is_command(pipe, "ERROR")) {
                D("%s: Error transferring file to virtual device. Abort.\n",
                  __FUNCTION__);
                pipe->state.cmd = CMD_IDLE;
            } else {
                D("%s: Unknown AEB command (or file transfer fail): %s",
                  __FUNCTION__, pipe->state.aeb_command);
            }
            break;
        case CMD_PULL:
            if (pipe->state.pull_step == 0 && !strcmp(msg, "ERROR")) {
                D("%s: Error pulling file. Abort.\n", __FUNCTION__);
                pipe->state.cmd = CMD_IDLE;
                break;
            } else if (pipe->state.pull_step == 0 &&
                       !strcmp(msg, "PULL_SIZE")) {
                D("%s: got PULL_SIZE message\n", __FUNCTION__);
                pipe->state.current_transfer = 0;
                pipe->state.pull_step = 1;
            } else if (pipe->state.pull_step == 1) {
                pipe->state.pull_step = 2;
                sscanf(msg, "%016" PRIx64, &pipe->state.current_transfer);
                D("%s: File being pulled is %" PRIu64 " bytes\n", __FUNCTION__,
                  pipe->state.current_transfer);
                pipe->state.pulled_so_far = 0;
            } else {
                // D("%s: PULLING: write %d bytes\n", __FUNCTION__, msglen);
                fwrite(msg, 1, msglen, pipe->state.current_file_handle);
                pipe->state.pulled_so_far += msglen;
                // D("%s: PULLING: so far: %llu bytes. total: %llu bytes\n",
                // __FUNCTION__, pipe->state.pulled_so_far,
                // pipe->state.current_transfer);
            }

            if (pipe->state.pull_step == 2 &&
                pipe->state.pulled_so_far == pipe->state.current_transfer) {
                D("%s: PULLING: transfer done.\n", __FUNCTION__);
                fclose(pipe->state.current_file_handle);

                gettimeofday(&pipe->state.tp, NULL);
                pipe->state.current_ms = (uint64_t)(
                        pipe->state.tp.tv_sec * 1000 +
                        pipe->state.tp.tv_usec / 1000 - pipe->state.current_ms);

                D("%s: PULLING: Time taken to transfer:"
                  " %" PRIu64 " ms\n",
                  __FUNCTION__, pipe->state.current_ms);
                D("%s: PULLING: Rate = %f MiB/s\n", __FUNCTION__,
                  transfer_rate(pipe->state.current_transfer,
                                pipe->state.current_ms));

                pipe->state.cmd = CMD_IDLE;
                pipe->state.current_transfer = 0;
                pipe->state.pulled_so_far = 0;
            }
            break;
        case CMD_PULL_TIME:
            sscanf(msg, "%" PRIu64, &device_time);
            D("%s: Time taken on device to transfer: %s ms\n", __FUNCTION__,
              msg);
            D("%s: Somewhat overestimated transfer rate (counting device time "
              "only): %f MiB/s\n",
              __FUNCTION__,
              transfer_rate(pipe->state.current_transfer, device_time));

            pipe->state.cmd = CMD_IDLE;
            pipe->state.current_transfer = 0;
            pipe->state.current_ms = 0;
            break;
        case CMD_IDLE:
            D("%s: Currently idle\n", __FUNCTION__);
            D("%s: msg=%s msglen=%" PRIu64 "\n", __FUNCTION__, msg, msglen);
            memcpy(pipe->state.aeb_command, msg, AEB_COMMAND_WIDTH);
            D("%s: aeb_command=%s\n", __FUNCTION__, pipe->state.aeb_command);
            break;
        default:
            D("%s: unknown aeb cmd. pipe->state.cmd = %d\n", __FUNCTION__,
              pipe->state.cmd);
    }
}

void android_emulator_bridge_shutdown() {
    D("%s: call.\n", __FUNCTION__);
    AEBPipe* pipe = _aeb->pipe;
    send_aeb_command("CMD_SHUTDOWN");
    pipe->state.cmd = CMD_SHUTDOWN;
}

void android_emulator_bridge_pull(const char* filename) {
    D("Pulling %s from the device.\n", filename);

    if (strlen(filename) > AEB_COMMAND_WIDTH - 1) {
        D("%s is too long. Abort\n", filename);
        return;
    }

    AEBPipe* pipe = _aeb->pipe;

    char* file_basename = path_basename((char*)filename);

    D("Checking whether ./%s is available on the host.\n", file_basename);
    pipe->state.current_file_handle = fopen(file_basename, "w");
    if (!pipe->state.current_file_handle) {
        D("Cannot open file ./%s on the host. Abort.\n", file_basename);
        return;
    }

    D("Pulling...\n");

    gettimeofday(&pipe->state.tp, NULL);
    pipe->state.current_ms = (uint64_t)(pipe->state.tp.tv_sec * 1000 +
                                        pipe->state.tp.tv_usec / 1000);

    pipe->state.cmd = CMD_PULL;
    pipe->state.pull_step = 0;
    pipe->state.pulled_so_far = 0;
    pipe->state.current_transfer = 0;

    strncpy(pipe->state.current_file_name, file_basename, AEB_COMMAND_WIDTH);
    free(file_basename);

    int filename_length = strlen(filename) + 1;
    send_aeb_command("CMD_PULL");
    send_aeb_payload(filename, filename_length);
}

void android_emulator_bridge_push(const char* filename) {
    D("Pushing %s through AEB\n", filename);

    AEBPipe* pipe = _aeb->pipe;
    gettimeofday(&pipe->state.tp, NULL);
    pipe->state.current_ms = (uint64_t)(pipe->state.tp.tv_sec * 1000 +
                                        pipe->state.tp.tv_usec / 1000);

    uint64_t to_transfer = 0;
    char* bin = load_file(filename, &to_transfer,
                          &pipe->state.current_file_descriptor);
    if (bin == NULL) {
        D("Cannot read file %s. abort\n", filename);
        return;
    }
    uint64_t filesize = to_transfer;

    pipe->state.current_transfer = (uint64_t)filesize;
    D("Transferring %" PRIu64 " bytes to virtual device.\n",
      pipe->state.current_transfer);

    D("Sending \"CMD_PUSH\" to device %s\n", filename);
    send_aeb_command("CMD_PUSH");
    pipe->state.cmd = CMD_PUSH;

    D("Sending filename\n");
    int filename_length = strlen(filename) + 1;
    send_aeb_payload(filename, filename_length);
    D("Sending filename done\n");

    uint64_t max_chunk_size = AEB_PUSH_CHUNK_SIZE;
    uint64_t j = to_transfer > max_chunk_size ? max_chunk_size : to_transfer;

    char* wrk = bin;
    while (to_transfer > 0) {
        if (to_transfer <= max_chunk_size) {
            j = to_transfer;
        }
        send_aeb_payload(wrk, j);
        wrk += j;
        to_transfer -= j;
    }

#if USE_MMAP
    munmap(bin, filesize);
    close(pipe->state.current_file_descriptor);
#else
    free(bin);
#endif

    // We are done sending. Send "stop" cmd, which is that the next payload is
    // size 0.
    send_aeb_payload(NULL, 0);

    // Split for the time take on host to transfer.
    gettimeofday(&pipe->state.tp, NULL);
    pipe->state.host_ms =
            (uint64_t)(pipe->state.tp.tv_sec * 1000 +
                       pipe->state.tp.tv_usec / 1000 - pipe->state.current_ms);
}

void android_emulator_bridge_install(const char* filename) {
    D("Installing %s through AEB\n", filename);
    android_emulator_bridge_push(filename);
    send_aeb_command("CMD_INSTALL");
}

static void* _aebPipe_init(void* hwpipe, void* _looper, const char* args) {
    D("%s: call. args=%s\n", __FUNCTION__, args);
    AEBPipe* res = NULL;
    res = (AEBPipe*)malloc(sizeof(AEBPipe));
    res->hwpipe = hwpipe;
    res->messages_to_device = NULL;

    res->state.aeb_command = malloc(AEB_COMMAND_WIDTH);
    res->state.cmd = CMD_IDLE;

    res->state.comm_errors = 0;
    res->state.payload_size = 0;
    res->state.buffer = NULL;

    res->state.current_file_handle = NULL;
    res->state.current_file_descriptor = -1;
    res->state.current_file_name = malloc(AEB_COMMAND_WIDTH);
    res->state.current_system_command = malloc(AEB_COMMAND_WIDTH);

    res->state.pull_step = 0;
    res->state.pulled_so_far = 0;
    res->state.current_transfer = 0;
    res->state.current_ms = 0;
    res->state.host_ms = 0;

    _aeb->pipe = res;
    return res;
}

static void _aebPipe_closeFromGuest(void* opaque) {
    D("%s: call\n", __FUNCTION__);
}

static int _aebPipe_sendBuffers(void* opaque,
                                const AndroidPipeBuffer* buffers,
                                int numBuffers) {
    // D("%s: call\n", __FUNCTION__);
    AEBPipe* pipe = (AEBPipe*)(opaque);

    uint64_t transferred = 0;
    char* msg;
    char* wrk;
    int n = 0;
    bool need_free = false;

    if (numBuffers == 1) {
        transferred = buffers->size;
        msg = (char*)buffers->data;
    } else {
        for (n = 0; n < numBuffers; n++) {
            transferred += buffers[n].size;
        }

        msg = (char*)(malloc(transferred));
        wrk = msg;
        for (n = 0; n < numBuffers; n++) {
            memcpy(wrk, buffers[n].data, buffers[n].size);
            wrk += buffers[n].size;
        }
        need_free = true;
    }

    android_emulator_bridge_recv(pipe, msg, transferred);

    if (need_free) {
        free(msg);
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

    AEBPipeMessage** msg_list = &pipe->messages_to_device;

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
        if (pipe->messages_to_device != NULL) {
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

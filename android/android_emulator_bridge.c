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

#include "android/emulation/android_pipe_device.h"
#include "android/emulation/android_pipe_host.h"
#include "android/globals.h"
#include "android/utils/assert.h"
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
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

// Sizes of data types communicated between host/guest:

// Size of protocol commands such as SUCCESS | ERROR | CMD_PUSH ...
#define AEB_COMMAND_WIDTH 1024
// Size of numbers
#define AEB_NUMCONST_WIDTH 17
// Size when a nontrivial payload (such as a file transfer) is involved
#define AEB_BUFFER_SIZE (1024 * 1024 * 32)
// Alias for same
#define AEB_PUSH_CHUNK_SIZE AEB_BUFFER_SIZE
// Directory where pushed files are stored
#define AEB_DIR "/data/local/tmp/"

#define DEBUG 1

#if DEBUG
#define D(...) fprintf(stderr, __VA_ARGS__)
#else
#define D(...) 0
#endif

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
    CMD_SHUTDOWN = 6,
    CMD_SCREENCAP = 7,
    CMD_SPEEDTEST = 8,

} AEBCmd;

typedef struct {
    // Protocol-level command received from device.
    char* aeb_command;

    // Lower-level state for command implementations
    AEBCmd cmd;
    bool ready;

    // For holding transferred data (large)
    int comm_errors;
    int payload_size;
    char* buffer;

    // For processing push/pull: reading from/writing to host disk
    FILE* current_file_handle;
    int current_file_descriptor;
    char* current_host_filename;
    char* current_device_filename;
    char* current_system_command;

    int pull_step;
    uint64_t pulled_so_far;
    uint64_t current_transfer;

    // Tracking stats
    uint64_t current_ms;
    uint64_t current_us;
    uint64_t host_ms;
    struct timeval tp;

} AEBState;

typedef struct {
    void* hwpipe;
    AEBPipeMessage* messages_to_device;
    AEBState state;
    void* monitor;  // opaque Monitor
    void (*print_callback)(void*, const char* fmt, ...);
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
    android_pipe_host_signal_wake(_aeb->pipe->hwpipe, PIPE_WAKE_READ);
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

    snprintf(cmd, sizeof(cmd), "%016" PRIx64, msglen);
    _aeb_send(cmd, AEB_NUMCONST_WIDTH);
    _aeb_send(msg, msglen);
}

static char* load_file(const char* fn, uint64_t* sz, int* fd) {
#if USE_MMAP
    struct stat sb = {};
    *fd = HANDLE_EINTR(open(fn, O_RDONLY));

    if (*fd < 0) {
        D("%s: failed to open file: %s\n", __FUNCTION__, fn);
        return NULL;
    }

    int fstat_ret = fstat(*fd, &sb);
    if (fstat_ret == -1) {
        D("%s: failed to fstat file. errno=%d filename=%s\n", __FUNCTION__, errno, fn);
        return NULL;
    }

    char* res = NULL;
    D("%s: Size of %s: %" PRIu64 "\n", __FUNCTION__, fn, (uint64_t)sb.st_size);
    *sz = (uint64_t)(sb.st_size);
    res = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, *fd, 0);
    close(*fd);
    if (res == MAP_FAILED) {
        D("%s: mmap failed\n", __FUNCTION__);
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

    int fseek_ret;
    fseek_ret = fseek(fh, 0, SEEK_END);
    if (fseek_ret == -1) {
        D("%s: failed to fseek to end of file. errno=%d\n", __FUNCTION__, errno);
        fclose(fh);
        return NULL;
    }

    uint64_t fsize = (uint64_t)ftell(fh);
    uint64_t totalsize = fsize;

    fseek_ret = fseek(fh, 0, SEEK_SET);
    if (fseek_ret == -1) {
        D("%s: failed to fseek to beginning of file. errno=%d\n", __FUNCTION__, errno);
        fclose(fh);
        return NULL;
    }

    *sz = totalsize;
    *fd = -1;

    res = malloc(totalsize);
    uint64_t ret = fread(res, totalsize, 1, fh);
    if (!ret) {
        D("%s: error reading file %s\n", __FUNCTION__, fn);
        fclose(fh);
        return NULL;
    }

    fclose(fh);
    return res;
#endif
}

static bool is_command(AEBPipe* pipe, const char* cmd) {
    return !strcmp(pipe->state.aeb_command, cmd);
}

// transfer size: bytes
// transfer_ms: time taken to transfer
static float transfer_rate(uint64_t transfer_size, uint64_t transfer_ms) {
    return (float)((double)transfer_size) / ((double)(1024 * 1024)) / ((double)transfer_ms) *
           1000.0;
}

static void reset_timer() {
    AEBPipe* pipe = _aeb->pipe;
    gettimeofday(&pipe->state.tp, NULL);
    pipe->state.current_ms = (uint64_t)(pipe->state.tp.tv_sec * 1000 +
                                        pipe->state.tp.tv_usec / 1000);
    pipe->state.current_us = pipe->state.tp.tv_usec;
}

static uint64_t get_elapsed_timer_us() {
    struct timeval t;
    AEBPipe* pipe = _aeb->pipe;
    gettimeofday(&t, NULL);
    return t.tv_usec - pipe->state.current_us;
}

static uint64_t get_elapsed_timer_ms() {
    struct timeval t;
    AEBPipe* pipe = _aeb->pipe;

    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 + t.tv_usec / 1000 - pipe->state.current_ms;
}

static uint64_t split_timer() {
    AEBPipe* pipe = _aeb->pipe;
    uint64_t res = get_elapsed_timer_ms();
    pipe->state.current_ms = res;
    return res;
}

void android_emulator_bridge_recv(AEBPipe* pipe, char* msg, uint64_t msglen) {
    uint64_t device_time;

    switch (pipe->state.cmd) {
        case CMD_PUSH:
            // CMD_PUSH: We are expecting just one message
            // that says whether or not the file transfer failed.
            // msglen = AEB_COMMAND_WIDTH
            // msg = SUCCESS | ERROR
            D("%s: call. msg=%s msglen=%" PRIu64 "\n", __FUNCTION__, msg,
              msglen);
            assert(msglen == AEB_COMMAND_WIDTH);
            assert(msg[msglen - 1] == '\0');
            assert(!strcmp(msg, "SUCCESS") || !strcmp(msg, "ERROR"));

            memcpy(pipe->state.aeb_command, msg, AEB_COMMAND_WIDTH);

            D("%s: aeb_command=%s\n", __FUNCTION__, pipe->state.aeb_command);
            if (is_command(pipe, "SUCCESS")) {
                D("%s: Finished transferring file to virtual device.\n",
                  __FUNCTION__);

    // Split for the time take on host to transfer.
    uint64_t done_success_ms = get_elapsed_timer_ms();
    D("%s: DONE -> SUCCESS: %" PRIu64 "ms\n", __FUNCTION__, done_success_ms);
                split_timer();
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
                E("%s: Error transferring file to virtual device. Abort.\n",
                  __FUNCTION__);
                pipe->print_callback(pipe->monitor, "KO:Error pushing file\n");
                pipe->state.cmd = CMD_IDLE;
                pipe->state.ready = true;
            } else {
                E("%s: Unknown AEB command (or file transfer fail): %s",
                  __FUNCTION__, pipe->state.aeb_command);
            }
            break;
        case CMD_PULL:
            // CMD_PULL works with the following initialization stage
            // to get the file size:
            // pull_step = 0 => msg = PULL_SIZE | ERROR, len = AEB_COMMAND_WIDTH
            // pull_step = 1 => msg = %016PRIu64, msglen = AEB_NUMCONST_WIDTH
            // Then arbitrary transfers are allowed to happen:
            // pull_step = 2 => msg = ??? msglen = ??? (prob. <= 4096)
            assert(!(pipe->state.pull_step == 0) ||
                    (msglen == AEB_COMMAND_WIDTH &&
                     (!strcmp(msg, "ERROR") || !strcmp(msg, "PULL_SIZE"))));
            assert(!(pipe->state.pull_step == 1) || msglen == AEB_NUMCONST_WIDTH);

            if (pipe->state.pull_step == 0 && msg && (msglen > 0) &&
                !strcmp(msg, "ERROR")) {
                E("%s: Error pulling file. Abort.\n", __FUNCTION__);
                pipe->print_callback(pipe->monitor, "KO:error pulling file\n");
                D("%s: (called monitor print) Error pulling file. Abort.\n",
                  __FUNCTION__);
                pipe->state.cmd = CMD_IDLE;
                pipe->state.ready = true;
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
                fwrite(msg, 1, msglen, pipe->state.current_file_handle);
                pipe->state.pulled_so_far += msglen;
            }

            if (pipe->state.pull_step == 2 &&
                pipe->state.pulled_so_far == pipe->state.current_transfer) {
                D("%s: PULLING: transfer done.\n", __FUNCTION__);
                fclose(pipe->state.current_file_handle);

                split_timer();

                D("%s: PULLING: Time taken to transfer:"
                  " %" PRIu64 " ms\n",
                  __FUNCTION__, pipe->state.current_ms);
                D("%s: PULLING: Rate = %f MiB/s\n", __FUNCTION__,
                  transfer_rate(pipe->state.current_transfer,
                                pipe->state.current_ms));

                pipe->print_callback(pipe->monitor, "OK\n");
                pipe->state.cmd = CMD_IDLE;
                pipe->state.ready = true;
                pipe->state.current_transfer = 0;
                pipe->state.pulled_so_far = 0;
            }
            break;
        case CMD_PULL_TIME:
            // CMD_PULL_TIME: We expect a message
            // of AEB_NUMCONST_WIDTH
            assert(msglen == AEB_NUMCONST_WIDTH);
            assert(msg[msglen - 1] == '\0');

            sscanf(msg, "%" PRIu64, &device_time);
            split_timer();
            D("%s: Elapsed on host since SUCCESS: %" PRIu64 " ms\n", __FUNCTION__,
                    pipe->state.current_ms);
            D("%s: Time taken on device to transfer: %s ms\n", __FUNCTION__,
              msg);
            D("%s: Somewhat overestimated transfer rate (counting device time "
              "only): %f MiB/s\n",
              __FUNCTION__,
              transfer_rate(pipe->state.current_transfer, device_time));

            pipe->print_callback(pipe->monitor, "OK\n");
            pipe->state.cmd = CMD_IDLE;
            pipe->state.ready = true;
            pipe->state.current_transfer = 0;
            pipe->state.current_ms = 0;
            break;
        case CMD_SCREENCAP:
            // CMD_SCREENCAP: We are just looking for "SUCCESS" or "ERROR"
            assert(msglen == AEB_COMMAND_WIDTH);
            assert(msg[msglen - 1] == '\0');
            assert(!strcmp(msg, "SUCCESS") || !strcmp(msg, "ERROR"));

            D("%s: call. msg=%s msglen=%" PRIu64 "\n", __FUNCTION__, msg,
              msglen);
            memcpy(pipe->state.aeb_command, msg, AEB_COMMAND_WIDTH);
            D("%s: aeb_command=%s\n", __FUNCTION__, pipe->state.aeb_command);
            if (is_command(pipe, "SUCCESS")) {
                D("%s: Finished screencap. Downloading.\n", __FUNCTION__);

                pipe->state.cmd = CMD_IDLE;
                pipe->state.ready = true;
                pipe->state.current_transfer = 0;
                pipe->state.current_ms = 0;
                D("%s: Finished screencap. Downloading. File=%s\n",
                  __FUNCTION__, pipe->state.current_host_filename);
                android_emulator_bridge_pull(
                        pipe->state.current_device_filename,
                        pipe->state.current_host_filename);
            } else {
                E("%s: Something's wrong with the screencap.\n", __FUNCTION__);
                pipe->print_callback(pipe->monitor,
                                     "Something's wrong with the screencap.\n");
                pipe->state.cmd = CMD_IDLE;
                pipe->state.ready = true;
                pipe->state.current_transfer = 0;
                pipe->state.current_ms = 0;
            }
            break;
        case CMD_SPEEDTEST:
            // assert(msglen == AEB_COMMAND_WIDTH);
            assert(msg[msglen - 1] == '\0');
            if (!strcmp(msg, "SUCCESS") || !strcmp(msg, "ERROR")) {
                D("%s: CMD_SPEEDTEST received exit message %s from guest.\n", __FUNCTION__, msg);
                pipe->state.cmd = CMD_IDLE;
                pipe->state.ready = true;
            } else {
                D("received return msg (after guest received all 10), len=%d us elapsed: %d\n", msglen, get_elapsed_timer_us());
                pipe->state.cmd = CMD_SPEEDTEST;
                pipe->state.ready = true;
            }
            break;
        case CMD_IDLE:
            // Currently, we do not expect anything from the device
            // if we are in the idle state.
            assert(false);
            break;
        default:
            E("%s: unknown/invalid aeb cmd. pipe->state.cmd = %d msg=%s msglen=%" PRIu64 "\n",
              __FUNCTION__, pipe->state.cmd, msg, msglen);
            assert(false);
    }
}

#define SPEEDTEST_MAX_PACKET_SIZE 65536

void fake_packet(char* p, uint64_t sz) {
    int i;
    for (i = 0; i < SPEEDTEST_MAX_PACKET_SIZE; i++) {
        if (i < sz - 1) {
            p[i] = 'a';
        } else {
            p[i] = '\0';
        }
    }
}

void fake_send_packet(char* src, char* dst, uint64_t sz) {
    memcpy(dst, src, sz);
}

void send_host_guest_packet(uint64_t sz) {
    char send_buf[SPEEDTEST_MAX_PACKET_SIZE] = {};
    int i = 0;
    for (; i < SPEEDTEST_MAX_PACKET_SIZE; i++) {
        if (i < sz - 1) {
            send_buf[i] = 'a';
        } else {
            send_buf[i] = '\0';
        }
    }
    send_aeb_payload(send_buf, sz);
}

void android_emulator_bridge_speedtest() {
    D("%s: call.\n", __FUNCTION__);
    D("%s: sending 20 packets from host to guest, and then back again.\n", __FUNCTION__);
    D("%s: (10 calls to send_aeb_payload which sends total of 10x 17-byte size packets"
           "and 10x whatever we send here)\n", __FUNCTION__);
    int i;
    uint64_t test_sz = 4;

    AEBPipe* pipe = _aeb->pipe;
    send_aeb_command("CMD_SPEEDTEST");

    D("%s: First, here's an approx upper bound on speed---memcpy on the host side, 2x the 'real' packets\n",
            __FUNCTION__);

    char* fake_host_buf = malloc(SPEEDTEST_MAX_PACKET_SIZE);
    char* fake_guest_buf = malloc(SPEEDTEST_MAX_PACKET_SIZE);

    reset_timer();
    for (i = 0; i < 20; i++) {
        fake_packet(fake_host_buf, test_sz);
        fake_send_packet(fake_host_buf, fake_guest_buf, test_sz);
    }
    D("(fake) us to send all host packets: %d\n", get_elapsed_timer_us());
    // packets received by guest. sending back...
    for (i = 0; i < 20; i++) {
        fake_send_packet(fake_guest_buf, fake_host_buf, test_sz);
        D("(fake) (after sending '10' messages): us to receive this guest packet: %d\n", get_elapsed_timer_us());
    }

    reset_timer();
    for (i = 0; i < 10; i++) {
        send_host_guest_packet(test_sz);
    }
    D("us since sending 10x 64byte packets to guest (no ack):%d\n", get_elapsed_timer_us());
    pipe->state.cmd = CMD_SPEEDTEST;
}

void android_emulator_bridge_screencap(const char* filename) {
    D("%s: call.\n", __FUNCTION__);

    AEBPipe* pipe = _aeb->pipe;

    if (!pipe->state.ready) {
        E("Another command is pending. Abort\n");
        pipe->print_callback(pipe->monitor,
                             "Another command is pending. Abort\n");
        return;
    }

    char* file_basename = path_basename((char*)filename);

    if (strlen(file_basename) + strlen(AEB_DIR) + 1 > AEB_COMMAND_WIDTH) {
        D("Filename '%.5s...' is too long. Abort\n", file_basename);
        pipe->print_callback(pipe->monitor,
                             "Filename '%.5s...' is too long. Abort\n",
                             file_basename);
        return;
    }

    send_aeb_command("CMD_SCREENCAP");
    pipe->state.cmd = CMD_SCREENCAP;

    char device_filename[AEB_COMMAND_WIDTH] = {};
    strcpy(device_filename, AEB_DIR);
    strcat(device_filename, file_basename);

    memcpy(pipe->state.current_host_filename, filename, AEB_COMMAND_WIDTH);
    memcpy(pipe->state.current_device_filename, device_filename, AEB_COMMAND_WIDTH);

    D("%s: screenshot saved on device at %s\n", __FUNCTION__, device_filename);
    D("%s: saved on host at %s\n", __FUNCTION__, filename);
    send_aeb_payload(device_filename, AEB_COMMAND_WIDTH);
}

void android_emulator_bridge_shutdown() {
    D("%s: call.\n", __FUNCTION__);
    AEBPipe* pipe = _aeb->pipe;
    send_aeb_command("CMD_SHUTDOWN");
    pipe->state.cmd = CMD_SHUTDOWN;
}

void android_emulator_bridge_pull(const char* device_filename, const char* host_filename) {
    D("Pulling %s from the device.\n", device_filename);

    AEBPipe* pipe = _aeb->pipe;

    if (!pipe->state.ready) {
        E("Another command is pending. Abort\n");
        pipe->print_callback(pipe->monitor,
                             "Another command is pending. Abort\n");
        return;
    }

    int filename_length = strlen(device_filename) + 1;
    if (filename_length > AEB_COMMAND_WIDTH) {
        E("Filename '%.5s...' is too long. Abort\n", device_filename);
        pipe->print_callback(pipe->monitor,
                             "Filename '%.5s...' is too long. Abort\n",
                             device_filename);
        return;
    }

    D("Checking whether ./%s is available on the host.\n", host_filename);
    pipe->state.current_file_handle = fopen(host_filename, "w");
    if (!pipe->state.current_file_handle) {
        E("Cannot open file ./%s on the host for writing. Abort.\n",
          host_filename);
        pipe->print_callback(
                pipe->monitor,
                "Cannot open file ./%s on the host for writing. Abort.\n",
                host_filename);
        return;
    }

    D("Pulling...\n");

    reset_timer();

    pipe->state.cmd = CMD_PULL;
    pipe->state.ready = false;
    pipe->state.pull_step = 0;
    pipe->state.pulled_so_far = 0;
    pipe->state.current_transfer = 0;

    send_aeb_command("CMD_PULL");
    send_aeb_payload(device_filename, filename_length);

    strncpy(pipe->state.current_device_filename, device_filename, AEB_COMMAND_WIDTH);
    strncpy(pipe->state.current_host_filename, host_filename, AEB_COMMAND_WIDTH);
}

void android_emulator_bridge_push(const char* filename) {
    D("Pushing %s through AEB\n", filename);

    AEBPipe* pipe = _aeb->pipe;

    if (!pipe->state.ready) {
        D("Another command is pending. Abort\n");
        pipe->print_callback(pipe->monitor, "KO:Another command is pending.");
        return;
    }

    reset_timer();
    uint64_t to_transfer = 0;
    char* bin = load_file(filename, &to_transfer,
                          &pipe->state.current_file_descriptor);
    if (bin == NULL) {
        E("Cannot read file %s. Abort\n", filename);
        pipe->print_callback(pipe->monitor, "KO:Cannot read file %s from host",
                             filename);
        return;
    }
    uint64_t filesize = to_transfer;

    pipe->state.current_transfer = filesize;
    D("Transferring %" PRIu64 " bytes to virtual device.\n",
      pipe->state.current_transfer);

    D("Sending \"CMD_PUSH\" to device %s\n", filename);
    send_aeb_command("CMD_PUSH");
    pipe->state.cmd = CMD_PUSH;
    pipe->state.ready = false;

    char* file_basename = path_basename((char*)filename);
    send_aeb_payload(file_basename, strlen(file_basename) + 1);
    D("Filename on device: " AEB_DIR "%s", file_basename);
    free(file_basename);

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
#else
    free(bin);
#endif

    // We are done sending. Send empty payload and follow up with "DONE"
    send_aeb_payload(NULL, 0);
    send_aeb_command("DONE");

    // Split for the time take on host to transfer.
    pipe->state.host_ms = get_elapsed_timer_ms();


    strncpy(pipe->state.current_host_filename, filename, AEB_COMMAND_WIDTH);
}

void android_emulator_bridge_install(const char* filename) {
    D("Installing %s through AEB\n", filename);

    AEBPipe* pipe = _aeb->pipe;

    if (!pipe->state.ready) {
        E("Another command is pending. Abort\n");
        pipe->print_callback(pipe->monitor, "KO:Another command is pending.");
        return;
    }

    android_emulator_bridge_push(filename);
    send_aeb_command("CMD_INSTALL");
}

void null_print_callback(void* monitor, const char* fmt, ...) {
    D("Warning: AEB print callback is not set!\n");
}

static void* _aebPipe_init(void* hwpipe, void* _looper, const char* args) {
    D("%s: call. args=%s\n", __FUNCTION__, args);

    if (_aeb->pipe) {
        D("%s: Warning: AEBPipe already initialized. Reinitializing.\n",
          __FUNCTION__);
    }

    AEBPipe* res = NULL;
    res = (AEBPipe*)malloc(sizeof(AEBPipe));
    res->hwpipe = hwpipe;
    res->messages_to_device = NULL;

    res->monitor = NULL;
    res->print_callback = null_print_callback;

    res->state.aeb_command = malloc(AEB_COMMAND_WIDTH);
    res->state.cmd = CMD_IDLE;
    res->state.ready = true;

    res->state.comm_errors = 0;
    res->state.payload_size = 0;
    res->state.buffer = NULL;

    res->state.current_file_handle = NULL;
    res->state.current_file_descriptor = -1;
    res->state.current_host_filename = malloc(AEB_COMMAND_WIDTH);
    res->state.current_device_filename = malloc(AEB_COMMAND_WIDTH);
    res->state.current_system_command = malloc(AEB_COMMAND_WIDTH);

    res->state.pull_step = 0;
    res->state.pulled_so_far = 0;
    res->state.current_transfer = 0;
    res->state.current_ms = 0;
    res->state.current_us = 0;
    res->state.host_ms = 0;

    _aeb->pipe = res;
    return res;
}

static void _aebPipe_closeFromGuest(void* opaque) {
    D("%s: call\n", __FUNCTION__);
    AEBPipe* pipe = (AEBPipe*)(opaque);
    if (pipe != NULL) {
        AEBPipeMessage** msg_list = &pipe->messages_to_device;
        while (*msg_list != NULL) {
            AEBPipeMessage* to_free = *msg_list;
            *msg_list = to_free->next;
            free(to_free);
        }
    }
}

static int _aebPipe_sendBuffers(void* opaque,
                                const AndroidPipeBuffer* buffers,
                                int numBuffers) {
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
            android_pipe_host_signal_wake(pipe->hwpipe, PIPE_WAKE_READ);
        }
    }
}

// TODO: Handle snapshots properly.
static void _aebPipe_save(void* opaque, Stream* f) {
    D("%s: call (Not yet implemented)\n", __FUNCTION__);
}

// TODO: Handle snapshots properly.
static void* _aebPipe_load(void* hwpipe,
                           void* pipeOpaque,
                           const char* args,
                           Stream* f) {
    D("%s: call. (Not yet implemented)\n", __FUNCTION__);
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
    _aeb->pipe = NULL;
    android_pipe_add_type("android_emulator_bridge", looper_getForThread(),
                          &_aebPipe_funcs);
}

// register_monitor and register_print_callback are used to make the console
// interface print "OK" only when a file transfer has completed.
void android_emulator_bridge_register_monitor(void* mon) {
    _aeb->pipe->monitor = mon;
}

void android_emulator_bridge_register_print_callback(
        void (*monitor_printf)(void*, const char* fmt, ...)) {
    if (!monitor_printf) {
        _aeb->pipe->print_callback = null_print_callback;
    } else {
        _aeb->pipe->print_callback = monitor_printf;
    }
}

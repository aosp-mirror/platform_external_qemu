// Copyright (C) 2018 The Android Open Source Project
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
// limitations under the License.
#include "android/base/files/MemStream.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/address_space_device.h"
#include "android/emulation/address_space_device.hpp"
#include "android/emulation/address_space_graphics.h"
#include "android/emulation/address_space_graphics_types.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/control/vm_operations.h"
#include "android/emulation/control/window_agent.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/opengl/emugl_config.h"
#include "android/opengles-pipe.h"
#include "android/opengles.h"
#include "android/refcount-pipe.h"
#include "android/snapshot/interface.h"

#include <fstream>
#include <string>

#include <stdio.h>

extern "C" {
#include "hw/misc/goldfish_pipe.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
}  // extern "C"


static bool sInitialized = false;
static void init_global_state();

static const GoldfishPipeServiceOps goldfish_pipe_service_ops = {
        // guest_open()
        [](GoldfishHwPipe* hwPipe) -> GoldfishHostPipe* {
            init_global_state();
            return static_cast<GoldfishHostPipe*>(
                    android_pipe_guest_open(hwPipe));
        },
        // guest_open_with_flags()
        [](GoldfishHwPipe* hwPipe, uint32_t flags) -> GoldfishHostPipe* {
            return static_cast<GoldfishHostPipe*>(
                    android_pipe_guest_open_with_flags(hwPipe, flags));
        },
        // guest_close()
        [](GoldfishHostPipe* hostPipe, GoldfishPipeCloseReason reason) {
            static_assert((int)GOLDFISH_PIPE_CLOSE_GRACEFUL ==
                                  (int)PIPE_CLOSE_GRACEFUL,
                          "Invalid PIPE_CLOSE_GRACEFUL value");
            static_assert(
                    (int)GOLDFISH_PIPE_CLOSE_REBOOT == (int)PIPE_CLOSE_REBOOT,
                    "Invalid PIPE_CLOSE_REBOOT value");
            static_assert((int)GOLDFISH_PIPE_CLOSE_LOAD_SNAPSHOT ==
                                  (int)PIPE_CLOSE_LOAD_SNAPSHOT,
                          "Invalid PIPE_CLOSE_LOAD_SNAPSHOT value");
            static_assert(
                    (int)GOLDFISH_PIPE_CLOSE_ERROR == (int)PIPE_CLOSE_ERROR,
                    "Invalid PIPE_CLOSE_ERROR value");

            android_pipe_guest_close(hostPipe,
                                     static_cast<PipeCloseReason>(reason));
        },
        // guest_pre_load()
        [](QEMUFile* file) { (void)file; },
        // guest_post_load()
        [](QEMUFile* file) { (void)file; },
        // guest_pre_save()
        [](QEMUFile* file) { (void)file; },
        // guest_post_save()
        [](QEMUFile* file) { (void)file; },
        // guest_load()
        [](QEMUFile* file,
           GoldfishHwPipe* hwPipe,
           char* force_close) -> GoldfishHostPipe* {
            (void)file;
            (void)hwPipe;
            (void)force_close;
           return nullptr;
        },
        // guest_save()
        [](GoldfishHostPipe* hostPipe, QEMUFile* file) {
            (void)hostPipe;
            (void)file;
        },
        // guest_poll()
        [](GoldfishHostPipe* hostPipe) {
            static_assert((int)GOLDFISH_PIPE_POLL_IN == (int)PIPE_POLL_IN,
                          "invalid POLL_IN values");
            static_assert((int)GOLDFISH_PIPE_POLL_OUT == (int)PIPE_POLL_OUT,
                          "invalid POLL_OUT values");
            static_assert((int)GOLDFISH_PIPE_POLL_HUP == (int)PIPE_POLL_HUP,
                          "invalid POLL_HUP values");

            return static_cast<GoldfishPipePollFlags>(
                    android_pipe_guest_poll(hostPipe));
        },
        // guest_recv()
        [](GoldfishHostPipe* hostPipe,
           GoldfishPipeBuffer* buffers,
           int numBuffers) -> int {
            // NOTE: Assumes that AndroidPipeBuffer and GoldfishPipeBuffer
            //       have exactly the same layout.
            static_assert(
                    sizeof(AndroidPipeBuffer) == sizeof(GoldfishPipeBuffer),
                    "Invalid PipeBuffer sizes");
        // We can't use a static_assert with offsetof() because in msvc, it uses
        // reinterpret_cast.
        // TODO: Add runtime assertion instead?
        // https://developercommunity.visualstudio.com/content/problem/22196/static-assert-cannot-compile-constexprs-method-tha.html
#ifndef _MSC_VER
            static_assert(offsetof(AndroidPipeBuffer, data) ==
                                  offsetof(GoldfishPipeBuffer, data),
                          "Invalid PipeBuffer::data offsets");
            static_assert(offsetof(AndroidPipeBuffer, size) ==
                                  offsetof(GoldfishPipeBuffer, size),
                          "Invalid PipeBuffer::size offsets");
#endif
            return android_pipe_guest_recv(
                    hostPipe, reinterpret_cast<AndroidPipeBuffer*>(buffers),
                    numBuffers);
        },
        // guest_send()
        [](GoldfishHostPipe* hostPipe,
           const GoldfishPipeBuffer* buffers,
           int numBuffers) -> int {
            return android_pipe_guest_send(
                    hostPipe,
                    reinterpret_cast<const AndroidPipeBuffer*>(buffers),
                    numBuffers);
        },
        // guest_wake_on()
        [](GoldfishHostPipe* hostPipe, GoldfishPipeWakeFlags wakeFlags) {
            android_pipe_guest_wake_on(hostPipe, static_cast<int>(wakeFlags));
        },
        // dma_add_buffer()
        [](void* pipe, uint64_t paddr, uint64_t sz) {
            // not considered for virtio
        },
        // dma_remove_buffer()
        [](uint64_t paddr) {
            // not considered for virtio
        },
        // dma_invalidate_host_mappings()
        []() {
            // not considered for virtio
        },
        // dma_reset_host_mappings()
        []() {
            // not considered for virtio
        },
        // dma_save_mappings()
        [](QEMUFile* file) {
            (void)file;
        },
        // dma_load_mappings()
        [](QEMUFile* file) {
            (void)file;
        },
};

// android_pipe_hw_funcs but for virtio-gpu
static const AndroidPipeHwFuncs android_pipe_hw_virtio_funcs = {
        // resetPipe()
        [](void* hwPipe, void* hostPipe) {
            virtio_goldfish_pipe_reset(hwPipe, hostPipe);
        },
        // closeFromHost()
        [](void* hwPipe) {
            fprintf(stderr, "%s: closeFromHost not supported!\n", __func__);
        },
        // signalWake()
        [](void* hwPipe, unsigned flags) {
            fprintf(stderr, "%s: signalWake not supported!\n", __func__);
        },
        // getPipeId()
        [](void* hwPipe) {
            fprintf(stderr, "%s: getPipeId not supported!\n", __func__);
            return 0;
        },
        // lookupPipeById()
        [](int id) -> void* {
            fprintf(stderr, "%s: lookupPipeById not supported!\n", __func__);
            return nullptr;
        }
};

static void init_global_state() {
    android_pipe_set_hw_virtio_funcs(&android_pipe_hw_virtio_funcs);
    return true;
}

extern "C" const GoldfishPipeServiceOps* goldfish_pipe_get_service_ops() {
    return &goldfish_pipe_service_ops;
}

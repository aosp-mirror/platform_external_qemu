/* Copyright 2015 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#include "android-qemu2-glue/emulation/android_pipe_device.h"

#include "android/base/Compiler.h"
#include "android/base/Log.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/android_pipe_common.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/GoldfishDma.h"
#include "android/emulation/VmLock.h"
#include "android/utils/stream.h"
#include "android-qemu2-glue/base/files/QemuFileStream.h"

#include <assert.h>
#include <memory>

extern "C" {
#include "hw/misc/goldfish_pipe.h"
#include "hw/virtio/virtio-goldfish-pipe.h"
}  // extern "C"

// Technical note: This file contains the glue code used between the
// generic AndroidPipe service implementation, and the QEMU2-specific
// goldfish_pipe virtual device. More specifically:
//
// The host service pipe expects a device implementation that will
// implement the callbacks in AndroidHwPipeFuncs. These are injected
// into the service by calling android_pipe_set_hw_funcs().
//
// The virtual device expects a service implementation that will
// implement the callbacks in GoldfishPipeServiceOps. These are injected
// into the device through goldfish_pipe_set_service_ops().
//
// qemu_android_pipe_init() is used to inject all callbacks at setup time
// and initialize the threading mode of AndroidPipe (see below).
//

using android::qemu::QemuFileStream;

static ::Stream* asCStream(android::base::Stream* stream) {
    return reinterpret_cast<::Stream*>(stream);
}

// These callbacks are called from the virtual device into the pipe service.
static const GoldfishPipeServiceOps goldfish_pipe_service_ops = {
        // guest_open()
        [](GoldfishHwPipe* hwPipe) -> GoldfishHostPipe* {
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
        [](QEMUFile* file) {
            QemuFileStream stream(file);
            android_pipe_guest_pre_load(asCStream(&stream));
        },
        // guest_post_load()
        [](QEMUFile* file) {
            QemuFileStream stream(file);
            android_pipe_guest_post_load(asCStream(&stream));
        },
        // guest_pre_save()
        [](QEMUFile* file) {
            QemuFileStream stream(file);
            android_pipe_guest_pre_save(asCStream(&stream));
        },
        // guest_post_save()
        [](QEMUFile* file) {
            QemuFileStream stream(file);
            android_pipe_guest_post_save(asCStream(&stream));
        },
        // guest_load()
        [](QEMUFile* file,
           GoldfishHwPipe* hwPipe,
           char* force_close) -> GoldfishHostPipe* {
            QemuFileStream stream(file);
            return static_cast<GoldfishHostPipe*>(android_pipe_guest_load(
                    asCStream(&stream), hwPipe, force_close));
        },
        // guest_save()
        [](GoldfishHostPipe* hostPipe, QEMUFile* file) {
            QemuFileStream stream(file);
            android_pipe_guest_save(hostPipe, asCStream(&stream));
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
        [](GoldfishHostPipe** hostPipe,
           const GoldfishPipeBuffer* buffers,
           int numBuffers) -> int {
            return android_pipe_guest_send(
                    reinterpret_cast<void**>(hostPipe),
                    reinterpret_cast<const AndroidPipeBuffer*>(buffers),
                    numBuffers);
        },
        // guest_wake_on()
        [](GoldfishHostPipe* hostPipe, GoldfishPipeWakeFlags wakeFlags) {
            android_pipe_guest_wake_on(hostPipe, static_cast<int>(wakeFlags));
        },
        // dma_add_buffer()
        [](void* pipe, uint64_t paddr, uint64_t sz) {
            android_goldfish_dma_ops.add_buffer(pipe, paddr, sz);
        },
        // dma_remove_buffer()
        [](uint64_t paddr) { android_goldfish_dma_ops.remove_buffer(paddr); },
        // dma_invalidate_host_mappings()
        []() { android_goldfish_dma_ops.invalidate_host_mappings(); },
        // dma_reset_host_mappings()
        []() { android_goldfish_dma_ops.reset_host_mappings(); },
        // dma_save_mappings()
        [](QEMUFile* file) {
            QemuFileStream stream(file);
            android_goldfish_dma_ops.save_mappings(&stream);
        },
        // dma_load_mappings()
        [](QEMUFile* file) {
            QemuFileStream stream(file);
            android_goldfish_dma_ops.load_mappings(&stream);
        },
};

// These callbacks are called from the pipe service into the virtual device.
static const AndroidPipeHwFuncs android_pipe_hw_funcs = {
        // closeFromHost()
        [](void* hwPipe) {
            goldfish_pipe_close_from_host(static_cast<GoldfishHwPipe*>(hwPipe));
        },
        // signalWake()
        [](void* hwPipe, unsigned flags) {
            static_assert(
                    (int)GOLDFISH_PIPE_WAKE_CLOSED == (int)PIPE_WAKE_CLOSED,
                    "Invalid PIPE_WAKE_CLOSED values");
            static_assert((int)GOLDFISH_PIPE_WAKE_READ == (int)PIPE_WAKE_READ,
                          "Invalid PIPE_WAKE_READ values");
            static_assert((int)GOLDFISH_PIPE_WAKE_WRITE == (int)PIPE_WAKE_WRITE,
                          "Invalid PIPE_WAKE_WRITE values");
            static_assert((int)GOLDFISH_PIPE_WAKE_UNLOCK_DMA ==
                                  (int)PIPE_WAKE_UNLOCK_DMA,
                          "Invalid PIPE_WAKE_WRITE values");

            goldfish_pipe_signal_wake(
                    static_cast<GoldfishHwPipe*>(hwPipe),
                    static_cast<GoldfishPipeWakeFlags>(flags));
        },
        // getPipeId()
        [](void* hwPipe) {
            return goldfish_pipe_get_id(static_cast<GoldfishHwPipe*>(hwPipe));
        },
        // lookupPipeById()
        [](int id) {
            return (void*)goldfish_pipe_lookup_by_id(id);
        }
};

// android_pipe_hw_funcs but for virtio-gpu
static const AndroidPipeHwFuncs android_pipe_hw_virtio_funcs = {
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

bool qemu_android_pipe_init(android::VmLock* vmLock) {
    goldfish_pipe_set_service_ops(&goldfish_pipe_service_ops);
    android_pipe_set_hw_funcs(&android_pipe_hw_funcs);
    android_pipe_set_hw_virtio_funcs(&android_pipe_hw_virtio_funcs);
    android::AndroidPipe::initThreading(vmLock);
    return true;
}

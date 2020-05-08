// Copyright (C) 2015 The Android Open Source Project
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

#include "android/gpu_frame.h"

#include <atomic>         // for atomic, __a...
#include <cstddef>        // for NULL
#include <unordered_map>  // for unordered_map
#include <utility>        // for pair, move
#include <vector>         // for vector

#include "android/base/Log.h"                             // for LogMessage
#include "android/base/async/CallbackRegistry.h"
#include "android/base/memory/LazyInstance.h"             // for LazyInstance
#include "android/base/synchronization/Lock.h"            // for AutoLock, Lock
#include "android/base/synchronization/MessageChannel.h"  // for MessageChannel
#include "android/emulation/MultiDisplay.h"
#include "android/opengl/GpuFrameBridge.h"                // for GpuFrameBridge
#include "android/opengl/virtio_gpu_ops.h"                // for AndroidVirt...
#include "android/opengles.h"                             // for android_set...


using android::base::AutoLock;
using android::MultiDisplay;

/* set >0 for very verbose debugging */
#define DEBUG 1
#ifndef DEBUG
#define DD(...) (void)0
#else
#define DD(fmt, ...)                                                   \
    fprintf(stderr, "gpu_frame: %s:%d| " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__)
#endif

// Standard values from Khronos.
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401

using android::opengl::GpuFrameBridge;
using android::base::CallbackRegistry;

static GpuFrameBridge* sBridge[MultiDisplay::s_maxNumMultiDisplay];
static std::atomic_bool sRequestPost{false};

static android::base::LazyInstance<CallbackRegistry> sReceiverState =
        LAZY_INSTANCE_INIT;

static void frameReceivedForwader(void* __ignored) {
    sReceiverState->invokeCallbacks();
}

static android::base::Lock sLock[MultiDisplay::s_maxNumMultiDisplay];
// Used to keep track of how many recorders we have active.
// Frame forwarding will stop if this hits 0.
int sRecordCounter[MultiDisplay::s_maxNumMultiDisplay];

// Called from an EmuGL thread to transfer a new frame of the GPU display
// to the main loop.

typedef void (*on_new_gpu_frame_t)(void* opaque,
                                   uint32_t displayId,
                                   int width,
                                   int height,
                                   int ydir,
                                   int format,
                                   int type,
                                   unsigned char* pixels);

// Recording (synchronous):
static void onNewGpuFrame_record(void* opaque,
                                 uint32_t displayId,
                                 int width,
                                 int height,
                                 int ydir,
                                 int format,
                                 int type,
                                 unsigned char* pixels) {
    DCHECK(ydir == -1);
    DCHECK(format == GL_RGBA);
    DCHECK(type == GL_UNSIGNED_BYTE);

    GpuFrameBridge* bridge = reinterpret_cast<GpuFrameBridge*>(opaque);
    bridge->postRecordFrame(width, height, pixels);
}

// Recording (asynchronous):
static void onNewGpuFrame_recordAsync(void* opaque,
                                      uint32_t displayId,
                                      int width,
                                      int height,
                                      int ydir,
                                      int format,
                                      int type,
                                      unsigned char* pixels) {
    DCHECK(ydir == -1);
    DCHECK(format == GL_RGBA);
    DCHECK(type == GL_UNSIGNED_BYTE);

    GpuFrameBridge* bridge = reinterpret_cast<GpuFrameBridge*>(opaque);
    bridge->postRecordFrameAsync(width, height, pixels);
}

static on_new_gpu_frame_t choose_on_new_gpu_frame() {
    if (android_asyncReadbackSupported()) {
        return onNewGpuFrame_recordAsync;
    } else {
        return onNewGpuFrame_record;
    }
}

static void gpu_frame_set_post(bool on, GpuFrameBridge* bridge, uint32_t displayId) {
    CHECK(bridge);

    if (on) {
        android_setPostCallback(choose_on_new_gpu_frame(), bridge,
                                true /* BGRA readback */, displayId);
    } else {
        android_setPostCallback(nullptr, nullptr, true /* BGRA readback */, displayId);
    }
}

void gpu_frame_set_record_mode(bool on, uint32_t displayId) {
    AutoLock lock(sLock[displayId]);
    // Note that we can have multiple recorders active at the same time:
    // 1. The WebRTC module might want to expose the shared region
    // 2. A Java View might want expose the shared region inside android studio
    // 3. The ffmpeg based video recorder might want to receive frames.
    // The updates are atomic operations.
    sRecordCounter[displayId] += on ? 1 : -1;

    // No need to do any additional configuration if we are
    // still recording.
    if (sRecordCounter[displayId] > 1)
        return;
    if (sRecordCounter[displayId] == 1 && !on)
        return;

    if (!sBridge[displayId]) {
        sBridge[displayId] = android::opengl::GpuFrameBridge::create();
        sBridge[displayId]->setDisplayId(displayId);
        //TODO: support frameReceivedForwader for display >= 1
        if (displayId == 0) {
            sBridge[displayId]->setFrameReceiver(frameReceivedForwader, nullptr);
        }
    }

    // We need frames if we have at least one recorder.
    gpu_frame_set_post(sRecordCounter[displayId] > 0, sBridge[displayId], displayId);

    // Need to invalidate the recording buffers in GpuFrameBridge so on the next
    // recording we only read the new data and not data from the previous
    // recording. The buffers will be valid again once new data has been posted.
    if (!on) {
        sBridge[displayId]->invalidateRecordingBuffers();
    }
}

void* gpu_frame_get_record_frame(uint32_t displayId) {
    CHECK(sBridge[displayId]);
    if (android_asyncReadbackSupported()) {
        return sBridge[displayId]->getRecordFrameAsync();
    } else {
        return sBridge[displayId]->getRecordFrame();
    }
}

void gpu_register_shared_memory_callback(FrameAvailableCallback frameAvailable,
                                         void* opaque) {
    sReceiverState->registerCallback(frameAvailable, opaque);
    //TODO: need sync with grpc for multi display, put default 0 so far
    gpu_frame_set_record_mode(true, 0);
    bool expected = false;
    if (sRequestPost.compare_exchange_strong(expected, true)) {
        android_getVirtioGpuOps()->repost();
        sRequestPost.store(false);
    }
}

void gpu_unregister_shared_memory_callback(void* opaque) {
    sReceiverState->unregisterCallback(opaque);
    gpu_frame_set_record_mode(false, 0);
    bool expected = false;
    if (sRequestPost.compare_exchange_strong(expected, true)) {
        android_getVirtioGpuOps()->repost();
        sRequestPost.store(false);
    }
}

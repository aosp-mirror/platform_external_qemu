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

#include "android/opengl/GpuFrameBridge.h"
#include "android/gpu_frame.h"

#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/opengl/GpuFrameBridge.h"
#include "android/opengles.h"

#include <functional>

// Standard values from Khronos.
#define GL_RGBA 0x1908
#define GL_UNSIGNED_BYTE 0x1401

using android::opengl::GpuFrameBridge;

static GpuFrameBridge* sBridge = NULL;
// We need some way to disable the post() if only the recording is using that
// path and it is not in use because glReadPixels will slow down everything.
static bool sIsGuestMode = false;

// Called from an EmuGL thread to transfer a new frame of the GPU display
// to the main loop.

typedef void (*on_new_gpu_frame_t)(void* opaque, int width, int height,
                                   int ydir, int format, int type,
                                   unsigned char* pixels);
// Guest mode:
static void onNewGpuFrame_guest(void* opaque, int width, int height,
                                int ydir, int format, int type,
                                unsigned char* pixels) {
    DCHECK(ydir == -1);
    DCHECK(format == GL_RGBA);
    DCHECK(type == GL_UNSIGNED_BYTE);

    GpuFrameBridge* bridge = reinterpret_cast<GpuFrameBridge*>(opaque);
    bridge->postFrame(width, height, pixels);
}

// Recording (synchronous):
static void onNewGpuFrame_record(void* opaque, int width, int height,
                                int ydir, int format, int type,
                                unsigned char* pixels) {
    DCHECK(ydir == -1);
    DCHECK(format == GL_RGBA);
    DCHECK(type == GL_UNSIGNED_BYTE);

    GpuFrameBridge* bridge = reinterpret_cast<GpuFrameBridge*>(opaque);
    bridge->postRecordFrame(width, height, pixels);
}

// Recording (asynchronous):
static void onNewGpuFrame_recordAsync(void* opaque, int width, int height,
                                      int ydir, int format, int type,
                                      unsigned char* pixels) {
    DCHECK(ydir == -1);
    DCHECK(format == GL_RGBA);
    DCHECK(type == GL_UNSIGNED_BYTE);

    GpuFrameBridge* bridge = reinterpret_cast<GpuFrameBridge*>(opaque);
    bridge->postRecordFrameAsync(width, height, pixels);
}

static on_new_gpu_frame_t choose_on_new_gpu_frame() {
    if (sIsGuestMode) {
        return onNewGpuFrame_guest;
    } else {
        if (android_asyncReadbackSupported()) {
            return onNewGpuFrame_recordAsync;
        } else {
            return onNewGpuFrame_record;
        }
    }
}

static void gpu_frame_set_post(bool on) {
    CHECK(sBridge);

    if (on) {
        android_setPostCallback(choose_on_new_gpu_frame(), sBridge);
    } else {
        android_setPostCallback(nullptr, nullptr);
    }
}

void gpu_frame_set_post_callback(
        Looper* looper,
        void* context,
        on_post_callback_t callback) {
    DCHECK(!sBridge);

    sBridge = android::opengl::GpuFrameBridge::create(
            reinterpret_cast<android::base::Looper*>(looper),
            callback,
            context);
    CHECK(sBridge);

    android_setPostCallback(choose_on_new_gpu_frame(), sBridge);
    sIsGuestMode = true;
}

bool gpu_frame_set_record_mode(bool on) {
    // Assumption: gpu_frame_set_post_callback() is called before this one, so
    // we can determine if we are in host mode based on if sBridge is set.
    if (sIsGuestMode) {
        return false;
    }

    if (!sBridge) {
        sBridge = android::opengl::GpuFrameBridge::create(nullptr, nullptr, nullptr);
    }
    CHECK(sBridge);

    gpu_frame_set_post(on);
    // Need to invalidate the recording buffers in GpuFrameBridge so on the next
    // recording we only read the new data and not data from the previous
    // recording. The buffers will be valid again once new data has been posted.
    if (!on) {
        sBridge->invalidateRecordingBuffers();
    }
    return true;
}

void* gpu_frame_get_record_frame() {
    CHECK(sBridge);
    if (android_asyncReadbackSupported()) {
        return sBridge->getRecordFrameAsync();
    } else {
        return sBridge->getRecordFrame();
    }
}

// Copyright 2018 The Android Open Source Project
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
// limitations under the License.#pragma once
#include "VirtualDisplay.h"

#include "emugl/common/shared_library.h"

#include <unordered_map>

#include <system/window.h>
#include <stdio.h>

void openGrallocLib(const char* libName, GrallocDispatch* dispatch_out) {

    auto lib = emugl::SharedLibrary::open(libName);

    if (!lib) {
        fprintf(stderr, "%s: could not find gralloc library\n", __func__);
        return;
    }

    memset(dispatch_out, 0, sizeof(GrallocDispatch));

    dispatch_out->open_device = (gralloc_open_device_t)lib->findSymbol("grallocOpenDevice");
    dispatch_out->alloc = (gralloc_alloc_t)lib->findSymbol("grallocAlloc");
    dispatch_out->free = (gralloc_free_t)lib->findSymbol("grallocFree");
    dispatch_out->register_buffer = (gralloc_register_t)lib->findSymbol("grallocRegisterBuffer");
    dispatch_out->unregister_buffer = (gralloc_unregister_t)lib->findSymbol("grallocUnregisterBuffer");
    dispatch_out->lock = (gralloc_lock_t)lib->findSymbol("grallocLock");
    dispatch_out->unlock = (gralloc_unlock_t)lib->findSymbol("grallocUnlock");

    if (!dispatch_out->open_device) {
        fprintf(stderr, "%s: could not find grallocOpenDevice\n", __func__);
    }

    if (!dispatch_out->alloc) {
        fprintf(stderr, "%s: could not find grallocAlloc\n", __func__);
    }

    if (!dispatch_out->free) {
        fprintf(stderr, "%s: could not find grallocFree\n", __func__);
    }

    if (!dispatch_out->register_buffer) {
        fprintf(stderr, "%s: could not find grallocRegisterBuffer\n", __func__);
    }

    if (!dispatch_out->unregister_buffer) {
        fprintf(stderr, "%s: could not find grallocUnregisterBuffer\n", __func__);
    }

    if (!dispatch_out->lock) {
        fprintf(stderr, "%s: could not find grallocLock\n", __func__);
    }

    if (!dispatch_out->unlock) {
        fprintf(stderr, "%s: could not find grallocUnlock\n", __func__);
    }
}

class VirtualDisplay {
public:
    VirtualDisplay(int width = 256, int height = 256) : mWidth(width), mHeight(height) {
        openGrallocLib("gralloc_ranchu.dylib", &mGralloc);
        mGralloc.open_device("gpu0");
    }

    ANativeWindowBuffer* allocateWindowBuffer(int width, int height, int format, int usage) {
        buffer_handle_t* handle = new buffer_handle_t;
        int stride;
        mGralloc.alloc(width, height, format, usage, handle, &stride);

        ANativeWindowBuffer* buffer = new ANativeWindowBuffer;

        buffer->width = width;
        buffer->height = height;
        buffer->stride = stride;
        buffer->format = format;
        buffer->usage_deprecated = usage;
        buffer->usage = usage;

        buffer->handle = *handle;

        return buffer;
    }

    int mWidth;
    int mHeight;

    GrallocDispatch mGralloc;
};

// Android native window implementation
static int hook_setSwapInterval(struct ANativeWindow* window, int interval) {
    return 0;
}

static int hook_dequeueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer) {
    return 0;
}

static int hook_lockBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_query(const struct ANativeWindow* window, int what, int* value) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_perform(struct ANativeWindow* window, int operation, ... ) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_dequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer, int* fenceFd) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_queueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static int hook_cancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    fprintf(stderr, "%s:%p call\n", __func__, window);
    return 0;
}

static void hook_incRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p call\n", __func__, common);
}

static void hook_decRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p call\n", __func__, common);
}


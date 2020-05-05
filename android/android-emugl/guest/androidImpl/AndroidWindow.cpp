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
#include "AndroidWindow.h"

#include "AndroidHostCommon.h"

#include "android/base/Backtrace.h"

#include <errno.h>
#include <hardware/gralloc.h>
#include <stdio.h>
#include <cassert>

#define AW_DEBUG 1

#define E(fmt,...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__); \

#if AW_DEBUG
#define D(fmt,...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__); \

#else
#define D(fmt,...)
#endif

namespace aemu {

// Declarations for the ANativeWindow implementation.

static int hook_setSwapInterval(struct ANativeWindow* window, int interval);
static int hook_dequeueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer);
static int hook_lockBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_query(const struct ANativeWindow* window, int what, int* value);
static int hook_perform(struct ANativeWindow* window, int operation, ... );
static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer);
static int hook_dequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer, int* fenceFd);
static int hook_queueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd);
static int hook_cancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd);
static void hook_incRef(struct android_native_base_t* common);
static void hook_decRef(struct android_native_base_t* common);

AndroidWindow::AndroidWindow(int _width, int _height)
    : width(_width), height(_height) {
    // Initialize the ANativeWindow function pointers.
    ANativeWindow::setSwapInterval = hook_setSwapInterval;
    ANativeWindow::dequeueBuffer = hook_dequeueBuffer;
    ANativeWindow::cancelBuffer = hook_cancelBuffer;
    ANativeWindow::queueBuffer = hook_queueBuffer;
    ANativeWindow::query = hook_query;
    ANativeWindow::perform = hook_perform;

    ANativeWindow::dequeueBuffer_DEPRECATED = hook_dequeueBuffer_DEPRECATED;
    ANativeWindow::cancelBuffer_DEPRECATED = hook_cancelBuffer_DEPRECATED;
    ANativeWindow::lockBuffer_DEPRECATED = hook_lockBuffer_DEPRECATED;
    ANativeWindow::queueBuffer_DEPRECATED = hook_queueBuffer_DEPRECATED;

    const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
    const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;

    common.incRef = hook_incRef;
    common.decRef = hook_decRef;
}

void AndroidWindow::setProducer(AndroidBufferQueue* _fromProducer,
                                AndroidBufferQueue* _toProducer) {
    fromProducer = _fromProducer;
    toProducer = _toProducer;
}

int AndroidWindow::dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd) {
    assert(fromProducer);
    AndroidBufferQueue::Item item;
    fromProducer->dequeueBuffer(&item);
    *buffer = item.buffer;
    if (fenceFd)
        *fenceFd = item.fenceFd;
    return 0;
}

int AndroidWindow::queueBuffer(ANativeWindowBuffer* buffer, int fenceFd) {
    assert(toProducer);
    toProducer->queueBuffer({buffer, fenceFd});
    return 0;
}

int AndroidWindow::cancelBuffer(ANativeWindowBuffer* buffer, int fenceFd) {
    assert(toProducer);
    fromProducer->cancelBuffer({buffer, fenceFd});
    return 0;
}

int AndroidWindow::query(int what, int* value) const {
    switch (what) {
        case ANATIVEWINDOW_QUERY_DEFAULT_WIDTH:
        case NATIVE_WINDOW_WIDTH:
            *value = width;
            break;
        case ANATIVEWINDOW_QUERY_DEFAULT_HEIGHT:
        case NATIVE_WINDOW_HEIGHT:
            *value = height;
            break;
        case NATIVE_WINDOW_FORMAT:
            *value = HAL_PIXEL_FORMAT_RGBA_8888;
            break;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            *value = 0;
            break;
        case NATIVE_WINDOW_MAX_BUFFER_COUNT:
            *value = 2;
            break;
        case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
            *value = 2;
            break;

        case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER:
        case NATIVE_WINDOW_CONCRETE_TYPE:
        case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND:
        case NATIVE_WINDOW_BUFFER_AGE:
        case NATIVE_WINDOW_LAST_DEQUEUE_DURATION:
        case NATIVE_WINDOW_LAST_QUEUE_DURATION:
        case NATIVE_WINDOW_FRAME_TIMESTAMPS_SUPPORTS_PRESENT:
        case NATIVE_WINDOW_IS_VALID:
        case NATIVE_WINDOW_DATASPACE:
        default:
            E("Unknown query 0x%x, not implemented.", what);
            return -EINVAL;
    }
    return 0;
}

int AndroidWindow::perform(int operation, va_list args)
{
    int res = 0;
    switch (operation) {
    case NATIVE_WINDOW_CONNECT:
        // deprecated. must return NO_ERROR.
        break;
    case NATIVE_WINDOW_DISCONNECT:
        // deprecated. must return NO_ERROR.
        break;
    case NATIVE_WINDOW_GET_CONSUMER_USAGE64: {
        uint64_t* usage = va_arg(args, uint64_t*);
        *usage =
            GRALLOC_USAGE_HW_TEXTURE |
            GRALLOC_USAGE_HW_RENDER;
        va_end(args);
        break;
    }
    case NATIVE_WINDOW_API_CONNECT:
    case NATIVE_WINDOW_API_DISCONNECT: {
        break;
    }
    case NATIVE_WINDOW_GET_WIDE_COLOR_SUPPORT: {
        bool* outSupport = va_arg(args, bool*);
        *outSupport = false;
        va_end(args);
        break;
    }
    case NATIVE_WINDOW_SET_BUFFER_COUNT: {
        size_t bufferCount = va_arg(args, size_t);
        va_end(args);
        break;
    }
    case NATIVE_WINDOW_SET_AUTO_REFRESH:
    case NATIVE_WINDOW_SET_SHARED_BUFFER_MODE:
    case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
    case NATIVE_WINDOW_SET_BUFFERS_DATASPACE:
    case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
    case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
    case NATIVE_WINDOW_SET_SCALING_MODE:
    case NATIVE_WINDOW_SET_USAGE:
    case NATIVE_WINDOW_SET_USAGE64:
    case NATIVE_WINDOW_GET_REFRESH_CYCLE_DURATION:
    case NATIVE_WINDOW_SET_SURFACE_DAMAGE:
        break;
    case NATIVE_WINDOW_SET_CROP:
    case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
    case NATIVE_WINDOW_SET_BUFFERS_STICKY_TRANSFORM:
    case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
    case NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS:
    case NATIVE_WINDOW_LOCK:
    case NATIVE_WINDOW_UNLOCK_AND_POST:
    case NATIVE_WINDOW_SET_SIDEBAND_STREAM:
    case NATIVE_WINDOW_SET_BUFFERS_SMPTE2086_METADATA:
    case NATIVE_WINDOW_SET_BUFFERS_CTA861_3_METADATA:
    // case NATIVE_WINDOW_SET_BUFFERS_HDR10_PLUS_METADATA:
    case NATIVE_WINDOW_GET_NEXT_FRAME_ID:
    case NATIVE_WINDOW_ENABLE_FRAME_TIMESTAMPS:
    case NATIVE_WINDOW_GET_COMPOSITOR_TIMING:
    case NATIVE_WINDOW_GET_FRAME_TIMESTAMPS:
    case NATIVE_WINDOW_GET_HDR_SUPPORT:

    default:
        E("Unknown perform 0x%x, not implemented.", operation);
        res = -EINVAL;
        break;
    }
    return res;
}

// Android native window implementation
static int hook_setSwapInterval(struct ANativeWindow* window, int interval) {
    AndroidWindow* aw = AndroidWindow::getSelf(window);
    aw->swapInterval = interval;
    return 0;
}

static int hook_dequeueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer) {
    AndroidWindow* aw = AndroidWindow::getSelf(window);
    return aw->dequeueBuffer(buffer, nullptr);
}

static int hook_lockBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    E("Not implemented");
    return 0;
}

static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    E("Not implemented");
    return 0;
}

static int hook_query(const struct ANativeWindow* window, int what, int* value) {
    const AndroidWindow* aw = AndroidWindow::getSelfConst(window);
    return aw->query(what, value);
}

static int hook_perform(struct ANativeWindow* window, int operation, ... ) {
    va_list args;
    va_start(args, operation);
    AndroidWindow* w = AndroidWindow::getSelf(window);
    int result = w->perform(operation, args);
    va_end(args);
    return result;
}

static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    E("Not implemented");
    return 0;
}

static int hook_dequeueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer** buffer, int* fenceFd) {
    AndroidWindow* aw = AndroidWindow::getSelf(window);
    return aw->dequeueBuffer(buffer, fenceFd);
}

static int hook_queueBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    AndroidWindow* aw = AndroidWindow::getSelf(window);
    return aw->queueBuffer(buffer, fenceFd);
}

static int hook_cancelBuffer(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer, int fenceFd) {
    AndroidWindow* aw = AndroidWindow::getSelf(window);
    return aw->cancelBuffer(buffer, fenceFd);
}

static void hook_incRef(struct android_native_base_t* common) {
}

static void hook_decRef(struct android_native_base_t* common) {
}

} // namespace aemu

extern "C" {

EXPORT ANativeWindow* create_host_anativewindow(int width, int height) {
    aemu::AndroidWindow* res =
        new aemu::AndroidWindow(width, height);

    return (ANativeWindow*)res;
}

EXPORT void destroy_host_anativewindow(ANativeWindow* window) {
    delete aemu::AndroidWindow::getSelf(window);
}

} // extern "C"

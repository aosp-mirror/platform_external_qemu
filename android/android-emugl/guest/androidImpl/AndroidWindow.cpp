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

#include <errno.h>
#include <stdio.h>

#define E(fmt,...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__);

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
        default:
            E("Unknown query 0x%x, not implemented", what)
            return -EINVAL;
    }
    return 0;
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
    fprintf(stderr, "%s:%p not implemented\n", __func__, window);
    return 0;
}

static int hook_queueBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p not implemented\n", __func__, window);
    return 0;
}

static int hook_query(const struct ANativeWindow* window, int what, int* value) {
    const AndroidWindow* aw = AndroidWindow::getSelfConst(window);
    return aw->query(what, value);
}

static int hook_perform(struct ANativeWindow* window, int operation, ... ) {
    fprintf(stderr, "%s:%p not implemented\n", __func__, window);
    return 0;
}

static int hook_cancelBuffer_DEPRECATED(struct ANativeWindow* window, struct ANativeWindowBuffer* buffer) {
    fprintf(stderr, "%s:%p not implemented\n", __func__, window);
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
    fprintf(stderr, "%s:%p not implemented\n", __func__, window);
    return 0;
}

static void hook_incRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p not implemented\n", __func__, common);
}

static void hook_decRef(struct android_native_base_t* common) {
    fprintf(stderr, "%s:%p not implemented\n", __func__, common);
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
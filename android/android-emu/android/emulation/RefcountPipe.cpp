// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/RefcountPipe.h"

#include "android/refcount-pipe.h"
#include "android/utils/debug.h"

#include <iostream>
#include <fstream>

static void defaultLastRefFunc(uint32_t handle) {
    fprintf(stderr, "%s: call for 0x%x\n", __func__, handle);
}

static refcount_pipe_on_last_ref_t sLastRefFunc = defaultLastRefFunc;

namespace android {
namespace emulation {

RefcountPipe::RefcountPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) { }

void RefcountPipe::onGuestClose(PipeCloseReason reason) {
    fprintf(stderr, "%s: refs ran out for handle: 0x%x\n", __func__, mHandle);
    sLastRefFunc(mHandle);
}

unsigned RefcountPipe::onGuestPoll() const {
    // Guest can always write
    return PIPE_POLL_OUT;
}

int RefcountPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    // Guest is not supposed to read
    return PIPE_ERROR_IO;
}

int RefcountPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int numBuffers) {

    fprintf(stderr, "%s: call. bufs: %d\n", __func__, numBuffers);

    int result = 0;
    char forRecv[4] = {};

    while (numBuffers > 0) {
        memcpy(forRecv, buffers->data, buffers->size);
        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }

    if (result == 4) {
        memcpy(&mHandle, forRecv, 4);
        fprintf(stderr, "%s: Got hanlde as 0x%x\n", __func__, mHandle);
    }

    return result;
}

void registerRefcountPipeService() {
    fprintf(stderr, "%s: call\n", __func__);
    android::AndroidPipe::Service::add(new RefcountPipe::Service());
}


////////////////////////////////////////////////////////////////////////////////

RefcountPipe::Service::Service() : AndroidPipe::Service("refcount") {}

AndroidPipe* RefcountPipe::Service::create(void* hwPipe, const char* args) {
    return new RefcountPipe(hwPipe, this);
}

AndroidPipe* RefcountPipe::Service::load(void* hwPipe,
                  const char* args,
                  android::base::Stream* stream) {
    return new RefcountPipe(hwPipe, this);
}

bool RefcountPipe::Service::canLoad() const {
    return true;
}

void registerOnLastRefCallback(refcount_pipe_on_last_ref_t func) {
    fprintf(stderr, "%s: call\n", __func__);
    sLastRefFunc = func;
}

}  // namespace emulation
}  // namespace android

void android_init_refcount_pipe(void) {
    android::emulation::registerRefcountPipeService();
}

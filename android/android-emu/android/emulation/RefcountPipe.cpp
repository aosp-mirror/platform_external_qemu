// Copyright 2018 The Android Open Source Project
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

#include <fstream>
#include <iostream>

static refcount_pipe_on_last_ref_t sLastRefFunc = nullptr;

namespace android {
namespace emulation {

RefcountPipe::RefcountPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) {}

void RefcountPipe::onGuestClose(PipeCloseReason reason) {
    if (sLastRefFunc)
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
    }

    return result;
}

void registerRefcountPipeService() {
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
    sLastRefFunc = func;
}

}  // namespace emulation
}  // namespace android

void android_init_refcount_pipe(void) {
    android::emulation::registerRefcountPipeService();
}

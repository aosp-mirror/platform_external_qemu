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

#include "android/base/ArraySize.h"
#include "android/base/memory/LazyInstance.h"
#include "android/refcount-pipe.h"
#include "android/utils/debug.h"

using android::base::arraySize;
using android::base::LazyInstance;

namespace android {
namespace emulation {

static LazyInstance<OnLastColorBufferRef> sOnLastColorBufferRef =
        LAZY_INSTANCE_INIT;

RefcountPipe::RefcountPipe(void* hwPipe, Service* svc, base::Stream* loadStream)
    : AndroidPipe(hwPipe, svc) {
    if (loadStream) {
        mHandle = loadStream->getBe32();
    } else {
        mHandle = 0;
    }
}

RefcountPipe::~RefcountPipe() {
    OnLastColorBufferRef func = sOnLastColorBufferRef.get();
    if (func != nullptr)
        func(mHandle);
}

void RefcountPipe::onGuestClose(PipeCloseReason reason) {
    delete this;
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

    while (numBuffers > 0 && arraySize(forRecv) - result >= buffers->size) {
        memcpy(forRecv + result, buffers->data, buffers->size);
        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }

    if (result == arraySize(forRecv)) {
        memcpy(&mHandle, forRecv, arraySize(forRecv));
    }

    return result;
}

void RefcountPipe::onSave(base::Stream* stream) {
    stream->putBe32(mHandle);
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
    return new RefcountPipe(hwPipe, this, stream);
}

bool RefcountPipe::Service::canLoad() const {
    return true;
}

void registerOnLastRefCallback(OnLastColorBufferRef func) {
    *sOnLastColorBufferRef = func;
}

}  // namespace emulation
}  // namespace android

void android_init_refcount_pipe(void) {
    android::emulation::registerRefcountPipeService();
}

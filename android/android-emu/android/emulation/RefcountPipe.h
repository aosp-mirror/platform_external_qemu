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
#pragma once

#include "android/emulation/AndroidPipe.h"

#include <fstream>

typedef void (*refcount_pipe_on_last_ref_t)(uint32_t handle);

namespace android {
namespace emulation {

// This is a pipe for refcounting guest side objects.

class RefcountPipe final : public AndroidPipe {
public:
    class Service final : public AndroidPipe::Service {
    public:
        Service();
        AndroidPipe* create(void* hwPipe, const char* args) override;
        AndroidPipe* load(void* hwPipe,
                          const char* args,
                          android::base::Stream* stream) override;
        bool canLoad() const override;
    };

    RefcountPipe(void* hwPipe, Service* svc);

    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {}

private:
    uint32_t mHandle;
};

void registerRefcountPipeService();

void registerOnLastRefCallback(refcount_pipe_on_last_ref_t func);

}  // namespace emulation
}  // namespace android

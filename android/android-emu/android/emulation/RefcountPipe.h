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
#include <functional>

namespace android {
namespace emulation {

using OnLastColorBufferRef = std::function<void(uint32_t)>;

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

    RefcountPipe(void* hwPipe,
                 Service* svc,
                 base::Stream* loadStream = nullptr);
    ~RefcountPipe();
    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers,
                    void** newPipePtr) override;
    void onGuestWantWakeOn(int flags) override {}
    void onSave(base::Stream* stream) override;

private:
    uint32_t mHandle;
};

void registerRefcountPipeService();

void registerOnLastRefCallback(OnLastColorBufferRef func);

}  // namespace emulation
}  // namespace android

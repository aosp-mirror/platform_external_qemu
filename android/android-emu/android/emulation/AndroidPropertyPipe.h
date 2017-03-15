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

#pragma once

#include "android/emulation/AndroidPipe.h"

namespace android {
namespace emulation {

// This is a pipe for receiving properties/notifications from the guest.
// There is a background service installed on the guest that will
// forward intent actions + data we are interesting in through this
// pipe.
class AndroidPropertyPipe final : public AndroidPipe {
public:
    class Service final : public AndroidPipe::Service {
    public:
        Service();
        AndroidPipe* create(void* hwPipe, const char* args) override;
    };

    AndroidPropertyPipe(void* hwPipe, Service* svc);
    ~AndroidPropertyPipe();

    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {}

private:
};

void registerAndroidPropertyPipeService();

}  // namespace emulation
}  // namespace android

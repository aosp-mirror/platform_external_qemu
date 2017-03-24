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

// This is a pipe for sending apk to the guest.
// The other side is connected to Android's pm to install apk

class InstallPipe final : public AndroidPipe {
public:

    class Service final : public AndroidPipe::Service {
    public:
        Service();
        AndroidPipe* create(void* hwPipe, const char* args) override;
    };

    InstallPipe(void* hwPipe, Service* svc);

    void onGuestClose(PipeCloseReason reason) override;
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {
    }

private:
    char mPayLoadBuf[32];
    char* mPayLoad = NULL;
    int mPayLoadSize = 0;
    FILE* mFp = NULL;
    std::string mResult;

    bool hasDataForGuest() const;
    int copyData(unsigned char* buf, int buf_size);
    void openFile();


};

void registerInstallPipeService();

}  // namespace emulation
}  // namespace android

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

#include "android/emulation/LogcatPipe.h"

#include "android/logcat-pipe.h"

namespace android {
namespace emulation {


LogcatPipe::LogcatPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) {}

void LogcatPipe::onGuestClose(PipeCloseReason reason) {
}

unsigned LogcatPipe::onGuestPoll() const {
    // Guest can always write
    return PIPE_POLL_OUT;
}

int LogcatPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    // Guest is not supposed to read
    return PIPE_ERROR_IO;
}

int LogcatPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                               int numBuffers) {
    int result = 0;
    while (numBuffers > 0) {
        // BUG: b.android.com/261625
        printf("%s", (char*)buffers->data);
        result += static_cast<int>(buffers->size);
        buffers++;
        numBuffers--;
    }
    return result;
}

void registerLogcatPipeService() {
    android::AndroidPipe::Service::add(new LogcatPipe::Service());
}


////////////////////////////////////////////////////////////////////////////////

LogcatPipe::Service::Service() : AndroidPipe::Service("logcat") {}

AndroidPipe* LogcatPipe::Service::create(void* hwPipe, const char* args) {
    return new LogcatPipe(hwPipe, this);
}

}  // namespace emulation
}  // namespace android

void android_init_logcat_pipe(void) {
    android::emulation::registerLogcatPipeService();
}

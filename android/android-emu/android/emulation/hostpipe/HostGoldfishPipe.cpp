// Copyright (C) 2018 The Android Open Source Project
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
// limitations under the License.

#include "HostGoldfishPipe.h"

#include "android/emulation/testing/TestAndroidPipeDevice.h"

#include "android/base/memory/LazyInstance.h"

using android::base::LazyInstance;

namespace android {

class HostGoldfishPipeDevice {
public:
    HostGoldfishPipeDevice() {
        mGuest = TestAndroidPipeDevice::Guest::create();
    }

    // Also opens.
    void* connect(const char* name) {
        void* pipe = mGuest->open();
        if (mGuest->connect(pipe, name) == 0) { // success
            return pipe;
        } else {
            fprintf(stderr, "%s: failed to connect to host pipe service\n", __func__);
            mGuest->close(pipe);
            return nullptr;
        }
    }

    void close(void* pipe) {
        mGuest->close(pipe);
    }

    ssize_t read(void* pipe, void* buffer, size_t len) {
        return mGuest->read(pipe, buffer, len);
    }

    ssize_t write(void* pipe, const void* buffer, size_t len) {
        return mGuest->write(pipe, buffer, len);
    }

    unsigned poll(void* pipe) {
        return mGuest->poll(pipe);
    }

    ~HostGoldfishPipeDevice() {
        delete mGuest;
    }
private:
    TestAndroidPipeDevice::Guest* mGuest = nullptr;
};

static LazyInstance<HostGoldfishPipeDevice> sDevice = LAZY_INSTANCE_INIT;

HostGoldfishPipeDevice* getHostPipeInstance() {
    return sDevice.ptr();
}

// connect, close, read, write and poll.
void* host_pipe_connect(const char* name) {
    return sDevice->connect(name);
}

void host_pipe_close(void* pipe) {
    sDevice->close(pipe);
}

ssize_t host_pipe_read(void* pipe, void* buffer, size_t len) {
    return sDevice->read(pipe, buffer, len);
}

ssize_t host_pipe_write(void* pipe, const void* buffer, size_t len) {
    return sDevice->write(pipe, buffer, len);
}

unsigned host_pipe_poll(void* pipe) {
    return sDevice->poll(pipe);
}

} // namespace android

extern "C" {

HostPipeDispatch make_host_pipe_dispatch() {
    android::getHostPipeInstance();
    return { android::host_pipe_connect,
             android::host_pipe_close,
             android::host_pipe_read,
             android::host_pipe_write,
             android::host_pipe_poll };
}

}

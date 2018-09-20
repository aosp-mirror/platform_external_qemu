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

#include "android/emulation/android_pipe_device.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/testing/TestVmLock.h"
#include "android/emulation/VmLock.h"

#include "android/base/memory/LazyInstance.h"

#include <unordered_set>
#include <unordered_map>

#include <errno.h>
#include <stdint.h>
#include <string.h>

using android::base::LazyInstance;

namespace android {

static void sResetPipe(void* hwpipe, void* internal_pipe);
static void sCloseFromHost(void* hwpipe);
static void sSignalWake(void* hwpipe, unsigned wakes);

class HostGoldfishPipeDevice {
public:
    HostGoldfishPipeDevice() = default;

    ~HostGoldfishPipeDevice() {
        RecursiveScopedVmLock lock;
        for (auto pipe: mPipes) {
            close(pipe);
        }
        mPipes.clear();
    }

    void initialize() {
        if (mInitialized) return;
        const AndroidPipeHwFuncs funcs = {
            &sResetPipe,
            &sCloseFromHost,
            &sSignalWake,
        };
        android_pipe_set_hw_funcs(&funcs);
        AndroidPipe::Service::resetAll();
        AndroidPipe::initThreading(android::TestVmLock::getInstance());
        mInitialized = true;
    }

    void* open() {
        RecursiveScopedVmLock lock;
        void* res = android_pipe_guest_open(this);
        mPipes.insert(res);
        return res;
    }

    void close(void* pipe) {
        RecursiveScopedVmLock lock;
        if (mPipes.find(pipe) == mPipes.end()) {
            return;
        }
        android_pipe_guest_close(pipe, PIPE_CLOSE_GRACEFUL);
        mPipes.erase(pipe);
    }

    // Also opens.
    void* connect(const char* name) {
        RecursiveScopedVmLock lock;
        void * pipe = open();

        mCurrentPipeWantingConnection = pipe;

        std::string handshake("pipe:");

        handshake += name;

        int len = static_cast<int>(handshake.size()) + 1;
        int ret = write(pipe, handshake.c_str(), len);

        if (ret == len) {
            return getHwSidePipe(pipe);
        }

        return nullptr;
    }

    void resetPipe(void* pipe, void* internal_pipe) {
        RecursiveScopedVmLock lock;
        mResettedPipes[mCurrentPipeWantingConnection] = internal_pipe;
        // mResettedPipes.erase(mCurrentPipeWantingConnection);
    }

    void* getHwSidePipe(void* pipe) {
        RecursiveScopedVmLock lock;
        return mResettedPipes[pipe];
    }

    // Read/write/poll but for a particular pipe.
    ssize_t read(void* pipe, void* buffer, size_t len) {
        RecursiveScopedVmLock lock;
        AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
        return android_pipe_guest_recv(pipe, &buf, 1);
    }

    ssize_t write(void* pipe, const void* buffer, size_t len) {
        RecursiveScopedVmLock lock;
        AndroidPipeBuffer buf = {
                (uint8_t*)buffer, len };
        return android_pipe_guest_send(pipe, &buf, 1);
    }

    unsigned poll(void* pipe) const {
        RecursiveScopedVmLock lock;
        if (mPipes.find(pipe) == mPipes.end()) {
            return PIPE_POLL_HUP;
        }
        return android_pipe_guest_poll(pipe);
    }

    void signalWake(int wakes) {
        RecursiveScopedVmLock lock;
        // TODO
    }

private:
    bool mInitialized = false;

    void* mCurrentPipeWantingConnection = nullptr;

    std::unordered_set<void*> mPipes;
    std::unordered_map<void*, void*> mResettedPipes;
};

static LazyInstance<HostGoldfishPipeDevice> sDevice = LAZY_INSTANCE_INIT;

HostGoldfishPipeDevice* getHostPipeInstance() {
    auto* res = sDevice.ptr();
    res->initialize();
    return res;
}

static void sResetPipe(void* hwpipe, void* internal_pipe) {
    getHostPipeInstance()->resetPipe(hwpipe, internal_pipe);
}

static void sCloseFromHost(void* hwpipe) {
    getHostPipeInstance()->close(hwpipe);
}

static void sSignalWake(void* hwpipe, unsigned wakes) {
    getHostPipeInstance()->signalWake(wakes);
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
    HostPipeDispatch res;
    res.connect = android::host_pipe_connect;
    res.close = android::host_pipe_close;
    res.read = android::host_pipe_read;
    res.write = android::host_pipe_write;
    res.poll = android::host_pipe_poll;
    return res;
}

}

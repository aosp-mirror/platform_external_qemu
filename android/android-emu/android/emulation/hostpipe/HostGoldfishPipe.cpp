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

#include "android/emulation/hostpipe/HostGoldfishPipe.h"
#include "android/emulation/hostpipe/host_goldfish_pipe.h"

#include "android/base/Result.h"
#include "android/crashreport/crash-handler.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/AndroidPipe.h"
#include "android/emulation/testing/TestVmLock.h"
#include "android/emulation/VmLock.h"

#include "android/base/memory/LazyInstance.h"

#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <errno.h>
#include <stdint.h>
#include <string.h>

using android::base::LazyInstance;
using android::base::Result;
using android::base::Ok;
using android::base::Err;

namespace android {

static void sResetPipe(void* hwpipe, void* internal_pipe);
static void sCloseFromHost(void* hwpipe);
static void sSignalWake(void* hwpipe, unsigned wakes);

HostGoldfishPipeDevice::~HostGoldfishPipeDevice() {
    RecursiveScopedVmLock lock;
    for (auto pipe: mPipes) {
        close(pipe);
    }
    mPipes.clear();
}

void HostGoldfishPipeDevice::close(void* pipe) {
    RecursiveScopedVmLock lock;
    if (mPipes.find(pipe) == mPipes.end()) {
        mErrno = ENOENT;
        return;
    } else {
        android_pipe_guest_close(pipe, PIPE_CLOSE_GRACEFUL);
        mPipes.erase(pipe);
    }
}

// Also opens.
void* HostGoldfishPipeDevice::connect(const char* name) {
    RecursiveScopedVmLock lock;
    void * pipe = open();

    mCurrentPipeWantingConnection = pipe;

    std::string handshake("pipe:");

    handshake += name;

    int len = static_cast<int>(handshake.size()) + 1;
    int ret = write(pipe, handshake.c_str(), len);

    if (ret == len) {
        void* res = popHwSidePipe(pipe);
        if (!res) {
            crashhandler_die_format(
                "FATAL: Tried to connect with %s, "
                "but could not get hwpipe.",
                handshake.c_str());
        }
        return res;
    } else {
        LOG(ERROR) << "Could not connect to goldfish pipe with %s\n",
        mResettedPipes.erase(pipe);
        mErrno = EIO;
        return nullptr;
    }
}

void HostGoldfishPipeDevice::resetPipe(void* pipe, void* internal_pipe) {
    RecursiveScopedVmLock lock;
    mResettedPipes[mCurrentPipeWantingConnection] = internal_pipe;
}

// Read/write/poll but for a particular pipe.
ssize_t HostGoldfishPipeDevice::read(void* pipe, void* buffer, size_t len) {
    RecursiveScopedVmLock lock;
    AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
    ssize_t res = android_pipe_guest_recv(pipe, &buf, 1);
    setErrno(res);
    return res;
}

HostGoldfishPipeDevice::ReadResult HostGoldfishPipeDevice::read(void* pipe, size_t maxLength) {
    std::vector<uint8_t> resultBuffer(maxLength);
    void* buffer = (void*)resultBuffer.data();
    ssize_t read_size = read(pipe, buffer, maxLength);

    if (read_size < 0) {
        return Err(mErrno);
    } else {
        if (read_size < maxLength) {
            resultBuffer.resize(read_size);
        }
        return Ok(resultBuffer);
    }
}

ssize_t HostGoldfishPipeDevice::write(void* pipe, const void* buffer, size_t len) {
    RecursiveScopedVmLock lock;
    AndroidPipeBuffer buf = {
            (uint8_t*)buffer, len };
    ssize_t res = android_pipe_guest_send(pipe, &buf, 1);
    setErrno(res);
    return res;
}

HostGoldfishPipeDevice::WriteResult HostGoldfishPipeDevice::write(void* pipe, const std::vector<uint8_t>& data) {
    ssize_t res = write(pipe, data.data(), data.size());

    if (res < 0) {
        return Err(mErrno);
    } else {
        return Ok(res);
    }
}

unsigned HostGoldfishPipeDevice::poll(void* pipe) const {
    RecursiveScopedVmLock lock;
    if (mPipes.find(pipe) == mPipes.end()) {
        return PIPE_POLL_HUP;
    }
    return android_pipe_guest_poll(pipe);
}

void HostGoldfishPipeDevice::signalWake(int wakes) {
    RecursiveScopedVmLock lock;
    // TODO
}

void HostGoldfishPipeDevice::initialize() {
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

void* HostGoldfishPipeDevice::open() {
    RecursiveScopedVmLock lock;
    void* res = android_pipe_guest_open(this);
    if (!res) {
        mErrno = ENOENT;
        return nullptr;
    }
    mPipes.insert(res);
    return res;
}

void* HostGoldfishPipeDevice::popHwSidePipe(void* pipe) {
    RecursiveScopedVmLock lock;
    auto it = mResettedPipes.find(pipe);
    if (it == mResettedPipes.end()) {
        return nullptr;
    } else {
        auto res = it->second;
        mResettedPipes.erase(pipe);
        return res;
    }
}

void HostGoldfishPipeDevice::setErrno(ssize_t res) {
    if (res >= 0) return;
    switch (res) {
        case PIPE_ERROR_INVAL:
            mErrno = EINVAL;
            break;
        case PIPE_ERROR_AGAIN:
            mErrno = EAGAIN;
            break;
        case PIPE_ERROR_NOMEM:
            mErrno = ENOMEM;
            break;
        case PIPE_ERROR_IO:
            mErrno = EIO;
            break;
    }
}

static LazyInstance<HostGoldfishPipeDevice> sDevice = LAZY_INSTANCE_INIT;

// static
HostGoldfishPipeDevice* HostGoldfishPipeDevice::get() {
    auto res = sDevice.ptr();
    // Must be separate from construction
    // as some initialization routines require
    // the instance to be constructed.
    res->initialize();
    return res;
}

// Callbacks for AndroidPipeHwFuncs.
static void sResetPipe(void* hwpipe, void* internal_pipe) {
    HostGoldfishPipeDevice::get()->resetPipe(hwpipe, internal_pipe);
}

static void sCloseFromHost(void* hwpipe) {
    HostGoldfishPipeDevice::get()->close(hwpipe);
}

static void sSignalWake(void* hwpipe, unsigned wakes) {
    HostGoldfishPipeDevice::get()->signalWake(wakes);
}

} // namespace android

extern "C" {

void android_host_pipe_device_initialize() {
    android::HostGoldfishPipeDevice::get();
}

void* android_host_pipe_connect(const char* name) {
    return android::HostGoldfishPipeDevice::get()->connect(name);
}

void android_host_pipe_close(void* pipe) {
    android::HostGoldfishPipeDevice::get()->close(pipe);
}

ssize_t android_host_pipe_read(void* pipe, void* buffer, size_t len) {
    return android::HostGoldfishPipeDevice::get()->read(pipe, buffer, len);
}

ssize_t android_host_pipe_write(void* pipe, const void* buffer, size_t len) {
    return android::HostGoldfishPipeDevice::get()->write(pipe, buffer, len);
}

unsigned android_host_pipe_poll(void* pipe) {
    return android::HostGoldfishPipeDevice::get()->poll(pipe);
}

android_host_pipe_dispatch make_host_pipe_dispatch() {
    android::HostGoldfishPipeDevice::get();
    android_host_pipe_dispatch res;
    res.connect = android_host_pipe_connect;
    res.close = android_host_pipe_close;
    res.read = android_host_pipe_read;
    res.write = android_host_pipe_write;
    res.poll = android_host_pipe_poll;
    return res;
}

}

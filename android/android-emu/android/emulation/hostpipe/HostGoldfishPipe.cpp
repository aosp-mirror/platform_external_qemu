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
#include "android/utils/stream.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <errno.h>
#include <stdint.h>
#include <string.h>

#ifdef _MSC_VER
#ifdef ERROR
#undef ERROR
#endif
#endif

using android::base::LazyInstance;
using android::base::Result;
using android::base::Ok;
using android::base::Err;

namespace android {

HostGoldfishPipeDevice::~HostGoldfishPipeDevice() {
    for (auto it : mHwPipeToPipe) {
        close(it.first);
    }

    ScopedVmLock lock;
    mHwPipeWakeCallbacks.clear();
    mPipes.clear();
    mPipeToHwPipe.clear();
    mHwPipeToPipe.clear();
}

// Also opens.
void* HostGoldfishPipeDevice::connect(const char* name) {
    ScopedVmLock lock;
    void* pipe = open(createNewHwPipeId());

    mCurrentPipeWantingConnection = pipe;

    std::string handshake("pipe:");

    handshake += name;

    const ssize_t len = static_cast<ssize_t>(handshake.size()) + 1;
    const ssize_t ret = writeInternal(pipe, handshake.c_str(), len);

    if (ret == len) {
        // pipe is currently refencing the connect pipe.  After writing the pipe
        // name we should have received a resetPipe, which returns the real
        // host-side pipe.
        void* hostPipe = popHostPipe(pipe);
        if (!hostPipe) {
            crashhandler_die_format(
                    "FATAL: Tried to connect with %s, "
                    "but could not get host pipe.",
                    handshake.c_str());
        }
        fprintf(stderr, "%s: new host pipe: %p\n", __func__, hostPipe);
        mPipes.insert(hostPipe);
        return mPipeToHwPipe[hostPipe];
    } else {
        LOG(ERROR) << "Could not connect to goldfish pipe name: " << name;
        mResettedPipes.erase(pipe);
        android_pipe_guest_close(pipe, PIPE_CLOSE_GRACEFUL);
        mErrno = EIO;
        return nullptr;
    }
}

void HostGoldfishPipeDevice::close(void* pipe) {
    fprintf(stderr, "%s: close pipe %p\n", __func__, pipe);
    ScopedVmLock lock;

    auto it = mHwPipeToPipe.find(pipe);
    if (it != mHwPipeToPipe.end()) {
        android_pipe_guest_close(it->second, PIPE_CLOSE_GRACEFUL);
        mHwPipeWakeCallbacks.erase(pipe);
        mHwPipeToPipe.erase(pipe);
        fprintf(stderr, "%s: erase host pipe %p\n", __func__, it->second);
        mPipeToHwPipe.erase(it->second);
        mPipes.erase(it->second);
        for (auto remPipe : mPipes) {
            if (it->second == remPipe) {
                fprintf(stderr, "%s: wtf pipe %p still there\n", __func__, it->second);
            }
        }
    } else {
        fprintf(stderr, "%s: Could not close pipe %p, ENOENT.\n", __func__, pipe);
        LOG(INFO) << "Could not close pipe, ENOENT.";
        mErrno = ENOENT;
    }
}

void* HostGoldfishPipeDevice::load(base::Stream* stream) {
    ScopedVmLock lock;
    void* hwpipe = createNewHwPipeId();

    char forceClose = 0;
    void* hostPipe = android_pipe_guest_load(reinterpret_cast<Stream*>(stream),
                                             hwpipe, &forceClose);
    if (!forceClose) {
        mPipeToHwPipe[hostPipe] = hwpipe;
        mHwPipeToPipe[hwpipe] = hostPipe;
        fprintf(stderr, "%s: insert host pipe %p\n", __func__, hostPipe);
        mPipes.insert(hostPipe);
        return hwpipe;
    } else {
        LOG(ERROR) << "Could not load goldfish pipe";
        mErrno = EIO;
        return nullptr;
    }
}

void HostGoldfishPipeDevice::saveSnapshot(base::Stream* stream) {
    fprintf(stderr, "%s: call\n", __func__);

    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_save(cStream);

    stream_put_be32(cStream, mPipes.size());
    forEachHwPipe([cStream](void* hwpipe) {
        fprintf(stderr, "%s: save hw pipe: 0x%llx\n", __func__, (unsigned long long)hwpipe);
        stream_put_be64(cStream, (uint64_t)(uintptr_t)hwpipe);
    });

    forEachPipe([cStream](void* hostpipe) {
        fprintf(stderr, "%s: save host pipe: 0x%llx\n", __func__, (unsigned long long)hostpipe);
        android_pipe_guest_save(hostpipe, cStream);
    });

    android_pipe_guest_post_save(cStream);
}

void HostGoldfishPipeDevice::loadSnapshot(base::Stream* stream) {
    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_load(cStream);

    uint32_t pipeCount = stream_get_be32(cStream);

    mPipes.clear();
    mPipeToHwPipe.clear();
    mHwPipeToPipe.clear();

    std::vector<void*> hwPipesToLoad;
    for (uint32_t i = 0; i < pipeCount; ++i) {
        auto hwpipe = (void*)(uintptr_t)stream_get_be64(cStream);
        fprintf(stderr, "%s: load hw pipe: %p\n", __func__, hwpipe);
        hwPipesToLoad.push_back(hwpipe);
    }

    for (auto hwpipe : hwPipesToLoad) {
        char forceClose = 0;
        fprintf(stderr, "%s: load a host pipe\n", __func__);
        void* hostPipe =
            android_pipe_guest_load(cStream, hwpipe, &forceClose);
        if (!forceClose) {
            fprintf(stderr, "%s: map pipe %p to hw pipe %p\n", __func__, hostPipe, hwpipe);
            mPipeToHwPipe[hostPipe] = hwpipe;
            mHwPipeToPipe[hwpipe] = hostPipe;
            mPipes.insert(hostPipe);
        } else {
            fprintf(stderr, "%s: Could not load goldfish pipe\n", __func__);
            LOG(ERROR) << "Could not load goldfish pipe";
            mErrno = EIO;
            return nullptr;
        }
    }

    android_pipe_guest_post_load(cStream);
}

// Read/write/poll but for a particular pipe.
ssize_t HostGoldfishPipeDevice::read(void* pipe, void* buffer, size_t len) {
    ScopedVmLock lock;

    auto it = mHwPipeToPipe.find(pipe);
    if (it == mHwPipeToPipe.end()) {
        fprintf(stderr, "HostGoldfishPipeDevice::%s: Pipe %p not found.\n", __func__, pipe);
        LOG(ERROR) << "Pipe not found.";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }

    AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
    ssize_t res = android_pipe_guest_recv(it->second, &buf, 1);
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
    ScopedVmLock lock;

    auto it = mHwPipeToPipe.find(pipe);
    if (it == mHwPipeToPipe.end()) {
        fprintf(stderr, "HostGoldfishPipeDevice::%s: Pipe %p not found.\n", __func__, pipe);
        LOG(ERROR) << "Pipe not found.";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }

    return writeInternal(it->second, buffer, len);
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
    ScopedVmLock lock;
    const auto it = mHwPipeToPipe.find(pipe);
    return android_pipe_guest_poll(it->second);
}

int HostGoldfishPipeDevice::getErrno() const {
    ScopedVmLock lock;
    return mErrno;
}

void HostGoldfishPipeDevice::setWakeCallback(
        void* pipe,
        std::function<void(int)> callback) {
    ScopedVmLock lock;
    auto it = mHwPipeToPipe.find(pipe);
    if (it != mHwPipeToPipe.end()) {
        mHwPipeWakeCallbacks[pipe] = callback;
    } else {
        LOG(INFO) << "setWakeCallbacks could not find hwpipe for pipe.";
    }
}

static const AndroidPipeHwFuncs sHostGoldfishPipeHwFuncs = {
    &HostGoldfishPipeDevice::resetPipeCallback,
    &HostGoldfishPipeDevice::closeFromHostCallback,
    &HostGoldfishPipeDevice::signalWakeCallback,
};

void HostGoldfishPipeDevice::initialize() {
    if (mInitialized) return;
    android_pipe_set_hw_funcs(&sHostGoldfishPipeHwFuncs);
    AndroidPipe::Service::resetAll();
    AndroidPipe::initThreading(android::HostVmLock::getInstance());
    mInitialized = true;
}

// locked
void* HostGoldfishPipeDevice::open(void* hwpipe) {
    void* res = android_pipe_guest_open(hwpipe);
    if (!res) {
        mErrno = ENOENT;
        return nullptr;
    }
    return res;
}

ssize_t HostGoldfishPipeDevice::writeInternal(void* pipe,
                                              const void* buffer,
                                              size_t len) {
    AndroidPipeBuffer buf = {(uint8_t*)buffer, len};
    ssize_t res = android_pipe_guest_send(pipe, &buf, 1);
    setErrno(res);
    return res;
}

// locked
void* HostGoldfishPipeDevice::popHostPipe(void* pipe) {
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

void* HostGoldfishPipeDevice::createNewHwPipeId() {
    return reinterpret_cast<void*>(mNextHwPipe++);
}

// locked
void HostGoldfishPipeDevice::resetPipe(void* hwpipe, void* internal_pipe) {
    fprintf(stderr, "%s: set internal pipe %p to hwpipe %p\n", __func__, internal_pipe, hwpipe);
    mPipeToHwPipe[internal_pipe] = hwpipe;
    mHwPipeToPipe[hwpipe] = internal_pipe;
    mResettedPipes[mCurrentPipeWantingConnection] = internal_pipe;
}

void HostGoldfishPipeDevice::closeHwPipe(void* hwpipe) {
    ScopedVmLock lock;
    // To avoid having to also create a hwpipe to pipe map, just do a linear
    // scan.
    auto it = std::find_if(mPipeToHwPipe.begin(), mPipeToHwPipe.end(),
                           [hwpipe](const std::pair<void*, void*>& v) {
                               return v.second == hwpipe;
                           });
    if (it != mPipeToHwPipe.end()) {
        close(it->first);
    } else {
        LOG(INFO) << "Could not close pipe, ENOENT.";
        mErrno = ENOENT;
    }
}

void HostGoldfishPipeDevice::signalWake(void* hwpipe, int wakes) {
    ScopedVmLock lock;
    auto callbackIt = mHwPipeWakeCallbacks.find(hwpipe);
    if (callbackIt != mHwPipeWakeCallbacks.end()) {
        (callbackIt->second)(wakes);
    }
}

std::vector<void*> HostGoldfishPipeDevice::orderedHostPipes() const {
    std::vector<void*> res;
    for (auto hostpipe : mPipes) {
        fprintf(stderr, "%s: host pipe: %p\n", __func__, hostpipe);
        res.push_back(hostpipe);
    }

    std::sort(res.begin(), res.end(), [](void* a, void* b) {
        uint64_t aN = (uint64_t)(uintptr_t)a;
        uint64_t bN = (uint64_t)(uintptr_t)b;
        return aN < bN;
    });

    return res;
}

void HostGoldfishPipeDevice::forEachHwPipe(HostGoldfishPipeDevice::PipeCallback func) {
    for (auto hostpipe : orderedHostPipes()) {
        if (mPipeToHwPipe.find(hostpipe) == mPipeToHwPipe.end()) {
            fprintf(stderr, "%s: GOOFED: No hw pipe for host pipe %p\n", __func__, hostpipe);
        }
        func(mPipeToHwPipe[hostpipe]);
    }
}

void HostGoldfishPipeDevice::forEachPipe(HostGoldfishPipeDevice::PipeCallback func) {
    for (auto hostpipe : orderedHostPipes()) {
        func(hostpipe);
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
// static
void HostGoldfishPipeDevice::resetPipeCallback(void* hwpipe,
                                               void* internal_pipe) {
    HostGoldfishPipeDevice::get()->resetPipe(hwpipe, internal_pipe);
}

// static
void HostGoldfishPipeDevice::closeFromHostCallback(void* hwpipe) {
    // PIPE_WAKE_CLOSED gets translated to closeFromHostCallback.
    // To simplify detecting a close-from-host, signal a wake callback so that
    // the event can be detected.
    HostGoldfishPipeDevice::get()->signalWake(hwpipe, PIPE_WAKE_CLOSED);
    HostGoldfishPipeDevice::get()->closeHwPipe(hwpipe);
}

// static
void HostGoldfishPipeDevice::signalWakeCallback(void* hwpipe, unsigned wakes) {
    HostGoldfishPipeDevice::get()->signalWake(hwpipe, wakes);
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

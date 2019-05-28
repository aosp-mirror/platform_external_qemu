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

#include "android/base/containers/Lookup.h"
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

#define HOST_PIPE_DEBUG 0

#if HOST_PIPE_DEBUG

#define HOST_PIPE_DLOG(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);

#else

#define HOST_PIPE_DLOG(fmt,...)

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
        HOST_PIPE_DLOG("New pipe: service: [%s] hwpipe: %p hostpipe: %p",
                       name, mPipeToHwPipe[hostPipe], hostPipe);
        return mPipeToHwPipe[hostPipe];
    } else {
        LOG(ERROR) << "Could not connect to goldfish pipe name: " << name;
        mResettedPipes.erase(pipe);
        android_pipe_guest_close(pipe, PIPE_CLOSE_GRACEFUL);
        mErrno = EIO;
        return nullptr;
    }
}

void HostGoldfishPipeDevice::close(void* hwpipe) {
    HOST_PIPE_DLOG("Close hw pipe %p", hwpipe);
    ScopedVmLock lock;

    auto it = mHwPipeToPipe.find(hwpipe);
    if (it != mHwPipeToPipe.end()) {
        android_pipe_guest_close(it->second, PIPE_CLOSE_GRACEFUL);
        mHwPipeWakeCallbacks.erase(hwpipe);
        HOST_PIPE_DLOG("Erased host pipe %p corresponding to %p",
                       it->second, hwpipe);
        mPipeToHwPipe.erase(it->second);
        mHwPipeToPipe.erase(hwpipe);
    } else {
        LOG(INFO) << "Could not close pipe, ENOENT.";
        mErrno = ENOENT;
    }
}

void* HostGoldfishPipeDevice::getHostPipe(void* hwpipe) const {
    void** res = (void**)android::base::find(mHwPipeToPipe, hwpipe);
    if (!res) return nullptr;
    return *res;
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
        HOST_PIPE_DLOG("Successfully loaded hw pipe %p (host pipe %p)",
                       hwpipe, hostPipe);
        return hwpipe;
    } else {
        LOG(ERROR) << "Could not load goldfish pipe";
        mErrno = EIO;
        return nullptr;
    }
}

void HostGoldfishPipeDevice::saveSnapshot(base::Stream* stream) {
    HOST_PIPE_DLOG("Saving snapshot");

    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_save(cStream);

    stream_put_be32(cStream, mHwPipeToPipe.size());

    for (auto it : mHwPipeToPipe) {
        auto hwpipe = it.first;
        auto hostpipe = it.second;

        HOST_PIPE_DLOG("save hw pipe: %p", hwpipe);
        stream_put_be64(cStream, (uint64_t)(uintptr_t)hwpipe);
        android_pipe_guest_save(hostpipe, cStream);
    }

    android_pipe_guest_post_save(cStream);
}

void HostGoldfishPipeDevice::loadSnapshot(base::Stream* stream) {
    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_load(cStream);

    uint32_t pipeCount = stream_get_be32(cStream);

    mPipeToHwPipe.clear();
    mHwPipeToPipe.clear();

    for (uint32_t i = 0; i < pipeCount; ++i) {
        auto hwpipe = (void*)(uintptr_t)stream_get_be64(cStream);

        HOST_PIPE_DLOG("load hw pipe: %p", hwpipe);

        char forceClose = 0;

        HOST_PIPE_DLOG("attempt to load a host pipe");

        void* hostPipe =
            android_pipe_guest_load(cStream, hwpipe, &forceClose);

        if (!forceClose) {
            HOST_PIPE_DLOG("Successfully loaded host pipe %p for hw pipe %p", hostPipe, hwpipe);
            mPipeToHwPipe[hostPipe] = hwpipe;
            mHwPipeToPipe[hwpipe] = hostPipe;
        } else {
            HOST_PIPE_DLOG("Failed to load host pipe for hw pipe %p", hwpipe);
            LOG(ERROR) << "Could not load goldfish pipe";
            mErrno = EIO;
            return;
        }
    }

    android_pipe_guest_post_load(cStream);
}

void HostGoldfishPipeDevice::saveSnapshot(base::Stream* stream, void* hwpipe) {
    HOST_PIPE_DLOG("Saving snapshot for pipe %p", hwpipe);

    if (mHwPipeToPipe.find(hwpipe) == mHwPipeToPipe.end()) {
        LOG(ERROR) << "No host pipe found for hw pipe";
        mErrno = EIO;
        return;
    }

    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_save(cStream);

    stream_put_be32(cStream, 1);
    stream_put_be64(cStream, (uint64_t)(uintptr_t)hwpipe);
    android_pipe_guest_save(mHwPipeToPipe[hwpipe], cStream);
    android_pipe_guest_post_save(cStream);
}

void* HostGoldfishPipeDevice::loadSnapshotSinglePipe(base::Stream* stream) {
    HOST_PIPE_DLOG("Loading snapshot for a single pipe");

    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_load(cStream);
    uint32_t pipeCount = stream_get_be32(cStream);

    if (pipeCount != 1) {
        LOG(ERROR) << "Invalid pipe count from stream";
        mErrno = EIO;
        return nullptr;
    }

    auto hwPipeFromStream = (void*)(uintptr_t)stream_get_be64(cStream);
    mPipeToHwPipe.erase(mHwPipeToPipe[hwPipeFromStream]);

    HOST_PIPE_DLOG("attempt to load a host pipe");

    char forceClose = 0;
    void* hostPipe =
        android_pipe_guest_load(cStream, hwPipeFromStream, &forceClose);
    if (!forceClose) {
        HOST_PIPE_DLOG("Successfully loaded host pipe %p for hw pipe %p", hostPipe, hwPipeFromStream);
        mPipeToHwPipe[hostPipe] = hwPipeFromStream;
        mHwPipeToPipe[hwPipeFromStream] = hostPipe;
    } else {
        HOST_PIPE_DLOG("Failed to load host pipe for hw pipe %p", hwPipeFromStream);
        LOG(ERROR) << "Could not load goldfish pipe";
        mErrno = EIO;
        return nullptr;
    }

    android_pipe_guest_post_load(cStream);

    return hwPipeFromStream;
}

// Read/write/poll but for a particular pipe.
ssize_t HostGoldfishPipeDevice::read(void* pipe, void* buffer, size_t len) {
    ScopedVmLock lock;

    auto it = mHwPipeToPipe.find(pipe);
    if (it == mHwPipeToPipe.end()) {
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
    HOST_PIPE_DLOG("Set host pipe %p for hw pipe %p",
                   internal_pipe, hwpipe);
    mPipeToHwPipe[internal_pipe] = hwpipe;
    mHwPipeToPipe[hwpipe] = internal_pipe;
    mResettedPipes[mCurrentPipeWantingConnection] = internal_pipe;
}

void HostGoldfishPipeDevice::closeHwPipe(void* hwpipe) {
    ScopedVmLock lock;
    close(hwpipe);
}

void HostGoldfishPipeDevice::signalWake(void* hwpipe, int wakes) {
    ScopedVmLock lock;
    auto callbackIt = mHwPipeWakeCallbacks.find(hwpipe);
    if (callbackIt != mHwPipeWakeCallbacks.end()) {
        (callbackIt->second)(wakes);
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

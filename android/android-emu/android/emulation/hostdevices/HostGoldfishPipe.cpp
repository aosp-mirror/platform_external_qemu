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

#include "android/emulation/hostdevices/HostGoldfishPipe.h"

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

const AndroidPipeHwFuncs HostGoldfishPipeDevice::HostHwPipe::vtbl = {
    &closeFromHostCallback,
    &signalWakeCallback,
    &getPipeIdCallback,
};

HostGoldfishPipeDevice::HostHwPipe::HostHwPipe(int fd) : vtblPtr(&vtbl), mFd(fd) {}

HostGoldfishPipeDevice::HostHwPipe::~HostHwPipe() {}

std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe>
HostGoldfishPipeDevice::HostHwPipe::create(int fd) {
    return std::make_unique<HostHwPipe>(fd);
}

HostGoldfishPipeDevice::HostGoldfishPipeDevice() {}

HostGoldfishPipeDevice::~HostGoldfishPipeDevice() {
    clearLocked();  // no lock required in dctor
}

int HostGoldfishPipeDevice::getErrno() const {
    ScopedVmLock lock;
    return mErrno;
}

// Also opens.
int HostGoldfishPipeDevice::connect(const char* name) {
    const auto handshake = std::string("pipe:") + name;

    ScopedVmLock lock;
    const int hwPipeFd = ++mFdGenerator;
    std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create(hwPipeFd);

    InternalPipe* hostPipe = static_cast<InternalPipe*>(
        android_pipe_guest_open(hwPipe.get()));
    if (!hostPipe) {
        mErrno = ENOENT;
        return kNoFd;
    }

    const ssize_t len = static_cast<ssize_t>(handshake.size()) + 1;
    const ssize_t ret = writeInternal(&hostPipe, handshake.c_str(), len);

    if (ret == len) {
        HostHwPipe* hwPipeWeak = associatePipes(std::move(hwPipe), hostPipe);
        if (hwPipeWeak) {
            HOST_PIPE_DLOG("New pipe: service: for '%s', hwpipe: %p hostpipe: %p",
                           name, hwPipeWeak, hostPipe);

            return hwPipeWeak->getFd();
        } else {
            mErrno = ENOENT;
            return kNoFd;
        }
    } else {
        LOG(ERROR) << "Could not connect to goldfish pipe name: " << name;
        android_pipe_guest_close(hostPipe, PIPE_CLOSE_GRACEFUL);
        mErrno = EIO;
        return kNoFd;
    }
}

void HostGoldfishPipeDevice::close(const int fd) {
    HOST_PIPE_DLOG("Close fd=%d", fd);

    ScopedVmLock lock;
    if (!eraseFdInfo(fd)) {
        LOG(INFO) << "Could not close pipe, ENOENT.";
        mErrno = ENOENT;
    }
}

// Read/write/poll but for a particular pipe.
ssize_t HostGoldfishPipeDevice::read(const int fd, void* buffer, size_t len) {
    ScopedVmLock lock;

    FdInfo* fdInfo = lookupFdInfo(fd);
    if (!fdInfo) {
        LOG(ERROR) << "fd=" << fd << " is not a valid pipe fd";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }

    AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
    ssize_t res = android_pipe_guest_recv(fdInfo->hostPipe, &buf, 1);
    setErrno(res);
    return res;
}

HostGoldfishPipeDevice::ReadResult HostGoldfishPipeDevice::read(int fd, size_t maxLength) {
    std::vector<uint8_t> resultBuffer(maxLength);
    ssize_t read_size = read(fd, resultBuffer.data(), maxLength);

    if (read_size < 0) {
        return Err(mErrno);
    } else {
        if (read_size < maxLength) {
            resultBuffer.resize(read_size);
        }
        return Ok(resultBuffer);
    }
}

ssize_t HostGoldfishPipeDevice::write(const int fd, const void* buffer, size_t len) {
    ScopedVmLock lock;

    FdInfo* fdInfo = lookupFdInfo(fd);
    if (!fdInfo) {
        LOG(ERROR) << "fd=" << fd << " is not a valid pipe fd";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }

    return writeInternal(&fdInfo->hostPipe, buffer, len);
}

HostGoldfishPipeDevice::WriteResult
HostGoldfishPipeDevice::write(const int fd, const std::vector<uint8_t>& data) {
    ssize_t res = write(fd, data.data(), data.size());

    if (res < 0) {
        return Err(mErrno);
    } else {
        return Ok(res);
    }
}

unsigned HostGoldfishPipeDevice::poll(const int fd) const {
    ScopedVmLock lock;

    const FdInfo* fdInfo = lookupFdInfo(fd);
    if (!fdInfo) {
        LOG(ERROR) << "fd=" << fd << " is not a valid pipe fd";
        return 0;
    }

    return android_pipe_guest_poll(fdInfo->hostPipe);
}

void HostGoldfishPipeDevice::setWakeCallback(const int fd,
                                             std::function<void(int)> callback) {
    ScopedVmLock lock;

    FdInfo* fdInfo = lookupFdInfo(fd);
    if (fdInfo) {
        fdInfo->wakeCallback = std::move(callback);
    }
}

void* HostGoldfishPipeDevice::getHostPipe(const int fd) const {
    ScopedVmLock lock;

    const FdInfo* fdInfo = lookupFdInfo(fd);
    return fdInfo ? fdInfo->hostPipe : nullptr;
}

void HostGoldfishPipeDevice::saveSnapshot(base::Stream* stream) {
    HOST_PIPE_DLOG("Saving snapshot");

    auto cStream = reinterpret_cast<::Stream*>(stream);

    ScopedVmLock lock;

    android_pipe_guest_pre_save(cStream);
    stream_put_be32(cStream, mFdInfo.size());
    for (const auto& kv : mFdInfo) {
        HOST_PIPE_DLOG("save pipe: fd=%d hwPipe=%p hostPipe=%p",
                       kv.first, kv.second.hwPipe.get(), kv.second.hostPipe);
        stream_put_be32(cStream, kv.first);
        android_pipe_guest_save(kv.second.hostPipe, cStream);
    }
    android_pipe_guest_post_save(cStream);
}

void HostGoldfishPipeDevice::loadSnapshot(base::Stream* stream) {
    auto cStream = reinterpret_cast<::Stream*>(stream);

    ScopedVmLock lock;
    clearLocked();

    android_pipe_guest_pre_load(cStream);
    const uint32_t pipeCount = stream_get_be32(cStream);

    for (uint32_t i = 0; i < pipeCount; ++i) {
        const int fd = stream_get_be32(cStream);
        mFdGenerator = std::max(mFdGenerator, fd);

        std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create(fd);

        HOST_PIPE_DLOG("attempt to load a host pipe");

        char forceClose = 0;
        InternalPipe* hostPipe = static_cast<InternalPipe*>(
            android_pipe_guest_load(cStream, hwPipe.get(), &forceClose));

        if (!forceClose) {
            HostHwPipe* hwPipePtr = associatePipes(std::move(hwPipe), hostPipe);
            HOST_PIPE_DLOG("Successfully loaded host pipe %p for hw pipe %p",
                           hostPipe, hwPipePtr);
        } else {
            HOST_PIPE_DLOG("Failed to load host pipe for hw pipe %p", hwpipe);
            LOG(ERROR) << "Could not load goldfish pipe";
            mErrno = EIO;
            return;
        }
    }

    android_pipe_guest_post_load(cStream);
}

void HostGoldfishPipeDevice::saveSnapshot(base::Stream* stream, const int fd) {
    HOST_PIPE_DLOG("Saving snapshot for fd=%d", fd);

    ScopedVmLock lock;
    FdInfo* fdInfo = lookupFdInfo(fd);
    if (!fdInfo) {
        crashhandler_die_format("%s:%d fd=%d is a valid pipe fd",
                                __func__, __LINE__, fd);
    }

    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_save(cStream);

    stream_put_be32(cStream, 1);
    stream_put_be32(cStream, fd);
    android_pipe_guest_save(fdInfo->hostPipe, cStream);
    android_pipe_guest_post_save(cStream);
}

int HostGoldfishPipeDevice::loadSnapshotSinglePipe(base::Stream* stream) {
    HOST_PIPE_DLOG("Loading snapshot for a single pipe");

    auto cStream = reinterpret_cast<::Stream*>(stream);

    ScopedVmLock lock;

    android_pipe_guest_pre_load(cStream);
    uint32_t pipeCount = stream_get_be32(cStream);

    if (pipeCount != 1) {
        LOG(ERROR) << "Invalid pipe count from stream";
        mErrno = EIO;
        return kNoFd;
    }

    const int fd = stream_get_be32(cStream);
    eraseFdInfo(fd);

    HOST_PIPE_DLOG("attempt to load a host pipe");

    std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create(fd);

    char forceClose = 0;
    InternalPipe* hostPipe = static_cast<InternalPipe*>(
        android_pipe_guest_load(cStream, hwPipe.get(), &forceClose));
    if (!forceClose) {
        associatePipes(std::move(hwPipe), hostPipe);
        HOST_PIPE_DLOG("Successfully loaded host pipe %p for fd=%d", hostPipe, fd);
    } else {
        HOST_PIPE_DLOG("Failed to load host pipe for hw pipe %p", hwPipeFromStream);
        LOG(ERROR) << "Could not load goldfish pipe";
        mErrno = EIO;
    }

    android_pipe_guest_post_load(cStream);

    return fd;
}

void HostGoldfishPipeDevice::clear() {
    ScopedVmLock lock;
    clearLocked();
}

void HostGoldfishPipeDevice::initialize() {
    if (mInitialized) return;
    AndroidPipe::Service::resetAll();
    AndroidPipe::initThreading(android::HostVmLock::getInstance());
    mInitialized = true;
}

// locked
void HostGoldfishPipeDevice::clearLocked() {
    while (true) {
        const auto i = mFdInfo.begin();
        if (i == mFdInfo.end()) {
            break;
        } else {
            eraseFdInfo(i->first);
        }
    }
}

// locked
HostGoldfishPipeDevice::FdInfo* HostGoldfishPipeDevice::lookupFdInfo(int fd) {
    const auto i = mFdInfo.find(fd);
    return (i == mFdInfo.end()) ? nullptr : &i->second;
}

// locked
const HostGoldfishPipeDevice::FdInfo* HostGoldfishPipeDevice::lookupFdInfo(int fd) const {
    const auto i = mFdInfo.find(fd);
    return (i == mFdInfo.end()) ? nullptr : &i->second;
}

// locked
HostGoldfishPipeDevice::HostHwPipe*
HostGoldfishPipeDevice::associatePipes(std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe> hwPipe,
                                       HostGoldfishPipeDevice::InternalPipe* hostPipe) {
    HostHwPipe* hwPipePtr = hwPipe.get();
    const int hwPipeFd = hwPipePtr->getFd();

    mPipeToHwPipe[hostPipe] = hwPipePtr;

    FdInfo info;
    info.hwPipe = std::move(hwPipe);
    info.hostPipe = hostPipe;

    if (!mFdInfo.insert({hwPipeFd, std::move(info)}).second) {
        crashhandler_die_format("%s:%d hwPipeFd=%d already exists",
                        __func__, __LINE__, hwPipeFd);
    }

    return hwPipePtr;
}

// locked
bool HostGoldfishPipeDevice::eraseFdInfo(const int fd) {
    const auto i = mFdInfo.find(fd);
    if (i == mFdInfo.end()) {
        return false;
    }

    if (mPipeToHwPipe.erase(i->second.hostPipe) == 0) {
        crashhandler_die_format("%s:%d hostPipe=%p does not exit while fd=%d exists",
                                __func__, __LINE__, i->second.hostPipe, fd);
    }

    android_pipe_guest_close(i->second.hostPipe, PIPE_CLOSE_GRACEFUL);
    mFdInfo.erase(i);

    return true;
}

ssize_t HostGoldfishPipeDevice::writeInternal(InternalPipe** ppipe,
                                              const void* buffer,
                                              size_t len) {
    AndroidPipeBuffer buf = {(uint8_t*)buffer, len};
    ssize_t res = android_pipe_guest_send((void**)ppipe, &buf, 1);
    setErrno(res);
    return res;
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

void HostGoldfishPipeDevice::signalWake(const int fd, const int wakes) {
    ScopedVmLock lock;
    const FdInfo* fdInfo = lookupFdInfo(fd);
    if (fdInfo) {
        fdInfo->wakeCallback(wakes);
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
void HostGoldfishPipeDevice::closeFromHostCallback(void* hwPipeRaw) {
    const int fd = static_cast<const HostHwPipe*>(hwPipeRaw)->getFd();

    // PIPE_WAKE_CLOSED gets translated to closeFromHostCallback.
    // To simplify detecting a close-from-host, signal a wake callback so that
    // the event can be detected.
    HostGoldfishPipeDevice::get()->signalWake(fd, PIPE_WAKE_CLOSED);
    HostGoldfishPipeDevice::get()->close(fd);
}

// static
void HostGoldfishPipeDevice::signalWakeCallback(void* hwPipeRaw, unsigned wakes) {
    const int fd = static_cast<const HostHwPipe*>(hwPipeRaw)->getFd();
    HostGoldfishPipeDevice::get()->signalWake(fd, wakes);
}

// static
int HostGoldfishPipeDevice::getPipeIdCallback(void* hwPipeRaw) {
    return static_cast<const HostHwPipe*>(hwPipeRaw)->getFd();
}

} // namespace android

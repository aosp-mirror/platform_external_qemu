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

HostGoldfishPipeDevice::HostHwPipe::HostHwPipe(uint32_t i)
        : magic(kMagicAlive), id(i) {
    fprintf(stderr, "rkir555 %s:%d this=%p id=%u\n", __func__, __LINE__, this, id);
}

HostGoldfishPipeDevice::HostHwPipe::~HostHwPipe() {
    magic = kMagicDead;
    fprintf(stderr, "rkir555 %s:%d this=%p id=%u\n", __func__, __LINE__, this, id);
}

uint32_t HostGoldfishPipeDevice::HostHwPipe::getId() const { return id; }

std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe>
HostGoldfishPipeDevice::HostHwPipe::create() {
    static uint32_t nextId = 1;
    return create(nextId++);
}

std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe>
HostGoldfishPipeDevice::HostHwPipe::create(uint32_t id) {
    return std::make_unique<HostHwPipe>(id);
}

HostGoldfishPipeDevice::HostHwPipe*
HostGoldfishPipeDevice::HostHwPipe::from(void* ptr) {
    HostHwPipe* pipe = static_cast<HostHwPipe*>(ptr);
    const auto magic = pipe->magic;

    switch (magic) {
    case kMagicAlive:
        return pipe;

    case kMagicDead:
        crashhandler_die_format("%s:%d dead pipe, ptr=%p\n",
                __func__, __LINE__, ptr, magic);
        break;

    default:
        crashhandler_die_format("%s:%d not a pipe, ptr=%p, magic=0x%08X\n",
                __func__, __LINE__, ptr, magic);
        break;
    }

    return nullptr;
}

HostGoldfishPipeDevice::~HostGoldfishPipeDevice() {
    clear();
}

// Also opens.
void* HostGoldfishPipeDevice::connect(const char* name) {
    const auto handshake = std::string("pipe:") + name;

    ScopedVmLock lock;
    std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create();

    InternalPipe* hostPipe = static_cast<InternalPipe*>(
        android_pipe_guest_open(hwPipe.get()));
    if (!hostPipe) {
        mErrno = ENOENT;
        return nullptr;
    }
    mCurrentPipeWantingConnection = hostPipe;

    const ssize_t len = static_cast<ssize_t>(handshake.size()) + 1;
    const ssize_t ret = writeInternal(hostPipe, handshake.c_str(), len);

    if (ret == len) {
        // pipe is currently refencing the connect pipe.  After writing the pipe
        // name we should have received a resetPipe, which returns the real
        // host-side pipe.
        InternalPipe* newHostPipe = popHostPipe(hostPipe);
        if (!hostPipe) {
            crashhandler_die_format(
                    "FATAL: Tried to connect with %s, "
                    "but could not get host pipe.",
                    handshake.c_str());
        }

        HostHwPipe* newHwPipe = mPipeToHwPipe[newHostPipe];

        HOST_PIPE_DLOG("New pipe: service: [%s] hwpipe: %p hostpipe: %p",
                       name, newHwPipe, newHostPipe);

        return newHwPipe;
    } else {
        LOG(ERROR) << "Could not connect to goldfish pipe name: " << name;
        mResettedPipes.erase(hostPipe);
        android_pipe_guest_close(hostPipe, PIPE_CLOSE_GRACEFUL);
        mErrno = EIO;
        return nullptr;
    }
}

void HostGoldfishPipeDevice::close(void* hwPipeRaw) {
    close(HostHwPipe::from(hwPipeRaw)->getId());
}

void HostGoldfishPipeDevice::close(uint32_t hwPipeId) {
    HOST_PIPE_DLOG("Close hwPipeId=%u", hwPipeId);
    ScopedVmLock lock;

    auto it = mHwPipeInfo.find(hwPipeId);
    if (it != mHwPipeInfo.end()) {
        InternalPipe* hostPipe = it->second.hostPipe;
        android_pipe_guest_close(hostPipe, PIPE_CLOSE_GRACEFUL);
        HOST_PIPE_DLOG("Erased hostPipe=%p corresponding to hwPipe=%p",
                       hostPipe, it->second.hwPipe.get());
        mPipeToHwPipe.erase(hostPipe);
        mHwPipeInfo.erase(it);
    } else {
        LOG(INFO) << "Could not close pipe, ENOENT.";
        mErrno = ENOENT;
    }
}

void HostGoldfishPipeDevice::clear() {
    while (true) {
        const auto i = mHwPipeInfo.begin();
        if (i == mHwPipeInfo.end()) {
            break;
        } else {
            close(i->second.hwPipe->getId());
        }
    }
}

HostGoldfishPipeDevice::InternalPipe*
HostGoldfishPipeDevice::getHostPipe(const HostHwPipe* hwpipe) const {
    const auto i = mHwPipeInfo.find(hwpipe->getId());
    return (i != mHwPipeInfo.end()) ? i->second.hostPipe : nullptr;
}

void* HostGoldfishPipeDevice::load(base::Stream* stream) {
    ScopedVmLock lock;
    std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create();

    char forceClose = 0;
    InternalPipe* hostPipe = static_cast<InternalPipe*>(
        android_pipe_guest_load(reinterpret_cast<Stream*>(stream),
                                hwPipe.get(),
                                &forceClose));
    if (!forceClose) {
        HOST_PIPE_DLOG("Successfully loaded hw pipe %p (host pipe %p)",
                       hwpipe, hostPipe);

        return associatePipes(std::move(hwPipe), hostPipe);
    } else {
        LOG(ERROR) << "Could not load goldfish pipe";
        mErrno = EIO;
        return nullptr;
    }
}

void HostGoldfishPipeDevice::saveSnapshot(base::Stream* stream) {
    HOST_PIPE_DLOG("Saving snapshot");

    auto cStream = reinterpret_cast<::Stream*>(stream);

    ScopedVmLock lock;

    android_pipe_guest_pre_save(cStream);

    stream_put_be32(cStream, mHwPipeInfo.size());

    for (const auto& kv : mHwPipeInfo) {
        auto hwPipe = kv.second.hwPipe.get();
        auto hostPipe = kv.second.hostPipe;

        HOST_PIPE_DLOG("save hw pipe: %p", val.hwpipe.get());
        stream_put_be32(cStream, hwPipe->getId());
        android_pipe_guest_save(hostPipe, cStream);
    }

    android_pipe_guest_post_save(cStream);
}

void HostGoldfishPipeDevice::loadSnapshot(base::Stream* stream) {
    auto cStream = reinterpret_cast<::Stream*>(stream);

    ScopedVmLock lock;
    clear();

    android_pipe_guest_pre_load(cStream);
    const uint32_t pipeCount = stream_get_be32(cStream);

    for (uint32_t i = 0; i < pipeCount; ++i) {
        const uint32_t hwPipeId = stream_get_be32(cStream);
        std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create(hwPipeId);

        HOST_PIPE_DLOG("load hw pipe id: %u", hwPipeId);

        char forceClose = 0;

        HOST_PIPE_DLOG("attempt to load a host pipe");

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

#if 0
void HostGoldfishPipeDevice::saveSnapshot1(base::Stream* stream, void* hwpipe) {
    HOST_PIPE_DLOG("Saving snapshot for pipe %p", hwpipe);

    if (mHwPipeInfo.find(hwpipe) == mHwPipeInfo.end()) {
        LOG(ERROR) << "No host pipe found for hw pipe";
        mErrno = EIO;
        return;
    }

    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_save(cStream);

    stream_put_be32(cStream, 1);
    stream_put_be64(cStream, (uint64_t)(uintptr_t)hwpipe);
    android_pipe_guest_save(mHwPipeInfo[hwpipe], cStream);
    android_pipe_guest_post_save(cStream);
}
#endif

#if 0
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
    mPipeToHwPipe.erase(mHwPipeInfo[hwPipeFromStream]);

    HOST_PIPE_DLOG("attempt to load a host pipe");

    char forceClose = 0;
    void* hostPipe =
        android_pipe_guest_load(cStream, hwPipeFromStream, &forceClose);
    if (!forceClose) {
        HOST_PIPE_DLOG("Successfully loaded host pipe %p for hw pipe %p", hostPipe, hwPipeFromStream);
        mPipeToHwPipe[hostPipe] = hwPipeFromStream;
        mHwPipeInfo[hwPipeFromStream] = hostPipe;
    } else {
        HOST_PIPE_DLOG("Failed to load host pipe for hw pipe %p", hwPipeFromStream);
        LOG(ERROR) << "Could not load goldfish pipe";
        mErrno = EIO;
        return nullptr;
    }

    android_pipe_guest_post_load(cStream);

    return hwPipeFromStream;
}
#endif

#if 0
// Read/write/poll but for a particular pipe.
ssize_t HostGoldfishPipeDevice::read(void* pipe, void* buffer, size_t len) {
    ScopedVmLock lock;

    auto it = mHwPipeInfo.find(pipe);
    if (it == mHwPipeInfo.end()) {
        LOG(ERROR) << "Pipe not found.";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }

    AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
    ssize_t res = android_pipe_guest_recv(it->second, &buf, 1);
    setErrno(res);
    return res;
}
#endif

#if 0
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
#endif

#if 0
ssize_t HostGoldfishPipeDevice::write(void* pipe, const void* buffer, size_t len) {
    ScopedVmLock lock;

    auto it = mHwPipeInfo.find(pipe);
    if (it == mHwPipeInfo.end()) {
        LOG(ERROR) << "Pipe not found.";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }

    return writeInternal(it->second, buffer, len);
}
#endif

HostGoldfishPipeDevice::WriteResult HostGoldfishPipeDevice::write(void* pipe, const std::vector<uint8_t>& data) {
    ssize_t res = write(pipe, data.data(), data.size());

    if (res < 0) {
        return Err(mErrno);
    } else {
        return Ok(res);
    }
}

unsigned HostGoldfishPipeDevice::poll(void* hwPipeRaw) const {
    HostHwPipe *hwPipe = HostHwPipe::from(hwPipeRaw);

    ScopedVmLock lock;
    const auto it = mHwPipeInfo.find(hwPipe->getId());
    return android_pipe_guest_poll(it->second.hostPipe);
}

int HostGoldfishPipeDevice::getErrno() const {
    ScopedVmLock lock;
    return mErrno;
}

#if 0
void HostGoldfishPipeDevice::setWakeCallback(
        void* pipe,
        std::function<void(int)> callback) {
    ScopedVmLock lock;
    auto it = mHwPipeInfo.find(pipe);
    if (it != mHwPipeInfo.end()) {
        mHwPipeWakeCallbacks[pipe] = callback;
    } else {
        LOG(INFO) << "setWakeCallbacks could not find hwpipe for pipe.";
    }
}
#endif

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

HostGoldfishPipeDevice::HostHwPipe*
HostGoldfishPipeDevice::associatePipes(std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe> hwPipe,
                                       HostGoldfishPipeDevice::InternalPipe* hostPipe) {
    HostHwPipe* hwPipePtr = hwPipe.get();
    const uint32_t hwPipeId = hwPipePtr->getId();

    if (!mPipeToHwPipe.insert({hostPipe, hwPipePtr}).second) {
        // TODO
    }

    HostHwPipeInfo info;
    info.hwPipe = std::move(hwPipe);
    info.hostPipe = hostPipe;

    if (!mHwPipeInfo.insert({hwPipeId, std::move(info)}).second) {
        // TODO
    }

    return hwPipePtr;
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
HostGoldfishPipeDevice::InternalPipe*
HostGoldfishPipeDevice::popHostPipe(InternalPipe* pipe) {
    auto it = mResettedPipes.find(pipe);
    if (it == mResettedPipes.end()) {
        return nullptr;
    } else {
        auto res = it->second;
        mResettedPipes.erase(it);
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

// locked
void HostGoldfishPipeDevice::resetPipe(HostGoldfishPipeDevice::HostHwPipe* hwPipe,
                                       HostGoldfishPipeDevice::InternalPipe* internalPipe) {
    HOST_PIPE_DLOG("Set host pipe %p for hw pipe %p",
                   internalPipe, hwPipe);
//    TODO
//    mPipeToHwPipe[internal_pipe] = hwpipe;
//    mHwPipeInfo[hwpipe] = internal_pipe;
//    mResettedPipes[mCurrentPipeWantingConnection] = internal_pipe;
}

void HostGoldfishPipeDevice::closeHwPipe(HostGoldfishPipeDevice::HostHwPipe* hwpipe) {
    ScopedVmLock lock;
    close(hwpipe);
}

void HostGoldfishPipeDevice::signalWake(const HostHwPipe* hwPipe, int wakes) {
    ScopedVmLock lock;
    const auto i = mHwPipeInfo.find(hwPipe->getId());
    if (i != mHwPipeInfo.end()) {
        i->second.wakeCallback(wakes);
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
void HostGoldfishPipeDevice::resetPipeCallback(void* hwPipeRaw,
                                               void* internalPipe) {
    HostGoldfishPipeDevice::get()->resetPipe(
        HostHwPipe::from(hwPipeRaw),
        static_cast<InternalPipe*>(internalPipe));
}

// static
void HostGoldfishPipeDevice::closeFromHostCallback(void* hwpipe_raw) {
    HostHwPipe* hwpipe = HostHwPipe::from(hwpipe_raw);

    // PIPE_WAKE_CLOSED gets translated to closeFromHostCallback.
    // To simplify detecting a close-from-host, signal a wake callback so that
    // the event can be detected.
    HostGoldfishPipeDevice::get()->signalWake(hwpipe, PIPE_WAKE_CLOSED);
    HostGoldfishPipeDevice::get()->closeHwPipe(hwpipe);
}

// static
void HostGoldfishPipeDevice::signalWakeCallback(void* hwPipeRaw, unsigned wakes) {
    HostGoldfishPipeDevice::get()->signalWake(HostHwPipe::from(hwPipeRaw),
                                              wakes);
}

// static 
int HostGoldfishPipeDevice::getPipeIdCallback(void* hwPipeRaw) {
    return HostHwPipe::from(hwPipeRaw)->getId();
}

} // namespace android

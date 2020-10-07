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

HostGoldfishPipeDevice::HostHwPipe::HostHwPipe(uint32_t i) : magic(kMagic), id(i) {
    fprintf(stderr, "rkir555 %s:%d this=%p id=%u\n", __func__, __LINE__, this, id);
}

HostGoldfishPipeDevice::HostHwPipe::~HostHwPipe() {
    magic = 0;
    fprintf(stderr, "rkir555 %s:%d this=%p id=%u\n", __func__, __LINE__, this, id);
}

uint32_t HostGoldfishPipeDevice::HostHwPipe::getId() const { return id; }

HostGoldfishPipeDevice::HostHwPipe*
HostGoldfishPipeDevice::HostHwPipe::from(void* ptr) {
    HostHwPipe* that = static_cast<HostHwPipe*>(ptr);
    return (that->magic == kMagic) ? that : nullptr;
}

const HostGoldfishPipeDevice::HostHwPipe*
HostGoldfishPipeDevice::HostHwPipe::from(const void* ptr) {
    return from(const_cast<void*>(ptr));
}

std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe>
HostGoldfishPipeDevice::HostHwPipe::create() {
    static uint32_t nextId = 1;
    return create(nextId++);
}

std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe>
HostGoldfishPipeDevice::HostHwPipe::create(uint32_t id) {
    return std::make_unique<HostHwPipe>(id);
}

HostGoldfishPipeDevice::HostGoldfishPipeDevice() {
    fprintf(stderr, "rkir555 %s:%d this=%p\n", __func__, __LINE__, this);
}

HostGoldfishPipeDevice::~HostGoldfishPipeDevice() {
    fprintf(stderr, "rkir555 %s:%d this=%p\n", __func__, __LINE__, this);
    clear();
}

// Also opens.
void* HostGoldfishPipeDevice::connect(const char* name) {
    fprintf(stderr, "rkir555 %s:%d name='%s'\n", __func__, __LINE__, name);

    const auto handshake = std::string("pipe:") + name;

    ScopedVmLock lock;
    std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create();

    InternalPipe* hostPipe = static_cast<InternalPipe*>(
        android_pipe_guest_open(hwPipe.get()));
    if (!hostPipe) {
        fprintf(stderr, "rkir555 %s:%d name='%s'\n", __func__, __LINE__, name);
        mErrno = ENOENT;
        return nullptr;
    }
    mCurrentPipeWantingConnection = hostPipe;

    const ssize_t len = static_cast<ssize_t>(handshake.size()) + 1;
    const ssize_t ret = writeInternal(hostPipe, handshake.c_str(), len);

    if (ret == len) {
        fprintf(stderr, "rkir555 %s:%d name='%s'\n", __func__, __LINE__, name);
        // pipe is currently refencing the connect pipe.  After writing the pipe
        // name we should have received a resetPipe, which returns the real
        // host-side pipe.
        InternalPipe* newHostPipe = popHostPipe(hostPipe);
        if (!newHostPipe) {
            crashhandler_die_format(
                    "FATAL: Tried to connect with %s, "
                    "but could not get host pipe.",
                    handshake.c_str());
        }

        const auto i = mPipeToHwPipe.find(newHostPipe);
        if (i == mPipeToHwPipe.end()) {
            fprintf(stderr, "rkir555 %s:%d name='%s' hostPipe=%p newHostPipe=%p\n",
                    __func__, __LINE__, name, hostPipe, newHostPipe);
            mErrno = ENOENT;
            return nullptr;
        }

        hwPipe.release();
        HostHwPipe* newHwPipe = i->second;

        HOST_PIPE_DLOG("New pipe: service: [%s] hwpipe: %p hostpipe: %p",
                       name, newHwPipe, newHostPipe);

        fprintf(stderr, "rkir555 %s:%d name='%s'\n", __func__, __LINE__, name);
        return newHwPipe;
    } else {
        LOG(ERROR) << "Could not connect to goldfish pipe name: " << name;
        mResettedPipes.erase(hostPipe);
        android_pipe_guest_close(hostPipe, PIPE_CLOSE_GRACEFUL);
        mErrno = EIO;
        fprintf(stderr, "rkir555 %s:%d name='%s'\n", __func__, __LINE__, name);
        return nullptr;
    }
}

void HostGoldfishPipeDevice::close(void* hwPipeRaw) {
    HOST_PIPE_DLOG("Close hwPipeRaw=%p", hwPipeRaw);
    fprintf(stderr, "rkir555 %s:%d hwPipeRaw=%p\n", __func__, __LINE__, hwPipeRaw);

    ScopedVmLock lock;
    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        LOG(INFO) << "Could not close pipe, ENOENT.";
        mErrno = ENOENT;
        return;
    }

    if (!eraseHwPipe(hwPipe->getId())) {
        crashhandler_die_format("%s:%d hwPipeId=%u is not found",
                                    __func__, __LINE__, hwPipe->getId());
    }
}

void HostGoldfishPipeDevice::clear() {
    while (true) {
        const auto i = mHwPipeInfo.begin();
        if (i == mHwPipeInfo.end()) {
            break;
        } else {
            eraseHwPipe(i->second.hwPipe->getId());
        }
    }
}

void* HostGoldfishPipeDevice::getHostPipe(const void* hwPipeRaw) const {
    ScopedVmLock lock;

    const HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        return nullptr;
    }

    const auto i = mHwPipeInfo.find(hwPipe->getId());
    if (i == mHwPipeInfo.end()) {
        crashhandler_die_format("%s:%d hwPipeId=%u is not found",
                                    __func__, __LINE__, hwPipe->getId());
    }

    return i->second.hostPipe;
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

void HostGoldfishPipeDevice::saveSnapshot(base::Stream* stream, void* hwPipeRaw) {
    HOST_PIPE_DLOG("Saving snapshot for hwPipeRaw=%p", hwPipeRaw);

    ScopedVmLock lock;
    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        crashhandler_die_format("%s:%d hwPipeRaw=%p is not alive",
                                __func__, __LINE__, hwPipeRaw);
    }

    const uint32_t hwPipeId = hwPipe->getId();
    const auto i = mHwPipeInfo.find(hwPipeId);
    if (i == mHwPipeInfo.end()) {
        crashhandler_die_format("%s:%d hwPipeId=%u is not found",
                                __func__, __LINE__, hwPipeId);
    }

    auto cStream = reinterpret_cast<::Stream*>(stream);

    android_pipe_guest_pre_save(cStream);

    stream_put_be32(cStream, 1);
    stream_put_be32(cStream, hwPipeId);
    android_pipe_guest_save(i->second.hostPipe, cStream);
    android_pipe_guest_post_save(cStream);
}

void* HostGoldfishPipeDevice::loadSnapshotSinglePipe(base::Stream* stream) {
    HOST_PIPE_DLOG("Loading snapshot for a single pipe");

    auto cStream = reinterpret_cast<::Stream*>(stream);

    ScopedVmLock lock;

    android_pipe_guest_pre_load(cStream);
    uint32_t pipeCount = stream_get_be32(cStream);

    if (pipeCount != 1) {
        LOG(ERROR) << "Invalid pipe count from stream";
        mErrno = EIO;
        return nullptr;
    }

    const uint32_t hwPipeId = stream_get_be32(cStream);
    eraseHwPipe(hwPipeId);

    HOST_PIPE_DLOG("attempt to load a host pipe");

    HostHwPipe* hwPipePtr = nullptr;
    std::unique_ptr<HostHwPipe> hwPipe = HostHwPipe::create(hwPipeId);

    char forceClose = 0;
    InternalPipe* hostPipe = static_cast<InternalPipe*>(
        android_pipe_guest_load(cStream, hwPipe.get(), &forceClose));
    if (!forceClose) {
        hwPipePtr = associatePipes(std::move(hwPipe), hostPipe);
        HOST_PIPE_DLOG("Successfully loaded host pipe %p for hw pipe %p", hostPipe, hwPipePtr);
    } else {
        HOST_PIPE_DLOG("Failed to load host pipe for hw pipe %p", hwPipeFromStream);
        LOG(ERROR) << "Could not load goldfish pipe";
        mErrno = EIO;
    }

    android_pipe_guest_post_load(cStream);

    return hwPipePtr;
}

// Read/write/poll but for a particular pipe.
ssize_t HostGoldfishPipeDevice::read(void* hwPipeRaw, void* buffer, size_t len) {
    ScopedVmLock lock;

    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        LOG(ERROR) << "hwPipeRaw=" << hwPipeRaw << " is not a valid pipe";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }
    
    const uint32_t hwPipeId = hwPipe->getId();
    auto i = mHwPipeInfo.find(hwPipeId);
    if (i == mHwPipeInfo.end()) {
        crashhandler_die_format("%s:%d hwPipeId=%u is not found",
                                __func__, __LINE__, hwPipeId);
    }

    AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
    ssize_t res = android_pipe_guest_recv(i->second.hostPipe, &buf, 1);
    setErrno(res);
    return res;
}

HostGoldfishPipeDevice::ReadResult HostGoldfishPipeDevice::read(void* pipe, size_t maxLength) {
    std::vector<uint8_t> resultBuffer(maxLength);
    ssize_t read_size = read(pipe, resultBuffer.data(), maxLength);

    if (read_size < 0) {
        return Err(mErrno);
    } else {
        if (read_size < maxLength) {
            resultBuffer.resize(read_size);
        }
        return Ok(resultBuffer);
    }
}

ssize_t HostGoldfishPipeDevice::write(void* hwPipeRaw, const void* buffer, size_t len) {
    ScopedVmLock lock;

    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        LOG(ERROR) << "hwPipeRaw=" << hwPipeRaw << " is not a valid pipe";
        mErrno = EINVAL;
        return PIPE_ERROR_INVAL;
    }

    const uint32_t hwPipeId = hwPipe->getId();
    auto i = mHwPipeInfo.find(hwPipeId);
    if (i == mHwPipeInfo.end()) {
        crashhandler_die_format("%s:%d hwPipeId=%u is not found",
                        __func__, __LINE__, hwPipeId);
    }

    return writeInternal(i->second.hostPipe, buffer, len);
}

HostGoldfishPipeDevice::WriteResult
HostGoldfishPipeDevice::write(void* hwPipeRaw, const std::vector<uint8_t>& data) {
    ssize_t res = write(hwPipeRaw, data.data(), data.size());

    if (res < 0) {
        return Err(mErrno);
    } else {
        return Ok(res);
    }
}

unsigned HostGoldfishPipeDevice::poll(void* hwPipeRaw) const {
    ScopedVmLock lock;

    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        LOG(ERROR) << "hwPipeRaw=" << hwPipeRaw << " is not a valid pipe";
        return 0;  // TODO
    }

    const uint32_t hwPipeId = hwPipe->getId();
    const auto i = mHwPipeInfo.find(hwPipeId);
    if (i == mHwPipeInfo.end()) {
        crashhandler_die_format("%s:%d hwPipeId=%u is not found",
                        __func__, __LINE__, hwPipeId);
    }

    return android_pipe_guest_poll(i->second.hostPipe);
}

int HostGoldfishPipeDevice::getErrno() const {
    ScopedVmLock lock;
    return mErrno;
}

void HostGoldfishPipeDevice::setWakeCallback(
        void* hwPipeRaw,
        std::function<void(int)> callback) {
    ScopedVmLock lock;

    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        return;
    }

    const uint32_t hwPipeId = hwPipe->getId();
    const auto i = mHwPipeInfo.find(hwPipeId);
    if (i == mHwPipeInfo.end()) {
        crashhandler_die_format("%s:%d hwPipeId=%u is not found",
                        __func__, __LINE__, hwPipeId);
    }

    i->second.wakeCallback = std::move(callback);
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
HostGoldfishPipeDevice::HostHwPipe*
HostGoldfishPipeDevice::associatePipes(std::unique_ptr<HostGoldfishPipeDevice::HostHwPipe> hwPipe,
                                       HostGoldfishPipeDevice::InternalPipe* hostPipe) {
    HostHwPipe* hwPipePtr = hwPipe.get();
    const uint32_t hwPipeId = hwPipePtr->getId();

    fprintf(stderr, "%s:%d hwPipe=%p hwPipeId=%u hostPipe=%p\n",
            __func__, __LINE__, hwPipePtr, hwPipeId, hostPipe);

//    if (!mPipeToHwPipe.insert({hostPipe, hwPipePtr}).second) {
//        crashhandler_die_format("%s:%d hostPipe=%p already exists",
//                        __func__, __LINE__, hostPipe);
//    }
    mPipeToHwPipe[hostPipe] = hwPipePtr;

    HostHwPipeInfo info;
    info.hwPipe = std::move(hwPipe);
    info.hostPipe = hostPipe;

    if (!mHwPipeInfo.insert({hwPipeId, std::move(info)}).second) {
        crashhandler_die_format("%s:%d hwPipeId=%u already exists",
                        __func__, __LINE__, hwPipeId);
    }

    return hwPipePtr;
}

bool HostGoldfishPipeDevice::eraseHwPipe(const uint32_t hwPipeId) {
    const auto i = mHwPipeInfo.find(hwPipeId);
    if (i == mHwPipeInfo.end()) {
        return false;
    }

    if (mPipeToHwPipe.erase(i->second.hostPipe) == 0) {
        crashhandler_die_format("%s:%d hostPipe=%p does not exit while hwPipeId=%u exists",
                        __func__, __LINE__, i->second.hostPipe, hwPipeId);
    }

    android_pipe_guest_close(i->second.hostPipe, PIPE_CLOSE_GRACEFUL);
    mHwPipeInfo.erase(i);
    return true;
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
void HostGoldfishPipeDevice::resetPipe(void* hwPipeRaw, void* internalPipeRaw) {
    HOST_PIPE_DLOG("Set host pipe %p for hw pipe %p",
                   internalPipeRaw, hwPipeRaw);

    fprintf(stderr, "%s:%d hwPipe=%p internalPipe=%p\n", __func__, __LINE__, hwPipeRaw, internalPipeRaw);

    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    if (!hwPipe) {
        crashhandler_die_format("%s:%d hwPipeRaw=%p is not a valid HostHwPipe",
                                 __func__, __LINE__, hwPipeRaw);
    }

    InternalPipe* internalPipe = static_cast<InternalPipe*>(internalPipeRaw);
    if (!associatePipes(std::unique_ptr<HostHwPipe>(hwPipe), internalPipe)) {
        fprintf(stderr, "%s:%d bad hwPipe=%p internalPipe=%p\n",
                __func__, __LINE__, hwPipe, internalPipe);
    }

//    if (mResettedPipes.insert({mCurrentPipeWantingConnection, internalPipe}).second) {
//        fprintf(stderr, "%s:%d bad hwPipe=%p mCurrentPipeWantingConnection=%p internalPipe=%p\n",
//                __func__, __LINE__, hwPipe, mCurrentPipeWantingConnection, internalPipe);
//    }

    mResettedPipes[mCurrentPipeWantingConnection] = internalPipe;
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
void HostGoldfishPipeDevice::closeFromHostCallback(void* hwPipeRaw) {
    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);

    // PIPE_WAKE_CLOSED gets translated to closeFromHostCallback.
    // To simplify detecting a close-from-host, signal a wake callback so that
    // the event can be detected.
    HostGoldfishPipeDevice::get()->signalWake(hwPipe, PIPE_WAKE_CLOSED);
    HostGoldfishPipeDevice::get()->closeHwPipe(hwPipe);
}

// static
void HostGoldfishPipeDevice::signalWakeCallback(void* hwPipeRaw, unsigned wakes) {
    HostGoldfishPipeDevice::get()->signalWake(HostHwPipe::from(hwPipeRaw),
                                              wakes);
}

// static 
int HostGoldfishPipeDevice::getPipeIdCallback(void* hwPipeRaw) {
    HostHwPipe* hwPipe = HostHwPipe::from(hwPipeRaw);
    return hwPipe ? hwPipe->getId() : -1;
}

} // namespace android

// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AndroidPipe.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/ThreadStore.h"
#include "android/emulation/android_pipe_device.h"
#include "android/emulation/android_pipe_host.h"
#include "android/emulation/DeviceContextRunner.h"
#include "android/emulation/VmLock.h"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#define DEBUG 0

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define DD(...) (void)0
#endif

#define E(...) fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n")

static const AndroidPipeHwFuncs* sPipeHwFuncs = nullptr;

using namespace android::base;

using AndroidPipe = android::AndroidPipe;
using BaseStream = android::base::Stream;
using CStream = ::Stream;
using OptionalString = android::base::Optional<std::string>;
using Service = android::AndroidPipe::Service;
using ServiceList = std::vector<std::unique_ptr<Service>>;
using VmLock = android::VmLock;

static BaseStream* asBaseStream(CStream* stream) {
    return reinterpret_cast<BaseStream*>(stream);
}

// Use CHECK_VM_STATE_LOCK() to panic in debug builds if the current thread
// doesn't hold the VM state lock.
#if (DEBUG > 0) || !defined(NDEBUG)
#define CHECK_VM_STATE_LOCK()  CHECK(VmLock::get()->isLockedBySelf())
#else
#define CHECK_VM_STATE_LOCK() (void)0
#endif

namespace android {

namespace {

// Write an optional string |str| to |stream|. |str| can be null. Use
// readOptionalString() to read it back later.
static void writeOptionalString(BaseStream* stream, const char* str) {
    if (str) {
        stream->putByte(1);
        stream->putString(str);
    } else {
        stream->putByte(0);
    }
}

// Read an optional string from |stream|. If the result is not constructed
// (i.e. equals to false), this means the original |str| parameter was null.
static OptionalString readOptionalString(BaseStream* stream) {
    if (stream->getByte()) {
        return OptionalString(std::move(stream->getString()));
    }
    return OptionalString();
}

// forward
Service* findServiceByName(const char* name);

// Implementation of a special AndroidPipe class used to model the state
// of a pipe connection before the service name has been written to the
// file descriptor by the guest. The most important method is onGuestSend()
// which will detect when this happens.
class ConnectorPipe : public AndroidPipe {
public:
    ConnectorPipe(void* hwPipe, Service* service)
        : AndroidPipe(hwPipe, service) {
        DD("%s: Creating new ConnectorPipe hwpipe=%p", __FUNCTION__, mHwPipe);
    }

    virtual void onGuestClose() override {
        // nothing to do here
        DD("%s: closing ConnectorPipe hwpipe=%p prematurily!", __FUNCTION__,
           mHwPipe);
    }

    virtual unsigned onGuestPoll() const override {
        // A connector always want to receive data.
        DD("%s: polling hwpipe=%p", __FUNCTION__, mHwPipe);
        return PIPE_POLL_OUT;
    }

    virtual int onGuestRecv(AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        // This pipe never wants to write to the guest, so getting there
        // is an error since PIPE_WAKE_IN is never signaled.
        DD("%s: trying to receive data from hwpipe=%p", __FUNCTION__, mHwPipe);
        return PIPE_ERROR_IO;
    }

    // TECHNICAL NOTE: This function reads data from the guest until it
    // finds a zero-terminated C-string. After that it parses it to find
    // a registered service corresponding to one of the allowed formats
    // (see below). In case of success, this creates a new AndroidPipe
    // instance and calls AndroidPipeHwFuncs::resetPipe() to associate it with
    // the current hardware-side |mHwPipe|, then *deletes* the current
    // instance! In case of error (e.g. invalid service name, or error during
    // initialization), PIPE_ERROR_INVAL will be returned, otherwise, the
    // number of bytes accepted from the guest is returned.
    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        int result = 0;
        size_t avail = kBufferSize - mPos;
        bool foundZero = false;
        for (; !foundZero && avail > 0 && numBuffers > 0;
             buffers++, numBuffers--) {
            const uint8_t* data = buffers[0].data;
            size_t count = std::min(avail, buffers[0].size);
            // Read up to |count| bytes, stopping after the first zero.
            size_t n = 0;
            while (n < count) {
                uint8_t byte = data[n++];
                mBuffer[mPos++] = (char) byte;
                if (!byte) {
                    foundZero = true;
                    break;
                }
            }
            result += static_cast<int>(n);
            avail -= n;
        }
        DD("%s: receiving %d connection bytes from hwpipe=%p", __FUNCTION__,
           result, mHwPipe);

        if (!foundZero) {
            if (avail == 0) {
                DD("%s: service name buffer full, force-closing connection",
                   __FUNCTION__);
                return PIPE_ERROR_IO;
            }
            // Still waiting for terminating zero.
            DD("%s: still waiting for terminating zero!", __FUNCTION__);
            return result;
        }

        // Acceptable formats for the connection string are:
        //
        //    pipe:<name>
        //    pipe:<name>:<arguments>
        //
        char* pipeName;
        char* pipeArgs;

        D("%s: connector: '%s'", __FUNCTION__, mBuffer);
        if (memcmp(mBuffer, "pipe:", 5) != 0) {
            // Nope, we don't handle these for now.
            D("%s: Unknown pipe connection: '%s'", __FUNCTION__, mBuffer);
            return PIPE_ERROR_INVAL;
        }

        pipeName = mBuffer + 5;
        pipeArgs = strchr(pipeName, ':');

        Service* svc = nullptr;

        // As a special case, if the service name is as:
        //    qemud:<name>
        //    qemud:<name>:args
        //
        // First look for a registered pipe service named "qemud:<name>"
        // and if not found, fallback to "qemud" only.
        //
        // This is useful to support qemud services that are now served
        // by a dedicated (and faster) pipe service, e.g. 'qemud:adb'
        // as currently implemented by QEMU2 (and soon by QEMU1).
        static const char kQemudPrefix[] = "qemud:";
        const size_t kQemudPrefixSize = sizeof(kQemudPrefix) - 1U;

        if (!::strncmp(pipeName, kQemudPrefix, kQemudPrefixSize)) {
            assert(pipeArgs == pipeName + kQemudPrefixSize - 1);
            char* pipeArgs2 = strchr(pipeArgs + 1, ':');
            if (pipeArgs2) {
                *pipeArgs2 = '\0';
            }
            svc = findServiceByName(pipeName);
            if (svc) {
                pipeArgs = pipeArgs2;
            } else if (pipeArgs2) {
                // Restore colon.
                *pipeArgs2 = ':';
            }
        }
        if (pipeArgs) {
            *pipeArgs++ = '\0';
            if (!pipeArgs) {
                pipeArgs = NULL;
            }
        }

        if (!svc) {
            svc = findServiceByName(pipeName);
        }

        if (!svc) {
            D("%s: Unknown server with name %s!", __FUNCTION__, pipeName);
            return PIPE_ERROR_INVAL;
        }

        AndroidPipe* newPipe = svc->create(mHwPipe, pipeArgs);
        if (!newPipe) {
            D("%s: Initialization failed for %s pipe!", __FUNCTION__, pipeName);
            return PIPE_ERROR_INVAL;
        }

        // Swap your host-side pipe instance with this one weird trick!
        D("%s: starting new pipe %p (swapping %p) for service %s",
          __FUNCTION__,
          newPipe,
          mHwPipe,
          pipeName);
        sPipeHwFuncs->resetPipe(mHwPipe, newPipe);
        delete this;

        return result;
    }

    virtual void onGuestWantWakeOn(int wakeFlags) override {
        // nothing to do here
        DD("%s: signaling wakeFlags=%d for hwpipe=%p", __FUNCTION__, wakeFlags,
           mHwPipe);
    }

    virtual void onSave(BaseStream* stream) override {
        DD("%s: saving connector state for hwpipe=%p", __FUNCTION__, mHwPipe);
        stream->putBe32(mPos);
        stream->write(mBuffer, mPos);
    }

    bool onLoad(BaseStream* stream) {
        DD("%s: loading connector state for hwpipe=%p", __FUNCTION__, mHwPipe);
        int32_t len = stream->getBe32();
        if (len < 0 || len > kBufferSize) {
            D("%s: invalid length %d (expected 0 <= len <= %d)", __FUNCTION__,
              static_cast<int>(len), kBufferSize);
            return false;
        }
        mPos = (int)len;
        int ret = (int)stream->read(mBuffer, mPos);
        DD("%s: read %d bytes (%d expected)", __FUNCTION__, ret, mPos);
        return (ret == mPos);
    }

private:
    static constexpr int kBufferSize = 128;
    char mBuffer[kBufferSize];
    int mPos = 0;
};

// Associated AndroidPipe::Service class for ConnectorPipe instances.
class ConnectorService : public Service {
public:
    ConnectorService() : Service("<connector>") {}

    virtual AndroidPipe* create(void* hwPipe, const char* args) override {
        return new ConnectorPipe(hwPipe, this);
    }

    virtual bool canLoad() const override { return true; }

    virtual AndroidPipe* load(void* hwPipe,
                              const char* args,
                              BaseStream* stream) override {
        ConnectorPipe* pipe = new ConnectorPipe(hwPipe, this);
        if (!pipe->onLoad(stream)) {
            delete pipe;
            return nullptr;
        }
        return pipe;
    }
};

// A helper class used to send signalWake() and closeFromHost() commands to
// the device thread, depending on the threading mode setup by the emulation
// engine.
struct PipeWakeCommand {
    void* hwPipe;
    int wakeFlags;
};

class PipeWaker final : public DeviceContextRunner<PipeWakeCommand> {
public:
    void signalWake(void* hwPipe, int wakeFlags) {
        queueDeviceOperation({ hwPipe, wakeFlags });
    }
    void closeFromHost(void* hwPipe) {
        signalWake(hwPipe, PIPE_WAKE_CLOSED);
    }
    void abortPending(void* hwPipe) {
        removeAllPendingOperations([hwPipe](const PipeWakeCommand& cmd) {
            return cmd.hwPipe == hwPipe;
        });
    }

private:
    virtual void performDeviceOperation(const PipeWakeCommand& wake_cmd) {
        void* hwPipe = wake_cmd.hwPipe;
        int flags = wake_cmd.wakeFlags;

        if (flags & PIPE_WAKE_CLOSED) {
            sPipeHwFuncs->closeFromHost(hwPipe);
        } else {
            sPipeHwFuncs->signalWake(hwPipe, flags);
        }
    }
};

struct Globals {
    ServiceList services;
    ConnectorService connectorService;
    PipeWaker pipeWaker;

    Service* findServiceByName(const char* name) const {
        for (const auto& service : services) {
            if (service->name() == name) {
                return service.get();
            }
        }
        return nullptr;
    }

    Service* loadServiceByName(BaseStream* stream) {
        OptionalString serviceName = readOptionalString(stream);
        if (!serviceName) {
            DD("%s: no name (assuming connector state)", __FUNCTION__);
            return &connectorService;
        }
        DD("%s: found [%s]", __FUNCTION__, serviceName->c_str());
        return this->findServiceByName(serviceName->c_str());
    }
};

android::base::LazyInstance<Globals> sGlobals = LAZY_INSTANCE_INIT;

Service* findServiceByName(const char* name) {
    return sGlobals->findServiceByName(name);
}

AndroidPipe* loadPipeFromStreamCommon(BaseStream* stream,
                                      void* hwPipe,
                                      Service* service,
                                      char* pForceClose) {
    *pForceClose = 0;

    OptionalString args = readOptionalString(stream);
    AndroidPipe* pipe = nullptr;
    if (service->canLoad()) {
        DD("%s: loading state for [%s] hwpipe=%p", __FUNCTION__,
           service->name().c_str(), hwPipe);
        pipe = service->load(hwPipe, args ? args->c_str() : nullptr, stream);
    } else {
        DD("%s: force-closing hwpipe=%p", __FUNCTION__, hwPipe);
        *pForceClose = 1;
    }
    return pipe;
}

}  // namespace

// static
void AndroidPipe::initThreading(VmLock* vmLock) {
    sGlobals->pipeWaker.init(vmLock);
}

AndroidPipe::~AndroidPipe() {
    DD("%s: for hwpipe=%p (host %p '%s')", __FUNCTION__, mHwPipe, this,
       mService->name().c_str());
}

// static
void AndroidPipe::Service::add(Service* service) {
    DD("Adding new pipe service '%s' this=%p", service->name().c_str(),
       service);
    std::unique_ptr<Service> svc(service);
    sGlobals->services.push_back(std::move(svc));
}

// static
void AndroidPipe::Service::resetAll() {
    DD("Resetting all pipe services");
    sGlobals->services.clear();
}

void AndroidPipe::signalWake(int wakeFlags) {
    sGlobals->pipeWaker.signalWake(mHwPipe, wakeFlags);
}

void AndroidPipe::closeFromHost() {
    sGlobals->pipeWaker.closeFromHost(mHwPipe);
}

void AndroidPipe::abortPendingOperation() {
    sGlobals->pipeWaker.abortPending(mHwPipe);
}

// static
void AndroidPipe::saveToStream(BaseStream* stream) {
    // First, write service name.
    if (mService == &sGlobals->connectorService) {
        // A connector pipe
        stream->putByte(0);
    } else {
        // A regular service pipe.
        stream->putByte(1);
        stream->putString(mService->name());
    }
    writeOptionalString(stream, mArgs.c_str());

    // Save pipe-specific state now.
    onSave(stream);
}

// static
AndroidPipe* AndroidPipe::loadFromStream(BaseStream* stream,
                                         void* hwPipe,
                                         char* pForceClose) {
    Service* service = sGlobals->loadServiceByName(stream);
    if (!service) {
        return nullptr;
    }
    return loadPipeFromStreamCommon(stream, hwPipe, service, pForceClose);
}

// static
AndroidPipe* AndroidPipe::loadFromStreamLegacy(BaseStream* stream,
                                               void* hwPipe,
                                               uint64_t* pChannel,
                                               unsigned char* pWakes,
                                               unsigned char* pClosed,
                                               char* pForceClose) {
    Service* service = sGlobals->loadServiceByName(stream);
    if (!service) {
        return nullptr;
    }
    *pChannel = stream->getBe64();
    *pWakes = stream->getByte();
    *pClosed = stream->getByte();

    return loadPipeFromStreamCommon(stream, hwPipe, service, pForceClose);
}

}  // namespace android

// API for the virtual device.

const AndroidPipeHwFuncs* android_pipe_set_hw_funcs(
        const AndroidPipeHwFuncs* hwFuncs) {
    const AndroidPipeHwFuncs* result = sPipeHwFuncs;
    sPipeHwFuncs = hwFuncs;
    return result;
}

void android_pipe_reset_services() {
    AndroidPipe::Service::resetAll();
}

void* android_pipe_guest_open(void* hwpipe) {
    CHECK_VM_STATE_LOCK();
    DD("%s: Creating new connector pipe for hwpipe=%p", __FUNCTION__, hwpipe);
    return android::sGlobals->connectorService.create(hwpipe, nullptr);
}

void android_pipe_guest_close(void* internalPipe) {
    CHECK_VM_STATE_LOCK();
    auto pipe = static_cast<android::AndroidPipe*>(internalPipe);
    if (pipe) {
        D("%s: host=%p [%s]", __FUNCTION__, pipe, pipe->name());
        pipe->onGuestClose();
    }
}

void android_pipe_guest_pre_load(CStream* stream) {
    CHECK_VM_STATE_LOCK();
    for (const auto& service : android::sGlobals->services) {
        service->preLoad(asBaseStream(stream));
    }
}

void android_pipe_guest_post_load(CStream* stream) {
    CHECK_VM_STATE_LOCK();
    for (const auto& service : android::sGlobals->services) {
        service->postLoad(asBaseStream(stream));
    }
}

void android_pipe_guest_pre_save(CStream* stream) {
    CHECK_VM_STATE_LOCK();
    for (const auto& service : android::sGlobals->services) {
        service->preSave(asBaseStream(stream));
    }
}

void android_pipe_guest_post_save(CStream* stream) {
    CHECK_VM_STATE_LOCK();
    for (const auto& service : android::sGlobals->services) {
        service->postSave(asBaseStream(stream));
    }
}

void android_pipe_guest_save(void* internalPipe, CStream* stream) {
    CHECK_VM_STATE_LOCK();
    auto pipe = static_cast<android::AndroidPipe*>(internalPipe);
    DD("%s: host=%p [%s]", __FUNCTION__, pipe, pipe->name());
    pipe->saveToStream(asBaseStream(stream));
}

void* android_pipe_guest_load(CStream* stream,
                              void* hwPipe,
                              char* pForceClose) {
    CHECK_VM_STATE_LOCK();
    DD("%s: hwpipe=%p", __FUNCTION__, hwPipe);
    return AndroidPipe::loadFromStream(asBaseStream(stream), hwPipe,
                                       pForceClose);
}

void* android_pipe_guest_load_legacy(CStream* stream,
                                     void* hwPipe,
                                     uint64_t* pChannel,
                                     unsigned char* pWakes,
                                     unsigned char* pClosed,
                                     char* pForceClose) {
    CHECK_VM_STATE_LOCK();
    DD("%s: hwpipe=%p", __FUNCTION__, hwPipe);
    return android::AndroidPipe::loadFromStreamLegacy(asBaseStream(stream),
                                                      hwPipe, pChannel, pWakes,
                                                      pClosed, pForceClose);
}

unsigned android_pipe_guest_poll(void* internalPipe) {
    CHECK_VM_STATE_LOCK();
    auto pipe = static_cast<AndroidPipe*>(internalPipe);
    DD("%s: host=%p [%s]", __FUNCTION__, pipe, pipe->name());
    return pipe->onGuestPoll();
}

int android_pipe_guest_recv(void* internalPipe,
                            AndroidPipeBuffer* buffers,
                            int numBuffers) {
    CHECK_VM_STATE_LOCK();
    auto pipe = static_cast<AndroidPipe*>(internalPipe);
    return pipe->onGuestRecv(buffers, numBuffers);
}

int android_pipe_guest_send(void* internalPipe,
                            const AndroidPipeBuffer* buffers,
                            int numBuffers) {
    CHECK_VM_STATE_LOCK();
    auto pipe = static_cast<AndroidPipe*>(internalPipe);
    return pipe->onGuestSend(buffers, numBuffers);
}

void android_pipe_guest_wake_on(void* internalPipe, unsigned wakes) {
    CHECK_VM_STATE_LOCK();
    auto pipe = static_cast<AndroidPipe*>(internalPipe);
    pipe->onGuestWantWakeOn(wakes);
}

// API implemented by the virtual device.
void android_pipe_host_close(void* hwpipe) {
    auto pipe = static_cast<android::AndroidPipe*>(hwpipe);
    D("%s: host=%p [%s]", __FUNCTION__, pipe, pipe->name());
    android::sGlobals->pipeWaker.closeFromHost(pipe);
}

void android_pipe_host_signal_wake(void* hwpipe, unsigned flags) {
    android::sGlobals->pipeWaker.signalWake(hwpipe, flags);
}

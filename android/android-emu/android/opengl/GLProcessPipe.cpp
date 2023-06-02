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

#include "host-common/AndroidPipe.h"
#include "aemu/base/synchronization/Lock.h"

#include "host-common/opengles.h"
#include <assert.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <unordered_set>

using android::base::AutoLock;
using android::base::Lock;

namespace android {
namespace opengl {

struct ProcessPipeIdRegistry {
    Lock lock;
    std::unordered_set<uint64_t> ids;
};

static ProcessPipeIdRegistry sRegistry;

static bool sIdExistsInRegistry(uint64_t id) {
    AutoLock lock(sRegistry.lock);
    auto it = sRegistry.ids.find(id);
    return it != sRegistry.ids.end();
}

namespace {

// GLProcessPipe is a pipe service that is used for releasing graphics resources
// per guest process. At the time being, guest processes can acquire host color
// buffer handles / EGLImage handles and they need to be properly released when
// guest process exits unexpectedly. This class is used to detect if guest
// process exits, so that a proper cleanup function can be called.

// It is done by setting up a pipe per guest process before acquiring color
// buffer handles. When guest process exits, the pipe will be closed, and
// onGuestClose() will trigger the cleanup path.

class GLProcessPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("GLProcessPipe") {}

        bool canLoad() const override { return true; }

        AndroidPipe* create(void* hwPipe, const char* args, enum AndroidPipeFlags flags) override {
            return new GLProcessPipe(hwPipe, this, flags);
        }

        AndroidPipe* load(void* hwPipe, const char* args,
                         base::Stream* stream) override {
            return new GLProcessPipe(hwPipe, this, (AndroidPipeFlags)0, stream);
        }

        void preLoad(base::Stream* stream) override {
            GLProcessPipe::s_headId.store(stream->getBe64());
        }

        void preSave(base::Stream* stream) override {
            stream->putBe64(GLProcessPipe::s_headId.load());
        }
    };

    GLProcessPipe(void* hwPipe, Service* service, enum AndroidPipeFlags flags,
                  base::Stream* loadStream = nullptr)
        : AndroidPipe(hwPipe, service) {
        if (loadStream) {
            m_uniqueId = loadStream->getBe64();
            m_hasData = (loadStream->getByte() != 0);
        } else {
            if (flags & ANDROID_PIPE_VIRTIO_GPU_BIT) {
                m_uniqueId = (uint64_t)(uintptr_t)hwPipe;
                s_headId = m_uniqueId;
            } else {
                m_uniqueId = ++s_headId;
            }
        }
        AutoLock lock(sRegistry.lock);
        sRegistry.ids.insert(m_uniqueId);
        android_onGuestGraphicsProcessCreate(m_uniqueId);
    }

    ~GLProcessPipe() {
        AutoLock lock(sRegistry.lock);
        sRegistry.ids.erase(m_uniqueId);
    }

    void onSave(base::Stream* stream) override {
        stream->putBe64(m_uniqueId);
        stream->putByte(m_hasData ? 1 : 0);
    }

    void onGuestClose(PipeCloseReason reason) override {
        if (sIdExistsInRegistry(m_uniqueId)) {
            android_cleanupProcGLObjects(m_uniqueId);
        }
        delete this;
    }

    unsigned onGuestPoll() const override {
        return PIPE_POLL_IN | PIPE_POLL_OUT;
    }

    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        assert(buffers[0].size >= 8);
        if (m_hasData) {
            m_hasData = false;
            memcpy(buffers[0].data, (const char*)&m_uniqueId, sizeof(m_uniqueId));
            return sizeof(m_uniqueId);
        } else {
            return 0;
        }
    }

    int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers,
                            void** newPipePtr) override {
        // The guest is supposed to send us a confirm code first. The code is
        // 100 (4 byte integer).
        assert(buffers[0].size >= 4);
        int32_t confirmInt = *((int32_t*)buffers[0].data);
        assert(confirmInt == 100);
        (void)confirmInt;
        m_hasData = true;
        return buffers[0].size;
    }

    void onGuestWantWakeOn(int flags) override {}

private:
    // An identifier for the guest process corresponding to this pipe.
    // With very high probability, all currently-active processes have unique
    // identifiers, since the IDs are assigned sequentially from a 64-bit ID
    // space.
    // Please change it if you ever have a use case that exhausts them
    uint64_t m_uniqueId;
    bool m_hasData = false;
    static std::atomic<uint64_t> s_headId;

};

std::atomic<uint64_t> GLProcessPipe::s_headId {0};

}

void registerGLProcessPipeService() {
    AndroidPipe::Service::add(std::make_unique<GLProcessPipe::Service>());
}

void forEachProcessPipeId(std::function<void(uint64_t)> f) {
    AutoLock lock(sRegistry.lock);
    for (auto id: sRegistry.ids) {
        f(id);
    }
}

void forEachProcessPipeIdRunAndErase(std::function<void(uint64_t)> f) {
    AutoLock lock(sRegistry.lock);
    auto it = sRegistry.ids.begin();
    while (it != sRegistry.ids.end()) {
        f(*it);
        it = sRegistry.ids.erase(it);
    }
}

}
}

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

#include "android/opengles.h"
#include <assert.h>
#include <atomic>
#include <memory>

namespace android {
namespace opengl {

namespace {

// TODO: explain what it is doing

class GrallocPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("grallocPipe") {}

        virtual AndroidPipe* create(void* mHwPipe, const char* args) const
                override {
            return new GrallocPipe(mHwPipe, this);
        }
    };

    GrallocPipe(void* hwPipe, const Service* service)
            : AndroidPipe(hwPipe, service) {
        m_uniqueId = ++s_headId;
    }

    virtual void onGuestClose() override {
        // process died on the guest, cleanup gralloc memory on the host
        android_cleanupProcColorbuffers(m_uniqueId);
    }
    virtual unsigned onGuestPoll() const override {
        return PIPE_POLL_IN | PIPE_POLL_OUT;
    }
    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
        assert(buffers[0].size >= 8);
        memcpy(buffers[0].data, (const char*)&m_uniqueId, 8);
        return 8;
    }
    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        // Not doing anything
        assert(buffers[0].size >= 4);
        return 4;
    }
    virtual void onGuestWantWakeOn(int flags) override {}
private:
    // A (supposed to be) unique ID assigned from the host per guest process
    // Please change it if you ever have a use case that exhausts them
    uint64_t m_uniqueId;
    static std::atomic_ullong s_headId;
};

std::atomic_ullong GrallocPipe::s_headId(0);

}

void registerGrallocPipeService() {
    android::AndroidPipe::Service::add(new GrallocPipe::Service());
}

}
}
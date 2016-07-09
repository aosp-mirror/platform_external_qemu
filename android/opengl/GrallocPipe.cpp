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

#include <assert.h>
#include <memory>

namespace android {
namespace opengl {

namespace {

class GrallocPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("gralloc") {}

        virtual AndroidPipe* create(void* mHwPipe, const char* args) const
                override {
            return new GrallocPipe(mHwPipe, this);
        }
    };

    GrallocPipe(void* hwPipe, const Service* service)
            : AndroidPipe(hwPipe, service) {}

    virtual void onGuestClose() override {
        // process died on the guest, cleanup gralloc memory on the host
        // TODO: cleanup stuff in framebuffer
        fprintf(stderr, "guest pid %d closed\n", m_guestPid);
    }
    virtual unsigned onGuestPoll() const override {
        return PIPE_POLL_IN | PIPE_POLL_OUT;
    }
    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
        assert(buffers[0].size >= 4);
        memcpy(buffers[0].data, (const char*)&m_guestPid, 4);
        return 4;
    }
    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        assert(buffers[0].size >= 4);
        m_guestPid = *((const int *)(buffers[0].data));
        fprintf(stderr, "recv guest pid %d\n", m_guestPid);
        return 4;
    }
    virtual void onGuestWantWakeOn(int flags) override {}
private:
    // Process ID on the guest
    int m_guestPid;
};

}

void registerGrallocPipeService() {
    android::AndroidPipe::Service::add(new GrallocPipe::Service());
}

}
}
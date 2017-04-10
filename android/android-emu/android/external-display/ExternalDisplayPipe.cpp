// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/external-display/ExternalDisplayPipe.h"
#include "android/emulation/AndroidPipe.h"
#include "android/external-display/Mirroring.h"
#include "android/utils/debug.h"

#include <assert.h>
#include <atomic>
#include <memory>

#include <sys/types.h>
#include <unistd.h>

namespace android {
namespace display {

namespace {

// ExternalDisplayPipe is a pipe service that is used for emulating external
// displays for the emulator.
// Inside guest, external displays are provided through android remote display
// api from EmulatorRemoteDisplayProvider.apk, installed as system app.

class ExternalDisplayPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("ExternalDisplayPipe") {}

        virtual AndroidPipe* create(void* mHwPipe, const char* args) override {
            return new ExternalDisplayPipe(mHwPipe, this);
        }
    };

    ExternalDisplayPipe(void* hwPipe, Service* service)
        : AndroidPipe(hwPipe, service) {
        VERBOSE_PRINT(extdisplay, "pipe created.\n");
        if (mMirroring != nullptr)
            delete mMirroring;

        mMirroring = new Mirroring();
        if (mMirroring != nullptr)
            mMirroring->setPlatformPrivate(this);
    }

    void onGuestClose(PipeCloseReason reason) override {
        VERBOSE_PRINT(extdisplay, "pipe closed.\n");

        if (mMirroring != nullptr)
            delete mMirroring;
    }

    unsigned onGuestPoll() const override {
        unsigned result = PIPE_POLL_OUT;
        if (mSendSize > 0) {
            result |= PIPE_POLL_IN;
        }
        return result;
    }

    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override {
        VERBOSE_PRINT(extdisplay, "%s(): mSendSize=%d\n", __FUNCTION__,
                      mSendSize);
        if (mSendSize > 0) {
            memcpy(buffers[0].data, (const char*)mSendBuffer, mSendSize);
            buffers[0].size = mSendSize;
            mSendSize = 0;
            return buffers[0].size;
        } else {
            return PIPE_ERROR_AGAIN;
        }
    }

    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override {
        if (mMirroring != nullptr) {
            int rc = mMirroring->processMessage((uint8_t*)buffers[0].data,
                                                buffers[0].size);
            if (rc != 0) {
            }
        }
        return buffers[0].size;
    }

    void onGuestWantWakeOn(int flags) override {
        VERBOSE_PRINT(extdisplay, "%s()\n", __FUNCTION__);
    }

    int sendToGuest(uint8_t* buffer, size_t size) {
        if (size < sizeof(mSendBuffer)) {
            memcpy(mSendBuffer, buffer, size);
            mSendSize = size;
            return size;
        } else {
            derror("ExternalDisplayPipe: too many data to send to guest.\n");
            return -1;
        }
    }

private:
    // buffer to send to the android guest
    uint8_t mSendBuffer[4096];
    size_t mSendSize = 0;

    Mirroring* mMirroring = nullptr;
};

}  // namespace

void registerExternalDisplayPipeService() {
    android::AndroidPipe::Service::add(new ExternalDisplayPipe::Service());
}

// this async send, don't wait for the data really got sent out
int external_display_reply_packet(android::display::Mirroring* mirroring,
                                  uint8_t* buffer,
                                  size_t size) {
    android::display::ExternalDisplayPipe* pipe =
            (android::display::ExternalDisplayPipe*)
                    mirroring->getPlatformPrivate();

    int rc = pipe->sendToGuest(buffer, size);

    int wakeFlags = 0;

    wakeFlags |= PIPE_WAKE_READ;
    wakeFlags |= PIPE_WAKE_WRITE;

    // pipe->signalWake(wakeFlags);
    return rc;
}

}  // namespace display
}  // namespace android

extern "C" void android_init_external_display_pipe() {
    android::display::registerExternalDisplayPipeService();
}

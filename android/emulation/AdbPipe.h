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

#pragma once

#include "android/base/async/Looper.h"
#include "android/emulation/android_pipe.h"

namespace android {
namespace emulation {

class AdbBridgeDevice;

class IAdbPipe {
public:
    IAdbPipe(AdbBridgeDevice* parent,
             android::base::Looper* looper,
             void* hwpipe)
        : mParent(parent), mLooper(looper), mQemuPipeDevice(hwpipe) {}
    virtual ~IAdbPipe() {}

    // Implementation of the AndroidPipeFuncs API.
    // Called from the guest via device infrastructure.
    virtual void guestClose() = 0;
    virtual int guestSendBuffers(const AndroidPipeBuffer* buffers,
                                 int numBuffers) = 0;
    virtual int guestRecvBuffers(AndroidPipeBuffer* buffers,
                                 int numBuffers) = 0;
    virtual unsigned guestPoll() = 0;
    virtual void guestWakeOn(int flags) = 0;

    // API used by AdbBridgeDevice to interact with the pipe.
    virtual bool init();
    virtual bool connectTo(int socket) = 0;

protected:
    AdbBridgeDevice* parent() { return mParent; }

    // Helper functions to call AndroidPipe C API functions.
    void requestGuestClose() { android_pipe_close(mQemuPipeDevice); }
    void requestGuestWakeOn(unsigned flags) {
        android_pipe_wake(mQemuPipeDevice, flags);
    }

public:
    // Static functions to register with the AndroidPipeFuns infrastructure.
    // These simply translate part of the C API in |AndroidPipeFuns| into this
    // interface.
    // These methods are not used anywhere in the implementation of this
    // interface.
    static void onGuestClose(void* pipe) {
        static_cast<IAdbPipe*>(pipe)->guestClose();
    }
    static int onGuestSendBuffers(void* pipe,
                                  const AndroidPipeBuffer* buffers,
                                  int numBuffers) {
        return static_cast<IAdbPipe*>(pipe)->guestSendBuffers(buffers,
                                                              numBuffers);
    }
    static int onGuestRecvBuffers(void* pipe,
                                  AndroidPipeBuffer* buffers,
                                  int numBuffers) {
        return static_cast<IAdbPipe*>(pipe)->guestRecvBuffers(buffers,
                                                              numBuffers);
    }
    static unsigned onGuestPoll(void* pipe) {
        return static_cast<IAdbPipe*>(pipe)->guestPoll();
    }
    static void onGuestWakeOn(void* pipe, int flags) {
        static_cast<IAdbPipe*>(pipe)->guestWakeOn(flags);
    }

protected:
    AdbBridgeDevice* mParent = nullptr;
    android::base::Looper* const mLooper;
    void* mQemuPipeDevice = nullptr;
};

class IAdbPipeFactory {
public:
    virtual ~IAdbPipeFactory();
    // Creates an uninitialized IAdbPipe instance.
    // You must initialize the instance by calling |init| on it.
    virtual IAdbPipe* create(AdbBridgeDevice* parent,
                             android::base::Looper* looper,
                             void* hwpipe) = 0;
};

class AdbPipeFactory : public IAdbPipeFactory {
public:
    IAdbPipe* create(AdbBridgeDevice* parent,
                     android::base::Looper* looper,
                     void* hwpipe) override;
};

}  // namespace emulation
}  // namespace android

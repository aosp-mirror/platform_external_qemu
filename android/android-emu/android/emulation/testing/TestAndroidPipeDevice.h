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

#include "android/emulation/android_pipe_device.h"
#include "android/emulation/android_pipe_base.h"
#include "android/emulation/testing/TestVmLock.h"

#include <memory>

namespace android {

// A class used to implement a test version of the Android pipe virtual device,
// in order to test the code in android/emulation/android_pipe.c.
// Usage is:
//  1) Create a new TestAndroidPipeDevice instance.
//
//  2) For each guest client you want to simulate, create a new
//     TestAndroidPipeDevice::Guest instance, then call its connect()
//     to try to connect with the corresponding host service. In case
//     of success, use its read() and write() methods.
//
class TestAndroidPipeDevice {
public:
    TestAndroidPipeDevice();
    ~TestAndroidPipeDevice();

    class Guest {
    public:
        ~Guest();

        // Create new Guest instance.
        static Guest* create();

        // Try to connect to a named service. |name| can be
        // either <service> or <service>:<args>. Returns 0 on
        // success, or -errno on failure.
        int connect(const char* name);

        // Try to read data from the host service. Return the
        // number of bytes transfered. 0 for EOF, and -errno for
        // i/o error.
        ssize_t read(void* buffer, size_t len);

        // Try to write data to the host service. Return the
        // number of bytes transfered. 0 for EOF, and -errno
        // for i/o error.
        ssize_t write(const void* buffer, size_t len);

        // Force-close the connection from the guest.
        void close();

        // Poll the current state of the guest connection.
        // Returns a combination of PIPE_POLL_IN and PIPE_POLL_OUT
        // flags.
        unsigned poll() const;

        // Return the AndroidPipe associated with this guest.
        void* getPipe() const;

    private:
        Guest();

        static void resetPipe(void* hwpipe, void* internal_pipe);
        static void closeFromHost(void* hwpipe);
        static void signalWake(void* hwpipe, unsigned flags);
        static int  getPipeId(void* hwpipe);

        const AndroidPipeHwFuncs* const vtblPtr;
        void* mPipe = nullptr;
        unsigned mWakes = 0;
        bool mClosed = true;

        static const AndroidPipeHwFuncs vtbl;
    };

private:
    TestVmLock mVmLock;
};

}  // namespace android

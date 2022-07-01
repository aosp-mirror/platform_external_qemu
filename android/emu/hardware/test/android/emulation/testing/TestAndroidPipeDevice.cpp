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

#include "android/emulation/testing/TestAndroidPipeDevice.h"

#include "android/emulation/AndroidPipe.h"
#include "android/base/Log.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#ifdef _MSC_VER
#ifdef ERROR
#undef ERROR
#endif
#endif

namespace android {

const AndroidPipeHwFuncs TestAndroidPipeDevice::Guest::vtbl = {
    &closeFromHost,
    &signalWake,
    &getPipeId,
};

TestAndroidPipeDevice::Guest::Guest() : vtblPtr(&vtbl) {
    mPipe = android_pipe_guest_open(this);
    if (!mPipe) {
        LOG(ERROR) << "Could not create new "
                      "TestAndroidPipeDevice::Guest instance!";
    }
}

TestAndroidPipeDevice::Guest::~Guest() {
    close();
}

int TestAndroidPipeDevice::Guest::connect(const char* name) {
    std::string handshake("pipe:");
    handshake += name;
    int len = static_cast<int>(handshake.size()) + 1;
    mClosed = false;
    int ret = write(handshake.c_str(), len);
    if (ret != len) {
        LOG(ERROR) << "Could not connect to service " << name
                   << " ret=" << ret << " expected len=" << len;
        mClosed = true;
        return -EINVAL;
    }
    return 0;
}

ssize_t TestAndroidPipeDevice::Guest::read(void* buffer, size_t len) {
    if (mClosed) {
        return 0;
    }
    AndroidPipeBuffer buf = { static_cast<uint8_t*>(buffer), len };
    return android_pipe_guest_recv(mPipe, &buf, 1);
}

ssize_t TestAndroidPipeDevice::Guest::write(const void* buffer, size_t len) {
    if (mClosed) {
        return 0;
    }
    AndroidPipeBuffer buf = { (uint8_t*)buffer, len };
    return android_pipe_guest_send(&mPipe, &buf, 1);
}

void TestAndroidPipeDevice::Guest::close() {
    if (!mClosed) {
        mClosed = true;
        android_pipe_guest_close(mPipe, PIPE_CLOSE_GRACEFUL);
    }
}

unsigned TestAndroidPipeDevice::Guest::poll() const {
    if (mClosed) {
        return PIPE_POLL_HUP;
    }
    return android_pipe_guest_poll(mPipe);
}

void TestAndroidPipeDevice::Guest::resetPipe(void* that_raw, void* internal_pipe) {
    auto that = static_cast<Guest *>(that_raw);
    that->mPipe = internal_pipe;
}

void TestAndroidPipeDevice::Guest::closeFromHost(void* that_raw) {
    auto that = static_cast<Guest *>(that_raw);
    that->mClosed = true;
}

void TestAndroidPipeDevice::Guest::signalWake(void* that_raw, unsigned wakes) {
    auto that = static_cast<Guest *>(that_raw);
    // NOTE: Update the flags, but for now don't do anything
    //       about them.
    that->mWakes |= wakes;
}

int TestAndroidPipeDevice::Guest::getPipeId(void* that_raw) {
    return -1;
}

TestAndroidPipeDevice::TestAndroidPipeDevice() {
    AndroidPipe::Service::resetAll();
    AndroidPipe::initThreading(&mVmLock);
    mVmLock.lock();
}

TestAndroidPipeDevice::~TestAndroidPipeDevice() {
    AndroidPipe::Service::resetAll();
    mVmLock.unlock();
}

// static
TestAndroidPipeDevice::Guest* TestAndroidPipeDevice::Guest::create() {
    return new Guest();
}

}  // namespace android

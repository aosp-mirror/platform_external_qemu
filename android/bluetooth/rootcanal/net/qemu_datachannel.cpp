// Copyright 2021 The Android Open Source Project
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
#include "net/qemu_datachannel.h"

#include <chrono>
#include <functional>  // for function
#include <thread>
#include "android/utils/debug.h"

//#define DEBUG 1
/* set  for very verbose debugging */
#ifndef DEBUG
#define DD(...) (void)1
#define DD_BUF(buf, len) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#define DD_BUF(buf, len)                                \
    do {                                                \
        dprint("QemuChannel %s (qemu):", __func__);     \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x])) {                 \
                dprint("%c", buf[x]);                   \
            } else {                                    \
                dprint("[0x%02x]", 0xff & (int)buf[x]); \
            }                                           \
        }                                               \
        dprint("\n");                                   \
    } while (0)

#endif

namespace android {
namespace net {

QemuDataChannel::QemuDataChannel(QAndroidHciAgent const* agent, Looper* looper)
    : mAgent(agent), mLooper(looper) {
    mAgent->registerDataAvailableCallback(this, [](void* opaque) {
        QemuDataChannel* pThis = reinterpret_cast<QemuDataChannel*>(opaque);
        pThis->ReadReady();
    });
}

ssize_t QemuDataChannel::Recv(uint8_t* buffer, uint64_t bufferSize) {
    if (!mOpen) {
        errno = EBADF;
        return 0;
    }
    ssize_t res = mAgent->recv(buffer, bufferSize);
    if (res < 0) {
        DD("Recv < 0: %s (qemu)", strerror(errno));
    }
    DD_BUF(buffer, res);
    DD("Recv: %zd bytes (qemu)", res);

    // Schedule another callback if we have more data available.
    if (mCallback && mAgent->available()) {
        ReadReady();
    }
    return res;
}

ssize_t QemuDataChannel::Send(const uint8_t* buffer, uint64_t bufferSize) {
    if (!mOpen) {
        errno = EBADF;
        return 0;
    }
    ssize_t res = mAgent->send(buffer, bufferSize);
    DD_BUF(buffer, res);
    DD("Send: %zd bytes (qemu)", res);
    return res;
}

bool QemuDataChannel::WatchForNonBlockingRead(
        const ReadCallback& on_read_ready_callback) {
    mCallback = on_read_ready_callback;

    // Notify listeners if we already have things!
    if (mCallback && mAgent->available()) {
        ReadReady();
    }
    return true;
}

void QemuDataChannel::StopWatching() {
    mCallback = nullptr;
}

using namespace std::chrono_literals;

void QemuDataChannel::ReadReady() {
    DD("ReadReady");
    if (mCallback) {
        mLooper->scheduleCallback([&]() { mCallback(this); });
    }
}

void QemuDataChannel::Close() {
    mOpen = false;
    ReadReady();
}

}  // namespace net
}  // namespace android

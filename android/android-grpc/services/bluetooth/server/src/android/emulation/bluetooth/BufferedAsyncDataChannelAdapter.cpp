// Copyright (C) 2022 The Android Open Source Project
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
#include "android/emulation/bluetooth/BufferedAsyncDataChannelAdapter.h"

#include <errno.h>    // for errno, EBADF, EAGAIN
#include <string.h>   // for memcpy
#include <algorithm>  // for min

#include "aemu/base/async/Looper.h"  // for Looper

#define DEBUG 0
/* set  >1 for very verbose debugging */
#if DEBUG <= 1
#define DD(...) (void)1
#define DD_BUF(buf, len) (void)0
#else
#define DD(fmt, ...) dinfo("(%p)" fmt, this, ##__VA_ARGS__)
#define DD_BUF(buf, len)                                             \
    do {                                                             \
        printf("BufferedAsyncDataChannel %s (%p):", __func__, this); \
        for (int x = 0; x < len; x++) {                              \
            if (isprint((int)buf[x])) {                              \
                printf("%c", buf[x]);                                \
            } else {                                                 \
                printf("[0x%02x]", 0xff & (int)buf[x]);              \
            }                                                        \
        }                                                            \
        printf("\n");                                                \
    } while (0)

#endif

namespace android {
namespace emulation {
namespace bluetooth {

BufferedAsyncDataChannel::BufferedAsyncDataChannel(size_t maxSize,
                                                   Looper* looper)
    : mMaxQueueSize(maxSize), mLooper(looper) {
    DD("BufferedAsyncDataChannel");
}

BufferedAsyncDataChannel::~BufferedAsyncDataChannel() {
    DD("~BufferedAsyncDataChannel");
}

ssize_t BufferedAsyncDataChannel::Recv(uint8_t* buffer, uint64_t bufferSize) {
    std::unique_lock<std::mutex> guard(mReadMutex);
    return readFromQueue(mReadQueue, buffer, bufferSize);
}

ssize_t BufferedAsyncDataChannel::ReadFromSendBuffer(uint8_t* buffer,
                                                     uint64_t bufferSize) {
    std::unique_lock<std::mutex> guard(mWriteMutex);
    return readFromQueue(mWriteQueue, buffer, bufferSize);
}

size_t BufferedAsyncDataChannel::availabeInReadBuffer() {
    std::unique_lock<std::mutex> guard(mReadMutex);
    return mReadQueue.size();
}

size_t BufferedAsyncDataChannel::availableInSendBuffer() {
    std::unique_lock<std::mutex> guard(mWriteMutex);
    return mWriteQueue.size();
}

size_t BufferedAsyncDataChannel::maxQueueSize() {
    return mMaxQueueSize;
}

// A trick to work around std::shared_ptr requirements.
struct PrivateBufferedAsyncDataChannel : BufferedAsyncDataChannel {
    PrivateBufferedAsyncDataChannel(size_t maxSize, Looper* looper)
        : BufferedAsyncDataChannel(maxSize, looper){};
};

std::shared_ptr<BufferedAsyncDataChannel> BufferedAsyncDataChannel::create(
        size_t maxSize,
        Looper* looper) {
    return std::make_shared<PrivateBufferedAsyncDataChannel>(maxSize, looper);
}

ssize_t BufferedAsyncDataChannel::Send(const uint8_t* buffer,
                                       uint64_t bufferSize) {
    std::unique_lock<std::mutex> guard(mWriteMutex);
    auto written = writeToQueue(mWriteQueue, buffer, bufferSize);
    if (mWriteCallback && written > 0) {
        DD("Scheduled callback for write.");
        mLooper->scheduleCallback([this, self = shared_from_this()]() {
            std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
            DD("Informing callback of bytes ready for send.");
            if (mWriteCallback)
                mWriteCallback(this);
        });
    }

    return written;
}

ssize_t BufferedAsyncDataChannel::WriteToRecvBuffer(const uint8_t* buffer,
                                                    uint64_t bufferSize) {
    std::unique_lock<std::mutex> guard(mReadMutex);
    auto written = writeToQueue(mReadQueue, buffer, bufferSize);
    if (mReadCallback && written > 0) {
        // See https://devblogs.microsoft.com/oldnewthing/20190104-00/?p=100635
        // for background on “capture a strong reference to yourself"
        mLooper->scheduleCallback([this, self = shared_from_this()]() {
            std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
            DD("Informing callback of bytes ready for recv.");
            if (mReadCallback) {
                DD("Sending.");
                mReadCallback(this);
            }
        });
    }

    return written;
}

bool BufferedAsyncDataChannel::WatchForNonBlockingSend(
        const SendCallback& on_send_ready_callback) {
    std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
    mWriteCallback = on_send_ready_callback;

    // Notify listeners if we already have things!
    if (mWriteCallback) {
        std::unique_lock<std::mutex> guard(mWriteMutex);
        if (!mWriteQueue.empty()) {
            mLooper->scheduleCallback([this, self = shared_from_this()]() {
                std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
                if (mWriteCallback)
                    mWriteCallback(this);
            });
        }
    }

    return true;
}

bool BufferedAsyncDataChannel::WatchForNonBlockingRead(
        const ReadCallback& on_read_ready_callback) {
    std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
    DD("Setting mReadCallback.");
    mReadCallback = on_read_ready_callback;

    // Notify listeners if we already have things!
    if (mReadCallback) {
        std::unique_lock<std::mutex> guard(mReadMutex);
        if (!mReadQueue.empty()) {
            mLooper->scheduleCallback([this, self = shared_from_this()]() {
                std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
                if (mReadCallback)
                    mReadCallback(this);
            });
        }
    }

    return true;
}

bool BufferedAsyncDataChannel::WatchForClose(
        const OnCloseCallback& on_close_callback) {
    std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
    mOnCloseCallback = on_close_callback;
    return true;
}

void BufferedAsyncDataChannel::StopWatching() {
    mReadCallback = nullptr;
}

void BufferedAsyncDataChannel::Close() {
    if (mOpen) {
        mOpen = false;
        mLooper->scheduleCallback([this, self = shared_from_this()]() {
            std::unique_lock<std::recursive_mutex> guard(mCallbackAccess);
            if (mReadCallback)
                mReadCallback(this);
            if (mOnCloseCallback)
                mOnCloseCallback(this);
        });
    }
}

ssize_t BufferedAsyncDataChannel::writeToQueue(std::vector<uint8_t>& queue,
                                               const uint8_t* buffer,
                                               uint64_t bufferSize) {
    errno = 0;
    if (!mOpen) {
        errno = EBADF;
        return 0;
    }
    if (queue.size() + bufferSize > mMaxQueueSize) {
        bufferSize = mMaxQueueSize - bufferSize;
    }

    queue.insert(queue.end(), buffer, buffer + bufferSize);

    DD_BUF(buffer, bufferSize);
    DD("Write: %zd bytes", bufferSize);
    return bufferSize;
}

ssize_t BufferedAsyncDataChannel::readFromQueue(std::vector<uint8_t>& queue,
                                                uint8_t* buffer,
                                                uint64_t bufferSize) {
    errno = 0;
    if (!mOpen) {
        errno = EBADF;
        return 0;
    }

    auto readCount = std::min<uint64_t>(queue.size(), bufferSize);
    memcpy(buffer, queue.data(), readCount);

    if (readCount == 0) {
        errno = EAGAIN;
        DD("Recv: %d, wanted: %d, errno %d", readCount, bufferSize, errno);
        return -1;
    } else {
        queue.erase(queue.begin(), queue.begin() + readCount);
    }

    DD("Recv: %d, wanted: %d, errno %d", readCount, bufferSize, errno);
    DD_BUF(buffer, readCount);
    return readCount;
}

}  // namespace bluetooth
}  // namespace emulation
}  // namespace android
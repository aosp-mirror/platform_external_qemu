// Copyright (C) 2019 The Android Open Source Project
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
#include "aemu/base/async/AsyncSocket.h"

#include <condition_variable>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>

#include "absl/log/log.h"

#include "aemu/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"

namespace android {
namespace base {

// This callback is called whenever an I/O event happens on the mSocket
// connecting the mSocket to the host ADB server.
static void socket_watcher(void* opaque, int fd, unsigned events) {
    const auto socket = static_cast<AsyncSocket*>(opaque);
    if (!socket) {
        return;
    }
    if ((events & Looper::FdWatch::kEventRead) != 0) {
        socket->onRead();
    } else if ((events & Looper::FdWatch::kEventWrite) != 0) {
        socket->onWrite();
    }
}

AsyncSocket::~AsyncSocket() {
    dispose();
    if (mConnectThread) {
        mConnecting = false;
        mConnectThread->join();
    }
}

AsyncSocket::AsyncSocket(Looper* looper, int port)
    : mLooper(looper), mPort(port), mWriteQueue(WRITE_BUFFER_SIZE, mWriteQueueLock) {}

AsyncSocket::AsyncSocket(Looper* looper, ScopedSocket socket)
    : mLooper(looper),
      mPort(-1),
      mSocket(std::move(socket)),
      mWriteQueue(WRITE_BUFFER_SIZE, mWriteQueueLock) {
    socketSetNonBlocking(mSocket.get());
    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mSocket.get(), socket_watcher, this));
}

void AsyncSocket::wantRead() {
    if (!mFdWatch) {
        return;
    }
    if (mLooper->onLooperThread()) {
        mFdWatch->wantRead();
        return;
    }
    mLooper->scheduleCallback([this] {
        if (mFdWatch) {
            mFdWatch->wantRead();
        }
    });
}

ssize_t AsyncSocket::recv(char* buffer, uint64_t bufferSize) {
    int fd = -1;
    {
        std::lock_guard<std::mutex> watchLock(mWatchLock);
        if (mFdWatch) {
            fd = mFdWatch->fd();
        }
        if (fd == -1) {
            return 0;
        }
        wantRead();
    }

    // It is still possible that the fd is no longer open
    ssize_t read = socketRecv(fd, buffer, bufferSize);
    if (read == 0) {
        // A read of 0, means the socket was closed, so clean up
        // everything properly.
        close();
        return 0;
    }
    return read;
}

void AsyncSocket::onRead() {
    if (!connected() || !mListener) {
        return;
    }
    std::lock_guard<std::recursive_mutex> lock(mListenerLock);
    mListener->onRead(this);
}

void AsyncSocket::wantWrite() {
    if (!mFdWatch) {
        return;
    }
    // Okay schedule left overs..
    if (mLooper->onLooperThread()) {
        mFdWatch->wantWrite();
    } else {
        mLooper->scheduleCallback([this] {
            if (mFdWatch) {
                mFdWatch->wantWrite();
            }
        });
    }
}

ssize_t AsyncSocket::send(const char* buffer, uint64_t bufferSize) {
    if (!mFdWatch) {
        errno = ECONNRESET;
        return -1;
    }
    {
        AutoLock alock(mWriteQueueLock);
        if (mWriteQueue.pushLocked(std::string(buffer, bufferSize)) != BufferQueueResult::Ok) {
            return 0;
        }
        mSendBuffer += bufferSize;
    }
    wantWrite();
    return bufferSize;
}

bool AsyncSocket::connected() {
    return mFdWatch.get() != nullptr;
}

void AsyncSocket::onWrite() {
    if (!connected()) {
        return;
    }
    auto delta = mAsyncWriter.written();
    auto status = mAsyncWriter.run();
    delta = mAsyncWriter.written() - delta;
    {
        std::unique_lock<std::mutex> lock(mSendBufferMutex);
        mSendBuffer -= delta;
        mSendBufferCv.notify_all();
    }
    switch (status) {
        case kAsyncCompleted:
            if (mWriteQueue.canPopLocked()) {
                AutoLock alock(mWriteQueueLock);
                if (mWriteQueue.popLocked(&mWriteBuffer) == BufferQueueResult::Ok) {
                    mAsyncWriter.reset(mWriteBuffer.data(), mWriteBuffer.size(), mFdWatch.get());
                }
            }
            return;
        case kAsyncError:
            return;
        case kAsyncAgain:
            return;
    }
}

void AsyncSocket::dispose() {
    // Only close if we are not actively closing.
    close();
    std::unique_lock<std::mutex> watchLock(mWatchLock);
    mWatchLockCv.wait(watchLock, [this] { return mFdWatch == nullptr; });
    setSocketEventListener(nullptr);
}

// Wait at most duration for the send buffer to be cleared
bool AsyncSocket::waitForSend(const std::chrono::milliseconds& rel_time) {
    std::unique_lock<std::mutex> lock(mSendBufferMutex);
    return mSendBufferCv.wait_for(lock, rel_time, [this] { return mSendBuffer == 0; });
}

void AsyncSocket::close() {
    if (!mFdWatch) {
        // Already closed
        return;
    }
    if (mLooper->onLooperThread()) {
        std::lock_guard<std::mutex> watchLock(mWatchLock);
        mFdWatch.reset();
        mSocket.close();
        mConnecting = false;
        if (mListener) {
            mListener->onClose(this, errno);
        }
        mWatchLockCv.notify_all();
    } else {
        mLooper->scheduleCallback([this] {
            std::lock_guard<std::mutex> watchLock(mWatchLock);
            if (!mFdWatch) {
                return;
            }
            mFdWatch.reset();
            mSocket.close();
            mConnecting = false;
            if (mListener) {
                mListener->onClose(this, errno);
            }
            mWatchLockCv.notify_all();
        });
    }
}

bool AsyncSocket::connect() {
    if (mConnecting) {
        return true;
    }

    mConnecting = true;
    mConnectThread = std::make_unique<std::thread>([this] { connectToPort(); });
    return mConnecting;
}

bool AsyncSocket::connectSync(std::chrono::milliseconds timeout) {
    if (connected()) return true;

    if (!this->connect()) return false;

    std::unique_lock<std::mutex> watchLock(mWatchLock);
    mWatchLockCv.wait_for(watchLock, timeout, [this]() { return connected(); });
    return connected();
}

void AsyncSocket::connectToPort() {
    int socket = 0;
    while (socket < 1 && mConnecting) {
        socket = socketTcp4LoopbackClient(mPort);
        if (socket < 0) {
            socket = socketTcp6LoopbackClient(mPort);
        }
        if (socket < 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    if (socket < 1) {
        return;
    }

    mSocket = socket;
    socketSetNonBlocking(mSocket.get());
    std::lock_guard<std::mutex> watchLock(mWatchLock);
    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mSocket.get(), socket_watcher, this));
    wantRead();

    // Now we have to notify the listener, if any
    std::mutex waitForNotification;
    std::unique_lock<std::mutex> waitForNotificationLock(waitForNotification);
    std::condition_variable waitForNotificationCv;
    mLooper->scheduleCallback([&] {
        if (mListener) {
            mListener->onConnected(this);
        }
        std::unique_lock<std::mutex> waitForNotificationLock(waitForNotification);
        waitForNotificationCv.notify_all();
    });

    // We now wait until the notification has been sent out.
    waitForNotificationCv.wait(waitForNotificationLock);
    mConnecting = false;
    mWatchLockCv.notify_all();
}

}  // namespace base
}  // namespace android

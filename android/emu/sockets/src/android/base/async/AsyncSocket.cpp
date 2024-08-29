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

#include <cstdio>
#include <mutex>
#include <thread>

#include "absl/log/log.h"

#include "aemu/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"

#define DEBUG 0

#if DEBUG >= 2
#define DD(str) LOG(INFO) << str
#else
#define DD(...) (void)0
#endif

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
}

AsyncSocket::AsyncSocket(Looper* looper, int port)
    : mLooper(looper), mPort(port), mWriteQueue(WRITE_BUFFER_SIZE, mWriteQueueLock) {}

AsyncSocket::AsyncSocket(Looper* looper, ScopedSocket socket)
    : mLooper(looper),
      mPort(-1),
      mWriteQueue(WRITE_BUFFER_SIZE, mWriteQueueLock),
      mSocket(std::move(socket)) {
    socketSetNonBlocking(mSocket.get());
    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mSocket.get(), socket_watcher, this));
}

void AsyncSocket::wantRead() {
    if (mFdWatch) {
        mFdWatch->wantRead();
    }
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

ssize_t AsyncSocket::send(const char* buffer, uint64_t bufferSize) {
    {
        AutoLock alock(mWriteQueueLock);
        if (mWriteQueue.pushLocked(std::string(buffer, bufferSize)) != BufferQueueResult::Ok) {
            return 0;
        }
    }
    {
        // Let's make sure we actually exists when requesting writes.
        std::lock_guard<std::mutex> watchLock(mWatchLock);
        if (mFdWatch) {
            mFdWatch->wantWrite();
        }
    }
    return bufferSize;
}

bool AsyncSocket::connected() {
    return mFdWatch.get() != nullptr;
}

void AsyncSocket::onWrite() {
    if (!connected()) {
        return;
    }
    auto status = mAsyncWriter.run();
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
    if (mFdWatch) {
        close();
    }
    setSocketEventListener(nullptr);
}

void AsyncSocket::close() {
    // Let's not accidentally trip a reader/writer up.
    bool fireClose;
    {
        std::lock_guard<std::mutex> watchLock(mWatchLock);
        mSocket.close();
        fireClose = mListener && mFdWatch != nullptr;
        mFdWatch = nullptr;
        mConnecting = false;
    }
    if (fireClose) {
        std::lock_guard<std::recursive_mutex> lock(mListenerLock);
        mListener->onClose(this, errno);
    }
}

bool AsyncSocket::connect() {
    if (mConnecting) {
        return true;
    }

    mConnecting = true;
    DD("Starting connect thread");
    mConnectThread.reset(new std::thread([this]() {
        connectToPort();
        return 0;
    }));
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
    DD("connecting to port " << mPort);
    while (socket < 1 && mConnecting) {
        socket = socketTcp4LoopbackClient(mPort);
        if (socket < 0) {
            socket = socketTcp6LoopbackClient(mPort);
        }
        if (socket < 0) {
            DD(<< "Failed to connect to: " << mPort << ", sleeping..");
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    if (socket < 1) {
        DD(<< "giving up..");
        return;
    }

    mSocket = socket;
    socketSetNonBlocking(mSocket.get());
    std::lock_guard<std::mutex> watchLock(mWatchLock);
    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mSocket.get(), socket_watcher, this));
    mFdWatch->wantRead();
    DD(<< "Connected to: " << mPort);
    if (mListener) {
        std::lock_guard<std::recursive_mutex> lock(mListenerLock);
        mListener->onConnected(this);
    }
    mConnecting = false;
    mWatchLockCv.notify_one();
}

}  // namespace base
}  // namespace android

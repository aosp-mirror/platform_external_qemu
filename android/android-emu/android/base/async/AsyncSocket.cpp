#include "android/base/async/AsyncSocket.h"

#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"

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

AsyncSocket::AsyncSocket(Looper* looper, int port)
    : mLooper(looper),
      mPort(port),
      mWriteQueue(WRITE_BUFFER_SIZE, mWriteQueueLock) {}

uint64_t AsyncSocket::recv(char* buffer, uint64_t bufferSize) {
    int fd = -1;
    {
        AutoLock watchLock(mWatchLock);
        if (mFdWatch) {
            fd = mFdWatch->fd();
        }
        if (fd == -1) {
            return 0;
        }
    }
    // It is still possible that the fd is no longer open
    int read = socketRecv(fd, buffer, bufferSize);
    if (read == 0) {
        // A read of 0, means the socket was closed, so clean up
        // everything properly.
        close();
    }
    return read;
}

void AsyncSocket::onRead() {
    if (!connected() || !mListener) {
        return;
    }
    mListener->onRead(this);
}

uint64_t AsyncSocket::send(const char* buffer, uint64_t bufferSize) {
    {
        AutoLock alock(mWriteQueueLock);
        if (mWriteQueue.pushLocked(std::string(buffer, bufferSize)) !=
            BufferQueueResult::Ok) {
            return 0;
        }
    }
    {
        // Let's make sure we actually exists when requesting writes.
        AutoLock watchLock(mWatchLock);
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
                if (mWriteQueue.popLocked(&mWriteBuffer) ==
                    BufferQueueResult::Ok) {
                    mAsyncWriter.reset(mWriteBuffer.data(), mWriteBuffer.size(),
                                       mFdWatch.get());
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
    close();
    setSocketEventListener(nullptr);
}

void AsyncSocket::close() {
    // Let's not accidentally trip a reader/writer up.
    AutoLock watchLock(mWatchLock);
    socketClose(mSocket);
    mFdWatch = nullptr;
    mConnecting = false;
}

bool AsyncSocket::connect() {
    if (mConnecting) {
        return true;
    }

    mConnecting = true;
    DD("Starting connect thread");
    mConnectThread.reset(new FunctorThread([this]() {
        connectToPort();
        return 0;
    }));
    return mConnectThread->start();
}

bool AsyncSocket::connectSync(uint64_t timeoutms) {
    if (connected())
        return true;

    if (!this->connect())
        return false;

    AutoLock watchLock(mWatchLock);
    auto waituntilus = System::get()->getHighResTimeUs() + timeoutms * 1000;
    while (!connected() && System::get()->getHighResTimeUs() < waituntilus) {
        mWatchLockCv.timedWait(&mWatchLock, waituntilus);
    }

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
            android::base::Thread::sleepMs(1000);
        }
    }
    if (socket < 1) {
        DD(<< "giving up..");
        return;
    }

    mSocket = socket;
    socketSetNonBlocking(mSocket);
    AutoLock watchLock(mWatchLock);
    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mSocket, socket_watcher, this));
    mFdWatch->wantRead();
    DD(<< "Connected to: " << mPort);
    if (mListener) {
        mListener->onConnected(this);
    }
    mConnecting = false;
    mWatchLockCv.signal();
}

}  // namespace base
}  // namespace android

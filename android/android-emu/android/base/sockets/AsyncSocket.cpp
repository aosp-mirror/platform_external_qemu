#include "android/base/sockets/AsyncSocket.h"

#include <ctype.h>

#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"

#if DEBUG >= 2
#define DD(fmt, ...)                                                  \
    do {                                                              \
        printf("AsyncSocket: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#define DD_BUF(buf, len)                                \
    do {                                                \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)
#else
#define DD(fmt, ...) (void)0
#define DD_BUF(...) (void)0
#endif

namespace android {
namespace base {

// This callback is called whenever an I/O event happens on the mSocket
// connecting the mSocket to the host ADB server.
static void socket_watcher(void* opaque, int fd, unsigned events) {
    const auto socket = static_cast<AsyncSocketFd*>(opaque);
    if (!socket) {
        return;
    }
    if ((events & Looper::FdWatch::kEventRead) != 0) {
        DD("fd: %d, read event.", fd, events);
        socket->onRead();
    } else if ((events & Looper::FdWatch::kEventWrite) != 0) {
        DD("fd: %d, write event.", fd, events);
        socket->onWrite();
    }
}

AsyncSocketFd::~AsyncSocketFd() {
    DD("~AsyncSocketFd: %p", this);
    removeAllEventListeners();
    close();
}

bool AsyncSocketFd::connect() {
    return true;
}

AsyncSocketFd::AsyncSocketFd(Looper* looper, int fd, SocketMode mode)
    : mLooper(looper),
      mMode(mode),
      mSocket(fd),
      mWriteQueue(WRITE_BUFFER_SIZE, mWriteQueueLock) {
    if (mSocket > 0) {
        socketSetNonBlocking(mSocket);
        mFdWatch = std::unique_ptr<Looper::FdWatch>(
                mLooper->createFdWatch(mSocket, socket_watcher, this));
        if ((mMode & SocketMode::Read) == SocketMode::Read) {
            mFdWatch->wantRead();
        }
    }
}

uint64_t AsyncSocketFd::recv(char* buffer, uint64_t bufferSize) {
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
    int read = socketRecv(fd, buffer, sizeof(buffer));
    if (read == 0) {
        // A read of 0, means the socket was closed, so clean up
        // everything properly.
        close();
    }
    DD("Read: %d <- len: %d, errno: %d", mSocket, read, errno);
    DD_BUF(buffer, read);
    return read;
}

void AsyncSocketFd::onRead() {
    notify(Notification::Read);
}

uint64_t AsyncSocketFd::send(const char* buffer, uint64_t bufferSize) {
    if ((mMode & SocketMode::Write) != SocketMode::Write) {
        LOG(ERROR) << "Writing on this socked is disabled";
        return;
    }
    {
        AutoLock alock(mWriteQueueLock);
        if (mWriteQueue.pushLocked(std::string(buffer, bufferSize)) !=
            BufferQueueResult::Ok) {
            DD("Dropping: %lu [%s]\n", bufferSize,
               std::string(buffer, bufferSize).c_str());
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

bool AsyncSocketFd::connected() {
    return mFdWatch.get() != nullptr;
}

void AsyncSocketFd::onWrite() {
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
                    DD("Write: %d <- len: %lu", mSocket, mWriteBuffer.size());
                    DD_BUF(mWriteBuffer.c_str(), mWriteBuffer.size());
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

void AsyncSocketFd::close() {
    DD("Closing %d", mSocket);
    // Let's not accidentally trip a reader/writer up.
    AutoLock watchLock(mWatchLock);
    socketClose(mSocket);
    mFdWatch = nullptr;
    mSocket = -1;
}

bool AsyncSocket::connect() {
    if (mSocket != -1 && mConnecting) {
        return true;
    }

    mConnecting = true;
    return mConnectThread.start();
}

void AsyncSocket::connectToPort() {
    int socket = 0;
    while (socket < 1 && mConnecting) {
        socket = socketTcp4LoopbackClient(mPort);
        if (socket < 0) {
            socket = socketTcp6LoopbackClient(mPort);
        }
        if (socket < 0) {
            LOG(INFO) << "Failed to connect to: " << mPort << ", sleeping..";
            android::base::Thread::sleepMs(1000);
        }
    }
    if (socket < 1) {
        LOG(INFO) << "giving up..";
        return;
    }

    mSocket = socket;
    socketSetNonBlocking(mSocket);
    AutoLock watchLock(mWatchLock);
    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mSocket, socket_watcher, this));
    mFdWatch->wantRead();
    LOG(INFO) << "Connected to: " << mPort;

    notify(Notification::Connected);
}

AsyncSocket::AsyncSocket(Looper* looper, int port)
    : AsyncSocketFd(looper, -1), mPort(port), mConnectThread([this]() {
          connectToPort();
          return 0;
      }) {}

}  // namespace base
}  // namespace android

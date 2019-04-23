#include "emulator/net/AsyncSocket.h"

#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"

namespace emulator {
namespace net {

using namespace android::base;

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

AsyncSocket::AsyncSocket(Looper* looper)
    : mLooper(looper), mWriteQueue(WRITE_BUFFER_SIZE, mLock) {}

uint64_t AsyncSocket::Recv(char* buffer, uint64_t bufferSize) {
    return socketRecv(mFdWatch->fd(), buffer, sizeof(buffer));
}

void AsyncSocket::onRead() {
    if (!connected() || !mListener) {
        return;
    }
    mListener->OnRead(this);
}

uint64_t AsyncSocket::Send(const char* buffer, uint64_t bufferSize) {
    {
        AutoLock alock(mLock);
        if (mWriteQueue.pushLocked(std::string(buffer, bufferSize)) !=
            BufferQueueResult::Ok) {
            return 0;
        }
    }
    mFdWatch->wantWrite();
    return bufferSize;
}

bool AsyncSocket::connected() const {
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
                AutoLock alock(mLock);
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
}  // namespace net
void AsyncSocket::AddSocketEventListener(AsyncSocketEventListener* listener) {
    mListener = listener;
}
void AsyncSocket::RemoveSocketEventListener(
        AsyncSocketEventListener* listener) {
    mListener = nullptr;
}
void AsyncSocket::Close() {
    socketClose(mSocket);
}

bool AsyncSocket::loopbackConnect(int port) {
    mSocket = socketTcp4LoopbackClient(port);
    if (mSocket < 0) {
        mSocket = socketTcp6LoopbackClient(port);
    }

    if (mSocket < 0) {
        return false;
    }

    socketSetNonBlocking(mSocket);

    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mSocket, socket_watcher, this));
    mFdWatch->wantRead();

    return true;
}

}  // namespace net
}  // namespace emulator

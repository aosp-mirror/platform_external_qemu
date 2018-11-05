#include "android/emulation/CrossSessionSocket.h"

#include "android/base/memory/LazyInstance.h"

#include <unordered_map>

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#include <sys/socket.h>
#endif

#define DEBUG 0

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#else
#define DD(...) (void)0
#endif

using android::emulation::CrossSessionSocket;
using SocketsMap = std::unordered_map<int, android::base::ScopedSocket>;

static android::base::LazyInstance<SocketsMap> sRecycledSockets
        = LAZY_INSTANCE_INIT;

CrossSessionSocket::CrossSessionSocket(
        CrossSessionSocket&& other)
    : mFdWatcher(std::move(other.mFdWatcher)),
      mSocket(std::move(other.mSocket)) {}

CrossSessionSocket& CrossSessionSocket::
operator=(CrossSessionSocket&& other) {
    mFdWatcher = std::move(other.mFdWatcher);
    mSocket = std::move(other.mSocket);
    return *this;
}

CrossSessionSocket::CrossSessionSocket(
        android::base::Looper::FdWatch* fdWatcher)
    : mFdWatcher(fdWatcher), mSocket(fdWatcher->fd()) {}

CrossSessionSocket::CrossSessionSocket(
        android::base::Looper::FdWatch* fdWatcher,
        android::base::ScopedSocket&& socket)
    : mFdWatcher(fdWatcher), mSocket(std::move(socket)) {}

void CrossSessionSocket::swapSocketAndClear(
        android::base::ScopedSocket* socket) {
    mFdWatcher.reset();
    mSocket.swap(socket);
    mSocket.reset(-1);
}

void CrossSessionSocket::reset() {
    mFdWatcher.reset();
    mSocket.reset(-1);
}

android::base::Looper::FdWatch*
CrossSessionSocket::fdWatcher() {
    return mFdWatcher.get();
}

const android::base::Looper::FdWatch*
CrossSessionSocket::fdWatcher() const {
    return mFdWatcher.get();
}

bool CrossSessionSocket::valid() const {
    return mSocket.valid();
}

void CrossSessionSocket::recycleSocket(CrossSessionSocket&& socketWatch) {
    if (!socketWatch.valid())
        return;
    int fd = socketWatch.fdWatcher()->fd();
    auto ite = sRecycledSockets->find(fd);
    D("%s: recycling %d: %d", __func__, fd, ite != sRecycledSockets->end());
    if (ite != sRecycledSockets->end()) {
        assert(!ite->second.valid());
        socketWatch.swapSocketAndClear(&ite->second);
    }
}

void CrossSessionSocket::registerForRecycle(int socket) {
    if (!sRecycledSockets->count(socket)) {
        sRecycledSockets->emplace(socket, -1);
    }
}

android::base::ScopedSocket CrossSessionSocket::reclaimSocket(int fd) {
    auto ite = sRecycledSockets->find(fd);
    if (ite == sRecycledSockets->end() || !ite->second.valid()) {
        return android::base::ScopedSocket();
    }
    return std::move(ite->second);
}

void CrossSessionSocket::drainSocket(DrainBehavior drainBehavior) {
    //android::base::Thread::sleepMs(50);
    int socket = mFdWatcher->fd();
    DD("%s: [%p] trying to drain recv socket", __func__, this);
    const int kBufferSize = 4096;
    // Set non-blocking socket
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(socket, FIONBIO, &mode);
    int recvFlags = 0;
#else
    int recvFlags = MSG_DONTWAIT;
#endif
    if (drainBehavior == DrainBehavior::AppendToBuffer) {
        if (!mRecvBuffer.size()) {
            mRecvBuffer.resize(kBufferSize);
        }
        DD("%s: [%p] appending to recv buffer", __func__, this);
        memmove((void*)mRecvBuffer.data(),
                (void*)(mRecvBuffer.data() + mRecvBufferBegin),
                mRecvBufferEnd - mRecvBufferBegin);
        mRecvBufferEnd -= mRecvBufferBegin;
        mRecvBufferBegin = 0;
        ssize_t recvSize = 0;
        do {
            recvSize =
                    ::recv(socket, (char*)mRecvBuffer.data() + mRecvBufferEnd,
                           mRecvBuffer.size() - mRecvBufferEnd, recvFlags);
            DD("%s: [%p] received %d bytes", __func__, this, (int)recvSize);
            if (recvSize <= 0) {
                DD("%s: [%p] recv errno %d", __func__, this, errno);
                break;
            }
            mRecvBufferEnd += recvSize;
            if (mRecvBuffer.size() == mRecvBufferEnd) {
                mRecvBuffer.resize(mRecvBuffer.size() + kBufferSize);
            }
        } while (1);
    } else {
        std::vector<char> localBuffer(kBufferSize);
        while (::recv(socket, localBuffer.data(), localBuffer.size(),
                      recvFlags) > 0) {
            DD("%s: [%p] clearing data from previous session", __func__, this);
        }
    }
#ifdef _WIN32
    // Set blocking socket
    mode = 0;
    ioctlsocket(socket, FIONBIO, &mode);
#endif
}

void CrossSessionSocket::onSave(android::base::Stream* stream) {
    stream->putBe32(mRecvBufferEnd - mRecvBufferBegin);
    stream->write(mRecvBuffer.data() + mRecvBufferBegin,
                  mRecvBufferEnd - mRecvBufferBegin);
}

void CrossSessionSocket::onLoad(android::base::Stream* stream) {
    mRecvBufferEnd = stream->getBe32();
    mRecvBufferBegin = 0;
    if (mRecvBuffer.size() < mRecvBufferEnd) {
        mRecvBuffer.resize(mRecvBufferEnd);
    }
    stream->read(mRecvBuffer.data(), mRecvBufferEnd);
}

bool CrossSessionSocket::hasStaleData() const {
    return mRecvBufferBegin != mRecvBufferEnd;
}

size_t CrossSessionSocket::readStaleData(void* data, size_t dataSize) {
    size_t len = std::min(dataSize, getReadableDataSize());
    memcpy(data, mRecvBuffer.data() + mRecvBufferBegin, len);
    mRecvBufferBegin += len;
    return len;
}

size_t CrossSessionSocket::getReadableDataSize() const {
    return mRecvBufferEnd - mRecvBufferBegin;
}



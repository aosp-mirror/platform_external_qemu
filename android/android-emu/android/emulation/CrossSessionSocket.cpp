// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/CrossSessionSocket.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"

#include <unordered_map>

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#include <sys/socket.h>
#endif

#define DEBUG 0

#if DEBUG >= 1
#include <stdio.h>
#define D(...) fprintf(fmt "\n", ##__VA_RAGS__)
#else
#define D(...) (void)0
#endif

#if DEBUG >= 2
#define DD(...) fprintf(fmt "\n", ##__VA_RAGS__)
#else
#define DD(...) (void)0
#endif

using SocketsMap = std::unordered_map<int, android::base::ScopedSocket>;

static android::base::LazyInstance<SocketsMap> sRecycledSockets
        = LAZY_INSTANCE_INIT;
static android::base::StaticLock sLock;

namespace android {
namespace emulation {

CrossSessionSocket::CrossSessionSocket(CrossSessionSocket&& other)
    : mSocket(std::move(other.mSocket)) {}

CrossSessionSocket& CrossSessionSocket::operator=(CrossSessionSocket&& other) {
    mSocket = std::move(other.mSocket);
    return *this;
}

CrossSessionSocket::CrossSessionSocket(
        int fd)
    : mSocket(fd) {}

CrossSessionSocket::CrossSessionSocket(
        android::base::ScopedSocket&& socket)
    : mSocket(std::move(socket)) {}

void CrossSessionSocket::reset() {
    mSocket.reset(-1);
}

bool CrossSessionSocket::valid() const {
    return mSocket.valid();
}

void CrossSessionSocket::recycleSocket(CrossSessionSocket&& socket) {
    if (!socket.valid())
        return;
    int fd = socket.mSocket.get();
    android::base::AutoLock lock(sLock);
    auto ite = sRecycledSockets->find(fd);
    D("%s: recycling %d: %d", __func__, fd, ite != sRecycledSockets->end());
    if (ite != sRecycledSockets->end()) {
        assert(!ite->second.valid());
        ite->second = std::move(socket.mSocket);
        socket.reset();
    }
}

void CrossSessionSocket::registerForRecycle(int socket) {
    android::base::AutoLock lock(sLock);
    if (!sRecycledSockets->count(socket)) {
        sRecycledSockets->emplace(socket, -1);
    }
}

CrossSessionSocket CrossSessionSocket::reclaimSocket(int fd) {
    android::base::AutoLock lock(sLock);
    auto ite = sRecycledSockets->find(fd);
    if (ite == sRecycledSockets->end() || !ite->second.valid()) {
        return android::base::ScopedSocket();
    }
    return CrossSessionSocket(std::move(ite->second));
}

void CrossSessionSocket::clearRecycleSockets() {
    android::base::AutoLock lock(sLock);
    sRecycledSockets->clear();
}

void CrossSessionSocket::drainSocket(DrainBehavior drainBehavior) {
    // It might need to wait a while for the server to reply.
    // Not sure where is the right place to sleep, though.
    //android::base::Thread::sleepMs(50);
    int socket = mSocket.get();
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
    D("%s: [%p] saved buffer size %d", __func__, this,
        mRecvBufferEnd - mRecvBufferBegin);
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
    D("%s: [%p] loaded buffer size %d", __func__, this,
        mRecvBufferEnd - mRecvBufferBegin);
}

int CrossSessionSocket::fd() const {
    return mSocket.get();
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

} // namespace emulation
} // namespace android

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

#include "WifiForwardPeer.h"

#include "android/base/async/ThreadLooper.h"
#include "android/base/EintrWrapper.h"
#include "android/base/misc/IpcPipe.h"
#include "android/base/sockets/SocketUtils.h"

#include <stdio.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <unistd.h>
#endif

namespace android {
namespace network {

static const size_t kReceiveBufferSize = 131072;
static const size_t kTransmitBufferSize = 131072;

WifiForwardPeer::WifiForwardPeer(OnDataAvailableCallback onDataAvailable) :
    mLooper(android::base::Looper::create()),
    mOnDataAvailable(onDataAvailable),
    mReceiveBuffer(kReceiveBufferSize),
    mTransmitBuffer(kTransmitBufferSize) {
}

bool WifiForwardPeer::init() {
    int fds[2];
    if (android::base::ipcPipeCreate(&fds[0], &fds[1]) != 0) {
        return false;
    }

    mWakePipeRead = android::base::ScopedFd(fds[0]);
    mWakePipeWrite = android::base::ScopedFd(fds[1]);

    mWakeFdWatch = mLooper->createFdWatch(mWakePipeRead.get(),
                                          &WifiForwardPeer::onWakePipe,
                                          this);
    mWakeFdWatch->wantRead();
    if (mFdWatch == nullptr) {
        return false;
    }
    return true;
}

bool WifiForwardPeer::initForTesting(android::base::Looper* looper, int fd) {
    mLooper = looper;
    if (!onConnect(fd))
        return false;
    return init();
}

void WifiForwardPeer::run() {
    if (mThread) {
        return;
    }
    auto loop = std::bind(&WifiForwardPeer::threadLoop, this);
    mThread.reset(new std::thread(loop));
}

void WifiForwardPeer::stop() {
    if (!mThread) {
        return;
    }
    sendWakeCommand(WakePipeCommand::Quit);
    mThread->join();
    mThread.reset();
}

const void* WifiForwardPeer::data() const {
    return mReceiveBuffer.data();
}
// The size of the available data from peers.
size_t WifiForwardPeer::size() const {
    return mAvailableDataFromPeers;
}

void WifiForwardPeer::clear() {
    mAvailableDataFromPeers = 0;
}

size_t WifiForwardPeer::queue(const uint8_t* data, size_t size) {
    std::unique_lock<std::mutex> lock(mTransmitBufferMutex);

    size_t bytesQueued = 0;
    if (mTransmitQueuePos < mTransmitSendPos) {
        // We're queueing things early in the buffer, the amount of space
        // equals the number of bytes up to the transmission position. Leave one
        // bytes to distinguish between an empty queue and a full queue.
        size_t availableSize = mTransmitSendPos - mTransmitQueuePos - 1;
        if (availableSize > 0) {
            bytesQueued = std::min(size, availableSize);
            memcpy(&mTransmitBuffer[mTransmitQueuePos], data, bytesQueued);
            mTransmitQueuePos += bytesQueued;
        }
    } else if (mTransmitQueuePos <= mTransmitBuffer.size()) {
        // Consider any bytes remaining at the end as well as any bytes
        // available at the beginning of the transmit buffer.
        size_t availableEnd = mTransmitBuffer.size() - mTransmitQueuePos;
        if (availableEnd > 0 && mTransmitSendPos == 0) {
            // If the transmission starts right at the beginning we need to
            // leave an empty byte at the end to distinguish an empty queue
            // from a full queue.
            --availableEnd;
        }
        if (availableEnd > 0) {
            bytesQueued = std::min(size, availableEnd);
            memcpy(&mTransmitBuffer[mTransmitQueuePos], data, bytesQueued);
            mTransmitQueuePos += bytesQueued;
            if (size > bytesQueued && mTransmitSendPos > 0) {
                // We need more space and there's room at the beginning. Leave
                // one byte to distinguish a full queue from an empty queue.
                size_t bytesToQueue = std::min(size - availableEnd,
                                               mTransmitSendPos - 1);
                if (bytesToQueue > 0) {
                    memcpy(&mTransmitBuffer[0],
                           data + availableEnd,
                           bytesToQueue);
                    mTransmitQueuePos = bytesToQueue;
                    bytesQueued = availableEnd + bytesToQueue;
                }
            } else if (mTransmitQueuePos >= mTransmitBuffer.size()) {
                // We managed to hit the end marker right on the spot, wrap around
                mTransmitQueuePos = 0;
            }
        }
    }
    if (mTransmitQueuePos != mTransmitSendPos) {
        lock.unlock();
        sendWakeCommand(WakePipeCommand::Send);
    }
    return bytesQueued;
}

size_t WifiForwardPeer::send(const void* data, size_t size) {
    if (mFdWatch == nullptr) {
        // There is no socket to send this on to, just dump and pretend
        // like we read it. This is not an error condition as there
        // might not yet be a connection between emulators.
        return size;
    }
    int fd = mFdWatch->fd();
    ssize_t sent = HANDLE_EINTR(android::base::socketSend(fd, data, size));
    if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // The call would block, indicate that we sent nothing. The caller
            // does not need to know about this detail, they'll just try again
            // later.
            return 0;
        }
        // Otherwise indicate disconnect
        onDisconnect();
        return 0;
    }
    return sent;
}

size_t WifiForwardPeer::receive(void* data, size_t size) {
    if (mFdWatch == nullptr) {
        // There is no socket to receive on, either the receiver was too slow
        // to call this and the watch got destroyed or someone called this for
        // no reason. Indicate that nothing was received, the receiver should
        // either have gotten an onDisconnect callback or stop misbehaving.
        return 0;
    }
    int fd = mFdWatch->fd();
    ssize_t received = HANDLE_EINTR(android::base::socketRecv(fd, data, size));
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // The receive call would block, indicate that we received nothing.
            // The caller does not need to know about this detail, they can try
            // again later.
            return 0;
        }
        // Otherwise indicate disconnect
        onDisconnect();
        return 0;
    }
    return received;
}

void WifiForwardPeer::threadLoop() {
    android::base::ThreadLooper::setLooper(mLooper);

    start();
    mLooper->run();
}

bool WifiForwardPeer::onConnect(int fd) {
    if (mFdWatch) {
        delete mFdWatch;
    }
    {
        // Clear the transmit buffer on a fresh connection.
        std::unique_lock<std::mutex> lock(mTransmitBufferMutex);
        mTransmitQueuePos = 0;
        mTransmitSendPos = 0;
    }
    mFdWatch = mLooper->createFdWatch(fd,
                                      &WifiForwardPeer::staticOnFdEvent,
                                      this);
    mFdWatch->wantRead();
    return true;
}

void WifiForwardPeer::onDisconnect() {
    if (mFdWatch) {
        android::base::socketClose(mFdWatch->fd());
        delete mFdWatch;
        mFdWatch = nullptr;
    }
    start();
}

void WifiForwardPeer::onFdEvent(int fd, unsigned events) {
    if ((events & android::base::Looper::FdWatch::kEventRead) == 0) {
        // Not interested in this event
        return;
    }
    // Data available, read it
    size_t size = mAvailableDataFromPeers;
    size += receive(mReceiveBuffer.data() + mAvailableDataFromPeers,
                    mReceiveBuffer.size() - mAvailableDataFromPeers);

    if (size > 0) {
        size_t consumed = mOnDataAvailable(mReceiveBuffer.data(), size);
        mAvailableDataFromPeers = size - consumed;
        memcpy(mReceiveBuffer.data(), mReceiveBuffer.data() + consumed,
               mAvailableDataFromPeers);
    }
}

void WifiForwardPeer::staticOnFdEvent(void *opaque, int fd, unsigned events) {
    auto peer = static_cast<WifiForwardPeer*>(opaque);
    peer->onFdEvent(fd, events);
}

void WifiForwardPeer::sendQueue() {
    std::unique_lock<std::mutex> lock(mTransmitBufferMutex);
    if (mTransmitQueuePos < mTransmitSendPos) {
        // Wrapped around, send end followed by beginning
        size_t bytesToSend = mTransmitBuffer.size() - mTransmitSendPos;
        size_t sent = send(&mTransmitBuffer[mTransmitSendPos], bytesToSend);
        mTransmitSendPos += sent;
        if (mTransmitSendPos >= mTransmitBuffer.size()) {
            // We sent enough data that we wrapped around
            mTransmitSendPos = 0;
            if (mTransmitQueuePos > 0) {
                sent = send(mTransmitBuffer.data(), mTransmitQueuePos);
                mTransmitSendPos = sent;
            }
        }
    } else if (mTransmitQueuePos > mTransmitSendPos) {
        // There is something left to send, send it
        size_t sent = send(&mTransmitBuffer[mTransmitSendPos],
                           mTransmitQueuePos - mTransmitSendPos);
        mTransmitSendPos += sent;
    }

    if (mTransmitSendPos != mTransmitQueuePos) {
        // There's still stuff left to send, we didn't send it all. Schedule
        // another transmission.
        lock.unlock();
        sendWakeCommand(WakePipeCommand::Send);
    }
}

void WifiForwardPeer::onWakePipe(void* opaque, int fd, unsigned events) {
    if ((events & android::base::Looper::FdWatch::kEventRead) == 0) {
        // Not interested in this event
        return;
    }
    // Drain the wake pipe
    uint8_t buffer[256];
    ssize_t received = android::base::ipcPipeRead(fd, buffer, sizeof(buffer));
    if (received <= 0) {
        return;
    }

    bool quit = false;
    bool send = false;
    for (ssize_t i = 0; !quit && i < received; ++i) {
        switch (static_cast<WakePipeCommand>(buffer[i])) {
            case WakePipeCommand::Quit:
                quit = true;
                break;
            case WakePipeCommand::Send:
                send = true;
                break;
        }
    }

    auto peer = static_cast<WifiForwardPeer*>(opaque);
    if (quit) {
        peer->mLooper->forceQuit();
    } else if (send) {
        peer->sendQueue();
    }
}

void WifiForwardPeer::sendWakeCommand(WakePipeCommand command) {
    if (mWakeFdWatch == nullptr) {
        return;
    }

    static_assert(sizeof(command) == sizeof(uint8_t), "command size mismatch");
    android::base::ipcPipeWrite(mWakePipeWrite.get(),
                                &command,
                                sizeof(command));
}

}  // namespace network
}  // namespace android


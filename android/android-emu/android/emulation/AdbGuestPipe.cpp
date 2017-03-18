// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AdbGuestPipe.h"

#include "android/base/async/AsyncSocketServer.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/StringView.h"
#include "android/emulation/VmLock.h"
#include "android/utils/debug.h"

#include <algorithm>
#include <string>

#include <assert.h>

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

#define E(...) fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n")

#define DINIT(...) do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

namespace android {
namespace emulation {

// Technical note: full state transition diagram
//
// State::WaitingForGuestAcceptCommand:
//   onGuestClose() -> State::ClosedByGuest
//   receive bad command bytes -> State::ClosedByHost
//   receive complete command -> State::WaitingForHostAdbConnection or
//                               State::SendingAcceptReplyOk
//   onHostConnection -> State::WaitingForGuestAcceptCommand (self)
//
// State::WaitingForHostAdbConnection:
//   onGuestClose -> State::ClosedByGuest
//   onHostConnection -> State::SendingAcceptReplyOk
//
// State::SendingAcceptReplyOk:
//   onGuestClose -> State::ClosedByGuest
//   all bytes sent -> State::WaitingForGuestStartCommand
//
// State::WaitingForGuestStartCommand:
//   onGuestClose -> State::ClosedByGuest
//   receive bad command bytes -> State::ClosedByHost
//   receive complete command -> State::ProxyingData
//
// State::ProxyingData:
//   onGuestClose -> State::ClosedByGuest
//   on host disconnect -> State::ClosedByHost
//   data in or out -> State;:ProxyingData (self)
//
// State::ClosedByHost:
//   onGuestClose -> State::ClosedByGuest
//
// State::ClosedByGuest:
//   close host socket ->  State::ClosedByGuest
//

using FdWatch = android::base::Looper::FdWatch;
using android::base::ScopedSocketWatch;
using android::base::StringView;

#if DEBUG >= 2
static int bufferBytes(const AndroidPipeBuffer* buffers, int count) {
    int result = 0;
    for (int n = 0; n < count; ++n) {
        result += buffers[n].size;
    }
    return result;
}
#endif

AndroidPipe* AdbGuestPipe::Service::create(void* mHwPipe, const char* args) {
    auto pipe = new AdbGuestPipe(mHwPipe, this, mHostAgent);
    onPipeOpen(pipe);
    DD("%s: [%p] created", __func__, pipe);
    return pipe;
}

void AdbGuestPipe::Service::onHostConnection(ScopedSocket&& socket) {
    // There must be no active pipe yet, but at least one waiting
    // for activation in mPipes.
    CHECK(mActivePipe != nullptr);
    // We have one connection from adb sever, stop listening for now.
    mHostAgent->stopListening();
    mActivePipe->onHostConnection(std::move(socket));
}

void AdbGuestPipe::Service::onPipeOpen(AdbGuestPipe* pipe) {
    mPipes.push_back(pipe);
}

void AdbGuestPipe::Service::onPipeClose(AdbGuestPipe* pipe) {
    mPipes.erase(std::remove(mPipes.begin(), mPipes.end(), pipe), mPipes.end());
    if (mActivePipe == pipe) {
        mActivePipe = nullptr;
        mHostAgent->stopListening();
        searchForActivePipe();
    }
    delete pipe;
}

void AdbGuestPipe::Service::searchForActivePipe() {
    if (!mActivePipe) {
        for (auto pipe : mPipes) {
            if (pipe->mState == State::WaitingForHostAdbConnection) {
                mActivePipe = pipe;
                // Tell the agent to start listening again.
                mHostAgent->startListening();

                // Also notify the server that an emulator instance is waiting
                // for a connection. This is useful when the ADB Server was
                // restarted on the host, but could not see the current emulator
                // instance because it's listening on a 'non-standard' ADB port.
                mHostAgent->notifyServer();
                break;
            }
        }
    }
}

AdbGuestPipe::~AdbGuestPipe() {
    DD("%s: [%p] destroyed", __func__, this);
    CHECK(mState == State::ClosedByGuest);
}

void AdbGuestPipe::onGuestClose() {
    DD("%s: [%p]", __func__, this);
    mState = State::ClosedByGuest;
    DINIT("%s: [%p] Adb closed by guest",__func__, this);
    mHostSocket.reset();
    service()->onPipeClose(this);  // This deletes the instance.
}

unsigned AdbGuestPipe::onGuestPoll() const {
    DD("%s: [%p]", __func__, this);
    unsigned result = 0;
    switch (mState) {
        case State::WaitingForGuestAcceptCommand:
        case State::WaitingForGuestStartCommand:
            result = PIPE_POLL_OUT;
            break;

        case State::SendingAcceptReplyOk:
            result = PIPE_POLL_IN;
            break;

        case State::ProxyingData: {
            unsigned flags = mHostSocket->poll();
            if (flags & FdWatch::kEventRead) {
                result |= PIPE_POLL_IN;
            }
            if (flags & FdWatch::kEventWrite) {
                result |= PIPE_POLL_OUT;
            }
            break;
        }

        case State::ClosedByHost:
            result |= PIPE_POLL_HUP;
            break;

        default:;
    }
    return result;
}

int AdbGuestPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d state=%s", __func__, this, numBuffers,
       bufferBytes(buffers, numBuffers), toString(mState));
    if (mState == State::ProxyingData) {
        // Common case, proxy-ing the data from the host to the guest.
        return onGuestRecvData(buffers, numBuffers);
    } else if (mState == State::SendingAcceptReplyOk) {
        // The guest is receiving the 'ok' reply.
        return onGuestRecvReply(buffers, numBuffers);
    } else if (mState == State::WaitingForHostAdbConnection ||
               mState == State::WaitingForGuestStartCommand) {
        // Not ready yet to send data to the guest.
        return PIPE_ERROR_AGAIN;
    } else {
        if (mState != State::ClosedByHost) {
            // Invalid state !!!
            LOG(ERROR) << "Invalid state: " << toString(mState);
        }
        return PIPE_ERROR_IO;
    }
}

int AdbGuestPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d state=%s", __func__, this, numBuffers,
       bufferBytes(buffers, numBuffers), toString(mState));
    if (mState == State::ProxyingData) {
        // Common-case, proxy-ing the data from the guest to the host.
        return onGuestSendData(buffers, numBuffers);
    } else if (mState == State::WaitingForGuestAcceptCommand ||
               mState == State::WaitingForGuestStartCommand) {
        // Waiting command bytes from the guest.
        return onGuestSendCommand(buffers, numBuffers);
    } else {
        if (mState != State::ClosedByHost) {
            // Invalid state !!!
            LOG(ERROR) << "Invalid state: " << toString(mState);
        }
        return PIPE_ERROR_IO;
    }
}

void AdbGuestPipe::onGuestWantWakeOn(int flags) {
    if (flags & PIPE_WAKE_READ) {
        if (mHostSocket.get()) {
            mHostSocket->wantRead();
        }
    }
    if (flags & PIPE_WAKE_WRITE) {
        if (mHostSocket.get()) {
            mHostSocket->wantWrite();
        }
    }
}

void AdbGuestPipe::onHostConnection(ScopedSocket&& socket) {
    DD("%s: [%p] host connection", __func__, this);
    CHECK(mState <= State::WaitingForHostAdbConnection);
    android::base::socketSetNonBlocking(socket.get());
    // socketSetNoDelay() reduces the latency of sending data, at the cost
    // of creating more TCP packets on the connection. It's useful when
    // doing lots of small send() calls, like the ADB protocol requires.
    // And since this is on localhost, the packet increase should not be
    // noticeable.
    android::base::socketSetNoDelay(socket.get());

    mHostSocket.reset(android::base::ThreadLooper::get()->createFdWatch(
            socket.release(),
            [](void* opaque, int fd, unsigned events) {
                static_cast<AdbGuestPipe*>(opaque)->onHostSocketEvent(events);
            },
            this));

    waitForHostConnection();
}

// static
const char* AdbGuestPipe::toString(AdbGuestPipe::State state) {
    switch (state) {
        case State::WaitingForGuestAcceptCommand:
            return "WaitingForGuestAcceptCommand";
        case State::WaitingForHostAdbConnection:
            return "WaitingForHostAdbConnection";
        case State::SendingAcceptReplyOk:
            return "SendingAcceptReplyOk";
        case State::WaitingForGuestStartCommand:
            return "WaitingForGuestStartCommand";
        case State::ProxyingData:
            return "ProxyingData";
        case State::ClosedByGuest:
            return "ClosedByGuest";
        case State::ClosedByHost:
            return "ClosedByHost";
        default:
            return "Unknown";
    }
}

// Called whenever an i/o event occurs on the host socket.
void AdbGuestPipe::onHostSocketEvent(unsigned events) {
    int wakeFlags = 0;
    if ((events & FdWatch::kEventRead) != 0) {
        wakeFlags |= PIPE_WAKE_READ;
        mHostSocket->dontWantRead();
    }
    if ((events & FdWatch::kEventWrite) != 0) {
        wakeFlags |= PIPE_WAKE_WRITE;
        mHostSocket->dontWantWrite();
    }
    if (wakeFlags) {
        signalWake(wakeFlags);
    }
}

int AdbGuestPipe::onGuestRecvData(AndroidPipeBuffer* buffers, int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d", __func__, this, numBuffers,
        bufferBytes(buffers, numBuffers));
    CHECK(mState == State::ProxyingData);
    int result = 0;
    while (numBuffers > 0) {
        uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        DD("%s: [%p] dataSize=%d", __func__, this, (int)dataSize);
        while (dataSize > 0) {
            ssize_t len;
            {
                ScopedVmUnlock unlockBql;
                len = android::base::socketRecv(mHostSocket->fd(),
                                                data, dataSize);
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // got less than requested -> no more data.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    mHostSocket->dontWantRead();
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    mHostSocket.reset();
                    mState = State::ClosedByHost;
                    DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            return result;
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

int AdbGuestPipe::onGuestSendData(const AndroidPipeBuffer* buffers,
                                  int numBuffers) {
    DD("%s: [%p] numBuffers=%d bytes=%d", __func__, this, numBuffers,
        bufferBytes(buffers, numBuffers));
    CHECK(mState == State::ProxyingData);
    int result = 0;
    while (numBuffers > 0) {
        const uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            ssize_t len;
            {
                ScopedVmUnlock unlockBql;
                len = android::base::socketSend(mHostSocket->fd(),
                                                data, dataSize);
            }
            if (len > 0) {
                data += len;
                dataSize -= len;
                result += static_cast<int>(len);
                if (dataSize != 0) {
                    return result; // sent less than requested -> no more room.
                } else {
                    break;
                }
            }
            if (len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                if (result == 0) {
                    mHostSocket->dontWantWrite();
                    return PIPE_ERROR_AGAIN;
                }
            } else {
                // End of stream or i/o error means the guest has closed
                // the connection.
                if (result == 0) {
                    mHostSocket.reset();
                    mState = State::ClosedByHost;
                    DINIT("%s: [%p] Adb closed by host",__func__, this);
                    return PIPE_ERROR_IO;
                }
            }
            // Some data was received so report it, the guest will have
            // to try again to get an error.
            return result;
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

int AdbGuestPipe::onGuestRecvReply(AndroidPipeBuffer* buffers, int numBuffers) {
    // Sending reply to the guest.
    CHECK(mState == State::SendingAcceptReplyOk);
    DD("%s: [%p] numBuffers=%d", __func__, this, numBuffers);
    int result = 0;
    while (numBuffers > 0) {
        uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            size_t avail = std::min(mBufferSize - mBufferPos, dataSize);
            memcpy(data, mBuffer + mBufferPos, avail);
            data += avail;
            dataSize -= avail;
            result += static_cast<int>(avail);
            mBufferPos += avail;
            if (mBufferPos == mBufferSize) {
                // Full reply sent, now wait for the guest 'start' command.
                setExpectedGuestCommand("start",
                                        State::WaitingForGuestStartCommand);
                return result;
            }
        }
        buffers++;
        numBuffers--;
    }
    return result;
}

int AdbGuestPipe::onGuestSendCommand(const AndroidPipeBuffer* buffers,
                                     int numBuffers) {
    DD("%s: [%p] numBuffers=%d", __func__, this, numBuffers);
    CHECK(mState == State::WaitingForGuestAcceptCommand ||
          mState == State::WaitingForGuestStartCommand);
    // Waiting for a command from the guest. Just match the bytes
    // with the expected command. Any mismatch triggers an I/O error
    // which will force the guest to close the connection.
    int result = 0;
    while (numBuffers > 0) {
        const char* data = reinterpret_cast<const char*>(buffers[0].data);
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            size_t avail = std::min(mBufferSize - mBufferPos, dataSize);
            if (memcmp(data, mBuffer + mBufferPos, avail) != 0) {
                // Mismatched, this is not what the pipe is expecting.
                // Closing the connection now is easier than sending 'ko'.
                DD("%s: [%p] mismatched command", __func__, this);
                mHostSocket.reset();
                return PIPE_ERROR_IO;
            }
            data += avail;
            dataSize -= avail;
            result += static_cast<int>(avail);
            mBufferPos += avail;
            if (mBufferPos == mBufferSize) {
                // Expected command was received.
                DD("%s: [%p] command validated", __func__, this);
                if (mState == State::WaitingForGuestAcceptCommand) {
                    waitForHostConnection();
                } else if (mState == State::WaitingForGuestStartCommand) {
                    // Proxying data can start right now.
                    mState = State::ProxyingData;
                    // when -verbose, print a message indicating adb is connected
                    DINIT("%s: [%p] Adb connected, start proxing data",__func__, this);
                }
                return result;
            }
        }
        numBuffers--;
        buffers++;
    }
    return result;
}

void AdbGuestPipe::setReply(StringView reply, State newState) {
    CHECK(newState == State::SendingAcceptReplyOk);
    CHECK(reply.size() <= sizeof(mBuffer));
    memcpy(mBuffer, reply.c_str(), reply.size());
    mBufferSize = reply.size();
    mBufferPos = 0;
    mState = newState;
}

void AdbGuestPipe::setExpectedGuestCommand(StringView command, State newState) {
    CHECK(newState == State::WaitingForGuestAcceptCommand ||
          newState == State::WaitingForGuestStartCommand);
    CHECK(command.size() <= sizeof(mBuffer));
    memcpy(mBuffer, command.c_str(), command.size());
    mBufferSize = command.size();
    mBufferPos = 0;
    mState = newState;
}

void AdbGuestPipe::waitForHostConnection() {
    if (mHostSocket.get()) {
        // A host connection already exists! Send the 'ok' reply back to
        // the guest.
        DD("%s: [%p] sending reply", __func__, this);
        setReply("ok", State::SendingAcceptReplyOk);
        signalWake(PIPE_WAKE_READ);
    } else {
        // No host connection yet. The guest is still waiting for the 'ok'
        // reply.
        DD("%s: [%p] waiting for host connection", __func__, this);
        mState = State::WaitingForHostAdbConnection;
        service()->searchForActivePipe();
    }
}

}  // namespace emulation
}  // namespace android

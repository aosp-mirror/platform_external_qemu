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

#include "android/base/Log.h"
#include "android/base/StringView.h"
#include "android/base/async/AsyncSocketServer.h"
#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/Async.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/VmLock.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/utils/debug.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <iostream>

#include <assert.h>

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

#define E(...) fprintf(stderr, "ERROR:" __VA_ARGS__), fprintf(stderr, "\n")

#define DINIT(...) do { if (DEBUG || VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

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
// How multiple active pipes are supported
// First of all, the guest adbd is driving the connection.
// in the guest, adbd is running the following loop (not exact code)
/*
   for (;;) {
        open_adb_pipe();//this will tell emulator-5554 (for example) to listen
                        //at 5555 (i.e., 5554+1) for adb server connection

        wait_for_ok_replay();//emulator sends "OK" when the pipe is connected with adb server

        register_the_connected_pipe();

        open_another_adb_pipe();//previously, emulator will not listen again, as there is
                                //one activepipe already; now it will just listen again
                                //now this new pipe will trigger emulator to listen again,
        wait_for_ok_replay();//emulator sends "OK" when the pipe is connected with adb server
        register_the_connected_pipe();
   }
*/

// The Service object has a vector of pipes |mPipes| waiting
// for host connection, in FIFO mode.
// Whenever there is a connection from host, Service
// will remove the first pipe in the queue and wait
// for another new connection. Each connected pipe runs
// independently from each other once connected by
// Service.
//
// Note that Service runs in the main-loop thread
// and AdbGuestPipe runs in other cpu threads;
// on the guest side, it always starts another pipe
// after the current one is connected with host side
// already: there is no race condition there.
//
// Previously, the new pipe has to wait for the existing
// active pipe to disconnect before becoming active;
// now, it only need to wait for the current active pipe to
// become fully connected. Once that happens, the new
// pipe is ready to connect with host.
//
// One example might help explain better:
// Assume there is only one emulator-5554.
//
// Suppose host adb server is running at 5037, and it tries to
// connect to 5555 port, but before there is any pipe,
// the 5555 port is not listening yet. When guest adbd
// starts, a new pipe is created and it sends "accept"
// to emulator, upon receiving "accept", emulator starts
// listening at 5555, and accept a socket from adb server
// running at 5037. And the first pipe is connected now. Once
// guest side adbd receives the "ok" from host ("ok" means
// fully connected), it starts another new pipe, this new pipe
// will cause emulator to listen again for adb server connection
// (previously, it wont listen again because there is one
//  active pipe running already).
//
// Assume there is another adb server running at 5038
// (previously, it cannot connect to 5555 port, because there
// is an active pipe running already, and emualtor wll stop
// listening at 5555 because of that), it
// will connect to this new pipe right away. Once the new pipe
// is connected, guest adbd will start another one immediately
// --there is no limit on how many pipes the guest adbd can start
// (of course, it may run out of fds)
//
// This way, multiple active pipes can run without affecting
// each other.

using FdWatch = android::base::Looper::FdWatch;
using android::base::AutoLock;
using android::base::LazyInstance;
using android::base::Lock;
using android::base::ScopedSocketWatch;
using android::base::StringView;

AndroidPipe* AdbGuestPipe::Service::create(void* mHwPipe, const char* args) {
    auto pipe = new AdbGuestPipe(mHwPipe, this, mHostAgent, nullptr);
    onPipeOpen(pipe);
    D("%s: [%p] created, %d pipes", __func__, pipe, (int)mPipes.size());
    return pipe;
}

bool AdbGuestPipe::Service::canLoad() const {
    return true;
}

AndroidPipe* AdbGuestPipe::Service::load(void* hwPipe,
                                         const char* args,
                                         android::base::Stream* stream) {
    auto pipe = new AdbGuestPipe(hwPipe, this, mHostAgent, stream);
    if (pipe->mState == State::ClosedByHost) {
        delete pipe;
        pipe = nullptr;
    } else if (pipe->mReuseFromSnapshot) {
        D("loaded pending pipe\n");
        mPipes.push_back(pipe);
    }
    D("%s: [%p] loaded", __func__, pipe);

    return pipe;
}

void AdbGuestPipe::Service::removeAdbGuestPipe(AdbGuestPipe* pipe) {
    mPipes.erase(std::remove(mPipes.begin(), mPipes.end(), pipe), mPipes.end());
}

void AdbGuestPipe::Service::onHostConnection(ScopedSocket&& socket,
                                             AdbPortType portType) {
    D("%s", __func__);
    AdbGuestPipe* activePipe = searchForActivePipe();
    CHECK(activePipe != nullptr);
    if (!hasActivePipe()) {
        mHostAgent->stopListening();
    }
    activePipe->onHostConnection(std::move(socket), portType);
}

void AdbGuestPipe::Service::preLoad(android::base::Stream* stream) {
    mCurrentActivePipe = nullptr;
    mPipes.clear();
}

void AdbGuestPipe::Service::postLoad(android::base::Stream* stream) {
    int activeFd = stream ? stream->getBe32() : 0;
    if (activeFd == 0) {
        mCurrentActivePipe = nullptr;
    } else {
        for (const auto& pipe : mPipes) {
            if (pipe && pipe->mHostSocket.valid() &&
                pipe->mFdWatcher &&
                pipe->mHostSocket.fd() == activeFd) {
                mCurrentActivePipe = pipe;
                break;
            }
        }
    }
}

void AdbGuestPipe::Service::postSave(android::base::Stream* stream) {
    DD("%s num. of pipes %d", __func__, (int)mPipes.size());
    if (mCurrentActivePipe && mCurrentActivePipe->mHostSocket.valid()) {
        stream->putBe32(mCurrentActivePipe->mHostSocket.fd());
    } else {
        stream->putBe32(0);
    }
    // TODO: handle mCurrentActivePipe and mPipes more appropriately in snapshot
}

void AdbGuestPipe::Service::onPipeOpen(AdbGuestPipe* pipe) {
    mPipes.push_back(pipe);
}

void AdbGuestPipe::Service::onPipeClose(AdbGuestPipe* pipe) {
    removeAdbGuestPipe(pipe);
    if (mPipes.empty() && mRecycledSockets.empty()) {
        mHostAgent->stopListening();
    }
    delete pipe;
}

AdbGuestPipe* AdbGuestPipe::Service::searchForActivePipe() {
    // Prefer reused pipes over new pipes
    auto pipeIt = std::find_if(
            mPipes.begin(), mPipes.end(), [](const AdbGuestPipe* pipe) {
                return pipe->mReuseFromSnapshot &&
                       pipe->mState == State::WaitingForHostAdbConnection;
            });
    if (pipeIt == mPipes.end()) {
        pipeIt = std::find_if(
                mPipes.begin(), mPipes.end(), [](const AdbGuestPipe* pipe) {
                    return pipe->mState == State::WaitingForHostAdbConnection;
                });
    }
    if (pipeIt != mPipes.end()) {
        AdbGuestPipe* activePipe = *pipeIt;
        removeAdbGuestPipe(activePipe);
        mCurrentActivePipe = activePipe;
        return activePipe;
    }
    return nullptr;
}

bool AdbGuestPipe::Service::hasActivePipe() const {
    return mPipes.end() !=
           std::find_if(mPipes.begin(), mPipes.end(),
                        [](const AdbGuestPipe* pipe) {
                            return pipe->mState ==
                                   State::WaitingForHostAdbConnection;
                        });
}

void AdbGuestPipe::Service::resetActiveGuestPipeConnection() {
    if (mCurrentActivePipe && mCurrentActivePipe->isProxyingData()) {
        mCurrentActivePipe->resetConnection();
        mCurrentActivePipe = nullptr;
    }
}

void AdbGuestPipe::Service::unregisterActivePipe(AdbGuestPipe* pipe) {
    if (mCurrentActivePipe == pipe) {
        mCurrentActivePipe = nullptr;
    }
}

void AdbGuestPipe::Service::hostCloseSocket(int fd) {
    DD("%s: [%p]", __func__, this);
    assert(!mRecycledSockets.count(fd) || !mRecycledSockets[fd].valid());
    mRecycledSockets.erase(fd);
}

AdbGuestPipe::AdbGuestPipe(void* mHwPipe,
                           Service* service,
                           AdbHostAgent* hostAgent,
                           android::base::Stream* stream)
    : AndroidPipe(mHwPipe, service), mHostAgent(hostAgent),
    mReceivedMesg(AdbMessageSniffer::create("HOST==>GUEST",  android_hw->test_monitorAdb, &std::cout)),
    mSendingMesg(AdbMessageSniffer::create("HOST<==GUEST",  android_hw->test_monitorAdb, &std::cout)) {
    mPlayStoreImage = android::featurecontrol::isEnabled(
            android::featurecontrol::PlayStoreImage);
    if (!stream) {
        setExpectedGuestCommand("accept", State::WaitingForGuestAcceptCommand);
    } else {
        onLoad(stream);
    }
}

void AdbGuestPipe::onLoad(android::base::Stream* stream) {
    stream->read(mBuffer, sizeof(mBuffer));
    mBufferSize = stream->getBe32();
    mBufferPos = stream->getBe32();
    mState = static_cast<State>(stream->getBe32());
    int socket = stream->getBe32();
    if (socket) {
        const bool needLoadBuffer = stream->getByte();
        assert(shouldUseRecvBuffer() == needLoadBuffer);
        DD("%s: [%p] load socket %d", __func__, this, socket);
        // We do not register sockets that are in earlier states
        if (shouldUseRecvBuffer()) {
            mHostSocket = CrossSessionSocket::reclaimSocket(socket);
            if (needLoadBuffer) {
                mHostSocket.onLoad(stream);
            }
            if (mHostSocket.valid()) {
                mFdWatcher.reset(
                        android::base::ThreadLooper::get()->createFdWatch(
                                socket,
                                [](void* opaque, int fd, unsigned events) {
                                    static_cast<AdbGuestPipe*>(opaque)
                                            ->onHostSocketEvent(events);
                                },
                                this));
                assert(mFdWatcher);
                mHostSocket.drainSocket(
                        CrossSessionSocket::DrainBehavior::Clear);
                DD("%s: [%p] poll %d", __func__, this,
                mFdWatcher->poll());
                mFdWatcher->wantRead();
                mFdWatcher->wantWrite();
                if (mHostSocket.hasStaleData()) {
                    signalWake(PIPE_WAKE_READ);
                }
            }
        }
    } else {
        mReuseFromSnapshot = true;
        mState = State::WaitingForHostAdbConnection;
    }
    int proxyCount = stream->getBe32();
    D("Loading %d jdwp proxies", proxyCount);
    if (!socket && proxyCount <= 0) {
        // Regular adb pipe, immediately close it after load
        mReuseFromSnapshot = false;
        mState = State::ClosedByHost;
    }
    if (proxyCount >= 0) {
        mAdbHub.reset(new AdbHub());
        mAdbHub->onLoad(stream);
    }
}

void AdbGuestPipe::onSave(android::base::Stream* stream) {
    if (shouldUseRecvBuffer()) {
        mHostSocket.drainSocket(
                CrossSessionSocket::DrainBehavior::AppendToBuffer);
    }
    stream->write(mBuffer, sizeof(mBuffer));
    stream->putBe32(mBufferSize);
    stream->putBe32(mBufferPos);
    stream->putBe32(static_cast<uint32_t>(mState));
    bool saveHostSocket =
            android::featurecontrol::isEnabled(
                    android::featurecontrol::Feature::SnapshotAdb) &&
            mHostSocket.valid();
    if (saveHostSocket) {
        stream->putBe32(mHostSocket.fd());
        bool needSaveBuffer = shouldUseRecvBuffer();
        DD("%s: [%p] save socket %d needSaveBuffer %d\n", __func__, this,
           mHostSocket.fd(), needSaveBuffer);
        stream->putByte(needSaveBuffer);
        if (needSaveBuffer) {
            mHostSocket.onSave(stream);
        }
        // We do not register the buffers that are in other states
        if (needSaveBuffer) {
            CrossSessionSocket::registerForRecycle(
                mHostSocket.fd());
        }
        if (mHostSocket.hasStaleData()) {
            signalWake(PIPE_WAKE_READ);
        }
        DD("%s: [%p] poll %d", __func__, this, mFdWatcher->poll());
    } else {
        stream->putBe32(0);
    }
    if (mPortType == Jdwp) {
        int proxyCount = mAdbHub->getProxyCount();
        D("Saving %d jdwp proxies", (int)proxyCount);
        stream->putBe32(proxyCount);
        mAdbHub->onSave(stream);
    } else {
        stream->putBe32(-1);
    }
}

AdbGuestPipe::~AdbGuestPipe() {
    if (needsHubTranslation()) {
        stopSocketTraffic();
    }
    CrossSessionSocket::recycleSocket(std::move(mHostSocket));
    DD("%s: [%p] destroyed", __func__, this);
    CHECK(mState == State::ClosedByGuest ||
          mState == State::ClosedByHost);
    service()->unregisterActivePipe(this);
}

void AdbGuestPipe::onGuestClose(PipeCloseReason reason) {
    DD("%s: [%p]", __func__, this);
    mState = State::ClosedByGuest;
    DINIT("%s: [%p] Adb closed by guest",__func__, this);
    if (needsHubTranslation()) {
        stopSocketTraffic();
    }
    CrossSessionSocket::recycleSocket(std::move(mHostSocket));
    service()->onPipeClose(this);  // This deletes the instance.
}

unsigned AdbGuestPipe::onGuestPoll() const {
    D("%s: [%p]", __func__, this);
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
            if (needsHubTranslation()) {
                result = mAdbHub->onGuestPoll();
            } else {
                unsigned flags = mFdWatcher->poll();
                if (flags & FdWatch::kEventRead) {
                    result |= PIPE_POLL_IN;
                }
                if (flags & FdWatch::kEventWrite) {
                    result |= PIPE_POLL_OUT;
                }
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
    D("%s: [%p] numBuffers=%d state=%s", __func__, this, numBuffers,
      toString(mState));
    if (mState == State::ProxyingData) {
        // Common case, proxy-ing the data from the host to the guest.
        int count = needsHubTranslation()
                            ? mAdbHub->onGuestRecvData(buffers, numBuffers)
                            : onGuestRecvData(buffers, numBuffers);
        if (android_hw->test_monitorAdb> 0) {
            mReceivedMesg->read(buffers, numBuffers, count);
        }
        if (needsHubTranslation()) {
            if (count == PIPE_ERROR_IO) {
                mState = State::ClosedByHost;
                stopSocketTraffic();
                mHostSocket.reset();
            }
            if (count == PIPE_ERROR_AGAIN && !mAdbHub->socketWantRead()) {
                mFdWatcher->dontWantRead();
            }
            if (mAdbHub->socketWantRead()) {
                mFdWatcher->wantRead();
            }
        }
        return count;
    } else if (guest_boot_completed == 0 && android_hw->test_delayAdbTillBootComplete == 1) {
        return PIPE_ERROR_AGAIN;
    } else if (guest_data_partition_mounted == 0 && mPlayStoreImage) {
        return PIPE_ERROR_AGAIN;
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
                              int numBuffers,
                              void** newPipePtr) {
    D("%s: [%p] numBuffers=%d state=%s", __func__, this, numBuffers,
      toString(mState));
    if (mState == State::ProxyingData) {
        // Common-case, proxy-ing the data from the guest to the host.
        int count = needsHubTranslation()
                            ? mAdbHub->onGuestSendData(buffers, numBuffers)
                            : onGuestSendData(buffers, numBuffers);
        if (android_hw->test_monitorAdb > 0) {
            mSendingMesg->read(buffers, numBuffers, count);
        }
        if (needsHubTranslation()) {
            if (count == PIPE_ERROR_IO) {
                D("onGuestSend PIPE_ERROR_IO");
                mState = State::ClosedByHost;
                stopSocketTraffic();
                mHostSocket.reset();
            }
            if (count == PIPE_ERROR_AGAIN && !mAdbHub->socketWantWrite()) {
                mFdWatcher->dontWantWrite();
            }
            if (mAdbHub->socketWantWrite()) {
                mFdWatcher->wantWrite();
            }
        }
        return count;
    } else if (mState == State::WaitingForGuestAcceptCommand ||
               mState == State::WaitingForGuestStartCommand) {
        // Waiting command bytes from the guest.
        return onGuestSendCommand(buffers, numBuffers);
    } else if (mReuseFromSnapshot &&
               mState == State::WaitingForHostAdbConnection) {
        return PIPE_ERROR_AGAIN;
    } else {
        if (mState != State::ClosedByHost) {
            // Invalid state !!!
            LOG(ERROR) << "Invalid state: " << toString(mState);
        }
        return PIPE_ERROR_IO;
    }
}

void AdbGuestPipe::onGuestWantWakeOn(int flags) {
    DD("%s: [%p] flags=%x (%d)", __func__, this, (unsigned)flags, flags);
    if (!mHostSocket.valid()) {
        return;
    }
    if (needsHubTranslation()) {
        int hubWakeFlags = mAdbHub->pipeWakeFlags();
        // If guest read/write would block, signal host socket event to
        // clear/fill the internal buffer.
        if (flags & PIPE_WAKE_READ) {
            if (!(hubWakeFlags & PIPE_WAKE_READ)) {
                mFdWatcher->wantRead();
            }
        }
        if (flags & PIPE_WAKE_WRITE) {
            if (!(hubWakeFlags & PIPE_WAKE_WRITE)) {
                mFdWatcher->wantWrite();
            }
        }
        flags &= hubWakeFlags;
        if (flags) {
            signalWake(flags);
        }
    } else {
        if (flags & PIPE_WAKE_READ) {
            mFdWatcher->wantRead();
        }
        if (flags & PIPE_WAKE_WRITE) {
            mFdWatcher->wantWrite();
        }
    }
}

void AdbGuestPipe::onHostConnection(ScopedSocket&& socket,
                                    AdbPortType portType) {
    DD("%s: [%p] host connection", __func__, this);
    CHECK(mState <= State::WaitingForHostAdbConnection);

    android::base::socketSetNonBlocking(socket.get());
    // socketSetNoDelay() reduces the latency of sending data, at the cost
    // of creating more TCP packets on the connection. It's useful when
    // doing lots of small send() calls, like the ADB protocol requires.
    // And since this is on localhost, the packet increase should not be
    // noticeable.
    android::base::socketSetNoDelay(socket.get());

    mFdWatcher.reset(android::base::ThreadLooper::get()->createFdWatch(
            socket.get(),
            [](void* opaque, int fd, unsigned events) {
                static_cast<AdbGuestPipe*>(opaque)->onHostSocketEvent(events);
            },
            this));
    assert(mFdWatcher);

    mHostSocket = CrossSessionSocket(std::move(socket));
    mPortType = portType;
    DD("set up pipe type %d\n", portType);

    if (mReuseFromSnapshot) {
        mState = State::ProxyingData;
        if (mAdbHub->socketWantRead()) {
            mFdWatcher->wantRead();
        }
        signalWake(PIPE_WAKE_READ | PIPE_WAKE_WRITE);
        D("Reuse pipe %p from snapshot", this);
    } else {
        DD("%s: [%p] sending reply", __func__, this);
        setReply("ok", State::SendingAcceptReplyOk);
        signalWake(PIPE_WAKE_READ);
    }
}

void AdbGuestPipe::resetConnection() {
    D("%s: [%p] reset connection\n", __func__, this);
    service()->hostCloseSocket(mHostSocket.fd());
    mHostSocket.reset();
    mState = State::ClosedByHost;
    signalWake(PIPE_WAKE_CLOSED);
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
    DD("%s: [%p] events=%x (%u)", __func__, this, events, events);

    if (needsHubTranslation()) {
        mAdbHub->onHostSocketEvent(mFdWatcher->fd(), events, [this]() {
            mState = State::ClosedByHost;
            resetConnection();
        });
        if (mState != State::ClosedByHost) {
            if (mAdbHub->socketWantRead()) {
                mFdWatcher->wantRead();
            } else {
                mFdWatcher->dontWantRead();
            }
            if (mAdbHub->socketWantWrite()) {
                mFdWatcher->wantWrite();
            } else {
                mFdWatcher->dontWantWrite();
            }
            int wakeFlags = mAdbHub->pipeWakeFlags();
            if (wakeFlags) {
                signalWake(wakeFlags);
            }
        }
    } else {
        int wakeFlags = 0;
        if ((events & FdWatch::kEventRead) != 0) {
            wakeFlags |= PIPE_WAKE_READ;
            mFdWatcher->dontWantRead();
        }
        if ((events & FdWatch::kEventWrite) != 0) {
            wakeFlags |= PIPE_WAKE_WRITE;
            mFdWatcher->dontWantWrite();
        }
        if (wakeFlags) {
            signalWake(wakeFlags);
        }
    }
}

int AdbGuestPipe::onGuestRecvData(AndroidPipeBuffer* buffers, int numBuffers) {
    DD("%s: [%p] numBuffers=%d", __func__, this, numBuffers);
    CHECK(mState == State::ProxyingData);
    int result = 0;
    while (numBuffers > 0) {
        uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        DD("%s: [%p] dataSize=%d", __func__, this, (int)dataSize);
        while (dataSize > 0) {
            ssize_t len;
            {
                // Possible that the host socket has been reset.
                if (mHostSocket.hasStaleData()) {
                    len = mHostSocket.readStaleData(data, dataSize);
                    DD("%s: [%p] loaded %d data from buffer", __func__, this,
                       (int)len);
                } else if (mHostSocket.valid()) {
                    len = android::base::socketRecv(
                            mHostSocket.fd(), data, dataSize);
                } else {
                    fprintf(stderr, "WARNING: AdbGuestPipe socket closed in the middle of recv\n");
                    mState = State::ClosedByHost;
                    len = -1;
                }
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
                    mFdWatcher->dontWantRead();
                    DD("%s: [%p] done try again", __func__, this);
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
            DD("%s: [%p] done %d", __func__, this, result);
            return result;
        }
        buffers++;
        numBuffers--;
    }
    DD("%s: [%p] done %d", __func__, this, result);
    return result;
}

#ifdef _DEBUG

static int parseMsgSize(const char* msg) {
    return *reinterpret_cast<const int32_t*>(msg + 12);
}

#endif

int AdbGuestPipe::onGuestSendData(const AndroidPipeBuffer* buffers,
                                  int numBuffers) {
    DD("%s: [%p] numBuffers=%d", __func__, this, numBuffers);
    CHECK(mState == State::ProxyingData);
    int result = 0;
#ifdef _DEBUG
    int msgSize = 0;
#endif
    while (numBuffers > 0) {
        const uint8_t* data = buffers[0].data;
        size_t dataSize = buffers[0].size;
        while (dataSize > 0) {
            ssize_t len;
            {
                // Possible that the host socket has been reset.
                if (mHostSocket.valid()) {
                    len = android::base::socketSend(
                            mHostSocket.fd(), data, dataSize);
#ifdef _DEBUG
                    if (msgSize) {
                        msgSize -= dataSize;
                        assert(msgSize >= 0);
                    } else {
                        msgSize = parseMsgSize(data);
                        assert(msgSize >= 0);
                    }
#endif
                } else {
                    fprintf(stderr, "WARNING: AdbGuestPipe socket closed in the middle of send\n");
                    mState = State::ClosedByHost;
                    len = -1;
                }
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
                    mFdWatcher->dontWantWrite();
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
#ifdef _DEBUG
    assert(!msgSize);
#endif
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
                stopSocketTraffic();
                mHostSocket.reset();
                return PIPE_ERROR_IO;
            }
            data += avail;
            dataSize -= avail;
            result += static_cast<int>(avail);
            mBufferPos += avail;
            if (mBufferPos == mBufferSize) {
                // Expected command was received.
                DD("%s: [%p] command '%.*s' validated", __func__, this,
                   (int)mBufferSize, mBuffer);
                if (mState == State::WaitingForGuestAcceptCommand) {
                    waitForHostConnection();
                } else if (mState == State::WaitingForGuestStartCommand) {
                    // Proxying data can start right now.
                    mState = State::ProxyingData;
                    // when -verbose, print a message indicating adb is connected
                    DINIT("%s: [%p] Adb connected, start proxing data",__func__, this);
                    if (needsHubTranslation()) {
                        mAdbHub.reset(new AdbHub());
                        if (mAdbHub->socketWantRead()) {
                            mFdWatcher->wantRead();
                        }
                    }
                }
                return result;
            }
        }
        numBuffers--;
        buffers++;
    }
    DD("%s: [%p] returning %d with buffer pos/size %d/%d", __func__, this,
       result, (int)mBufferPos, (int)mBufferSize);
    return result;
}

void AdbGuestPipe::stopSocketTraffic() {
    if (mFdWatcher) {
        mFdWatcher->dontWantWrite();
        mFdWatcher->dontWantRead();
    }
}

void AdbGuestPipe::setReply(StringView reply, State newState) {
    CHECK(newState == State::SendingAcceptReplyOk);
    CHECK(reply.size() <= sizeof(mBuffer));
    memcpy(mBuffer, reply.data(), reply.size());
    mBufferSize = reply.size();
    mBufferPos = 0;
    mState = newState;
}

void AdbGuestPipe::setExpectedGuestCommand(StringView command, State newState) {
    CHECK(newState == State::WaitingForGuestAcceptCommand ||
          newState == State::WaitingForGuestStartCommand);
    CHECK(command.size() <= sizeof(mBuffer));
    memcpy(mBuffer, command.data(), command.size());
    mBufferSize = command.size();
    mBufferPos = 0;
    mState = newState;
}

void AdbGuestPipe::waitForHostConnection() {
    // No host connection yet. The guest is still waiting for the 'ok'
    // reply.
    DD("%s: [%p] waiting for host connection", __func__, this);
    mState = State::WaitingForHostAdbConnection;
    // Tell the agent to start listening again.
    // Also notify the server that an emulator instance is waiting
    // for a connection. This is useful when the ADB Server was
    // restarted on the host, but could not see the current emulator
    // instance because it's listening on a 'non-standard' ADB port.
    mHostAgent->startListening();
    mHostAgent->notifyServer();
}

bool AdbGuestPipe::shouldUseRecvBuffer() {
    return isProxyingData();
}

bool AdbGuestPipe::needsHubTranslation() const {
    auto res = mReuseFromSnapshot || mPortType == AdbPortType::Jdwp;
    return res;
}

}  // namespace emulation
}  // namespace android

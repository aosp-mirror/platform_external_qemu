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

#include "android/base/async/ThreadLooper.h"
#include "android/emulation/AdbVsockPipe.h"

#include <chrono>

using FdWatch = android::base::Looper::FdWatch;

namespace android {
namespace emulation {

AdbVsockPipe::Service::Service(AdbHostAgent* hostAgent)
    : mHostAgent(hostAgent)
    , mGuestAdbdPollingThread([this](){ pollGuestAdbd(); })
{}

AdbVsockPipe::Service::~Service() {
    mGuestAdbdPollingThreadRunning = false;
    mGuestAdbdPollingThread.join();
}

void AdbVsockPipe::Service::onHostConnection(android::base::ScopedSocket&& socket,
                                             AdbPortType portType) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p socket=%d\n",
            "AdbVsockPipe::Service", __func__, __LINE__, this, socket.get());

    auto pipe = AdbVsockPipe::create(std::move(socket), portType);
    if (pipe) {
        mPipes.insert(std::move(pipe));
    }
}

void AdbVsockPipe::Service::pollGuestAdbd() {
    bool listening = false;

    while (mGuestAdbdPollingThreadRunning) {
        const bool alive = checkIfGuestAdbdAlive();
        if (listening != alive) {
            if (alive) {
                mHostAgent->startListening();
            } else {
                mHostAgent->stopListening();
            }
            listening = alive;
        }

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(500ms);
    }

    if (listening) {
        mHostAgent->stopListening();
    }
}

bool AdbVsockPipe::Service::checkIfGuestAdbdAlive() {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe::Service", __func__, __LINE__, this);

    return false;
}

void AdbVsockPipe::Service::resetActiveGuestPipeConnection() {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe::Service", __func__, __LINE__, this);

    mPipes.clear();
}

AdbVsockPipe::AdbVsockPipe(const void* guestStream,
                           android::base::ScopedSocket socket,
                           AdbPortType portType)
        : mSocket(std::move(socket)), mPortType(portType) {
    mSocketWatcher.reset(android::base::ThreadLooper::get()->createFdWatch(
        mSocket.fd(),
        [](void* opaque, int fd, unsigned events) {
            static_cast<AdbVsockPipe*>(opaque)->onHostSocketEvent(events);
        },
        this));

    mSocketWatcher->wantRead();
//    mSocketWatcher->wantWrite();
}

const void* AdbVsockPipe::connectToGuestAdbd() {
    fprintf(stderr, "rkir555 %s:%s:%d\n",
            "AdbVsockPipe", __func__, __LINE__);

    // TODO
    static const int q = 42;
    return &q;
}

std::unique_ptr<AdbVsockPipe> AdbVsockPipe::create(android::base::ScopedSocket socket,
                                                   AdbPortType portType) {
    fprintf(stderr, "rkir555 %s:%s:%d\n",
            "AdbVsockPipe", __func__, __LINE__);

    const void* guestStream = connectToGuestAdbd();
    if (guestStream) {
        android::base::socketSetNonBlocking(socket.get());
        android::base::socketSetNoDelay(socket.get());
        return std::make_unique<AdbVsockPipe>(guestStream, std::move(socket), portType);
    } else {
        return nullptr;
    }
}

void AdbVsockPipe::onHostSocketEvent(unsigned events) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p events=%08X (%s %s)\n",
            "AdbVsockPipe", __func__, __LINE__, this, events,
            ((events & FdWatch::kEventRead) ? "read" : ""),
            ((events & FdWatch::kEventWrite) ? "write" : ""));

    if (events & FdWatch::kEventRead) {
        char data[256];
        size_t n = android::base::socketRecv(mSocket.fd(), data, sizeof(data));

        fprintf(stderr, "rkir555 %s:%s:%d dataSize=%zu data='%.*s'\n",
                "AdbGuestPipe", __func__, __LINE__, n, int(n), data);
    }

    if (events & FdWatch::kEventWrite) {
    }
}

/*void AdbVsockPipe::onGuestClose(PipeCloseReason reason) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p reason=%d\n",
            "AdbVsockPipe", __func__, __LINE__, this, reason);
}

unsigned AdbVsockPipe::onGuestPoll() const {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe", __func__, __LINE__, this);
    return 0;
}

int AdbVsockPipe::onGuestRecv(AndroidPipeBuffer* buffers, int count) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p buffers=%p count=%d\n",
            "AdbVsockPipe", __func__, __LINE__, this, buffers, count);
    return 0;
}

int AdbVsockPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int count, void** newPipePtr) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p buffers=%p count=%d\n",
            "AdbVsockPipe", __func__, __LINE__, this, buffers, count);
    return 0;
}

void AdbVsockPipe::onGuestWantWakeOn(int flags) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe", __func__, __LINE__, this);
}

void AdbVsockPipe::onSave(android::base::Stream* stream) {
    fprintf(stderr, "rkir555 %s:%s:%d this=%p\n",
            "AdbVsockPipe", __func__, __LINE__, this);
}*/

}  // namespace emulation
}  // namespace android

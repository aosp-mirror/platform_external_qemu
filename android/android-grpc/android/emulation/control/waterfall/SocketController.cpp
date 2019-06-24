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

#include "android/emulation/control/waterfall/SocketController.h"

#include <cassert>
#include <thread>

#include "android/base/Log.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/containers/BufferQueue.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/android_pipe_unix.h"
#include "android/emulation/control/waterfall/WaterfallServiceForwarder.h"
#include "android/utils/path.h"
#include "android/utils/sockets.h"
#include "control_socket.pb.h"

/* set to 1 for very verbose debugging */
#define DEBUG 1

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("Controller: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_BUF(fd, buf, len)                            \
    do {                                                \
        printf("Controller (%d):", fd);                 \
        for (int x = 0; x < len; x++) {                 \
            if (isprint((int)buf[x]))                   \
                printf("%c", buf[x]);                   \
            else                                        \
                printf("[0x%02x]", 0xff & (int)buf[x]); \
        }                                               \
        printf("\n");                                   \
    } while (0)

#else
#define DD(...) (void)0
#define DD_BUF(...) (void)0
#endif

namespace android {
namespace emulation {
namespace control {
using namespace base;
using namespace ::waterfall;

ControlSocket::WaterfallSocket::WaterfallSocket(int fd,
                                                int id,
                                                ControlSocket* controller)
    : mFd(fd), mId(id), mController(controller) {
    mChannel = grpc::CreateCustomInsecureChannelFromFd("waterfall-fwd-channel",
                                                       fd, {});
    if (mChannel) {
        mStub = Waterfall::NewStub(mChannel);
    } else {
        DD("Unable to construct communication channel.");
    }
}

bool ControlSocket::WaterfallSocket::isOpen() {
    return mController->isIdOpen(mFd);
}

Stub* ASyncWaterfallServiceForwarder::open() {
    AutoLock lock(mMapLock);

    // Lets go over the available sockets, throwing away the ones
    // we know are not open..
    while (!mAvailableStubs.empty()) {
        Stub* stub = *mAvailableStubs.cbegin();
        mAvailableStubs.erase(stub);

        auto socket = mStubToSock[stub];
        // Make sure the socket is still open..
        if (socket->isOpen()) {
            DD("%p is still marked as 'open', handing it out.", stub);
            return stub;
        }

        DD("Discarding %p", stub);
        // Discard the closed socket and get the next one.
        mStubToSock.erase(stub);
    }

    // Looks like all stubs have been checked out, lets open up a new
    // connection.
    ControlSocket::WaterfallSocket* socket = mControlSocket.open();
    if (!socket) {
        // Bad news no socket available, waterfall probably disappeared.
        DD("Unable to open up a control socket before timeout.");
        return nullptr;
    }

    DD("Registering %p", socket->getStub());
    mStubToSock[socket->getStub()] =
            std::shared_ptr<ControlSocket::WaterfallSocket>(socket);
    return socket->getStub();
}

void ASyncWaterfallServiceForwarder::close(Stub* stub) {
    AutoLock lock(mMapLock);
    // If the stub has not been deleted we make it available.
    if (mStubToSock.count(stub) > 0) {
        DD("Making %p available", stub);
        mAvailableStubs.insert(stub);
    }
}

void ASyncWaterfallServiceForwarder::erase(Stub* stub) {
    AutoLock lock(mMapLock);
    DD("Erasing %p", stub);
    mStubToSock.erase(stub);
    mAvailableStubs.erase(stub);
}

ControlSocket::WaterfallSocket::~WaterfallSocket() {
    DD("Bye bye socket! id: %d, fd: %d", mId, mFd);
    mController->closeById(mId);
}

ControlSocket::ControlSocket() {
    mLooper = android::base::ThreadLooper::get();
    initialize();
}
static void socket_watcher(void* opaque, int fd, unsigned events) {
    const auto service = static_cast<ControlSocket*>(opaque);
    if (!service) {
        return;
    }
    if ((events & Looper::FdWatch::kEventRead) != 0) {
        service->onRead(fd);
    }
}

bool ControlSocket::readMessage(int fd, SocketControl* msg) {
    // We are calling closeByFd, which needs a lock.
    assert(!mMapLock.isLocked());

    // TODO(jansene): Hack attack! We really rely on our messages to
    // be fixed size here. It would be better to do proper decoding.
    // Or use a format (not protobuf) with a guaranteed fixed size.
    msg->set_fd(0);
    msg->set_sort(SocketControl::close);

    std::vector<char> msgBuffer(msg->ByteSizeLong());

    // TODO(jansene): We expect a frame to contain a whole message.
    // which is not really guaranteed..
    // this might not work if you are super aggressive on the control socket.
    int read = socketRecv(fd, msgBuffer.data(), msgBuffer.capacity());
    if (read == 0) {
        DD("Close socket %d -- %lu", fd, msgBuffer.capacity());
        // A read of 0, means the socket was closed, so clean up
        // everything properly.
        closeByFd(fd);
    }
    if (read < 0) {
        // Oh oh..
        DD("Read failure %d, err: %d, %s", read, errno, errno_str);
        return false;
    }

    DD_BUF(fd, msgBuffer.data(), msgBuffer.capacity());
    return msg->ParseFromArray(msgBuffer.data(), msgBuffer.capacity());
}

ControlSocket::WaterfallSocket* ControlSocket::open(int timeoutInMs) {
    SocketControl ctrlMsg;
    ctrlMsg.set_sort(SocketControl::open);
    ctrlMsg.set_fd(mFdCounter++);
    auto msg = ctrlMsg.SerializeAsString();

    // TOOD(jansene): We rely on being able to write out the whole frame.
    // this might not work if you are super aggressive on the control socket.
    int snd = socketSend(mControlSocket, msg.data(), msg.size());
    DD_BUF(mControlSocket, msg.data(), snd);

    AutoLock lock(mMapLock);
    if (mAvailableIds.empty()) {
        System::Duration waitUntilUs =
                System::get()->getUnixTimeUs() + timeoutInMs * 1000;
        mCv.timedWait(&mMapLock, waitUntilUs);
    }
    // Nobody replied in time..
    if (mAvailableIds.empty()) {
        DD("No timely response.. %" PRIu64, System::get()->getUnixTimeUs());
        return nullptr;
    }
    auto fst = mAvailableIds.begin();
    mAvailableIds.erase(fst);
    auto idFd = mIdToFd.find(*fst);
    assert(idFd != mIdToFd.end());

    DD("Returning id: %d -> fd: %d", idFd->first, idFd->second);
    return new ControlSocket::WaterfallSocket(idFd->second, idFd->first, this);
}

void ControlSocket::onRead(int fd) {
    DD("onRead fd: %d", fd);
    if (fd == mDomainSocket) {
        // A new connection is coming in..
        onAccept();
        return;
    }

    SocketControl ctrlMsg;
    if (fd == mControlSocket) {
        while (readMessage(fd, &ctrlMsg)) {
            DD("ControlSocket incoming from %d - %s", fd,
               ctrlMsg.DebugString().c_str());
            // The control channel is unable to open a connection to the
            // emulator, and sending the identity message on the opened
            // control socket makes no sense..
            if (ctrlMsg.sort() != SocketControl::close) {
                LOG(INFO) << "Ignoring unexpected message on control channel "
                          << ctrlMsg.DebugString();
                continue;
            }

            // Schedule a close operation..
            int id = ctrlMsg.fd();
            mLooper->scheduleCallback([&, id] { closeById(id); });
        }
        mFdToWatch[fd]->wantRead();
        return;
    }

    // First message on this fd! Welcome aboard.
    if (readMessage(fd, &ctrlMsg)) {
        DD("Incoming message from %d - %s", fd, ctrlMsg.DebugString().c_str());
        if (ctrlMsg.sort() != SocketControl::identity) {
            LOG(INFO) << "Ignoring unexpected message "
                      << ctrlMsg.DebugString();
        }
        if (ctrlMsg.fd() == 0) {
            // An incoming control socket.
            DD("Registered control socket at %d", fd);

            // Close out existing connections as a new controller is in town.
            AutoLock lock(mMapLock);
            for (auto idFd : mIdToFd) {
                close(idFd.second);
                mFdToWatch.erase(idFd.second);
            }
            mFdToId.clear();
            mIdToFd.clear();

            // And register a new control socket..
            mFdToId[fd] = ctrlMsg.fd();
            mIdToFd[ctrlMsg.fd()] = fd;
            mControlSocket = fd;
            mFdToWatch[fd]->wantRead();
        } else {
            DD("Registered incoming connection fd: %d <-> %d", fd,
               ctrlMsg.fd());
            AutoLock lock(mMapLock);
            mFdToId[fd] = ctrlMsg.fd();
            mIdToFd[ctrlMsg.fd()] = fd;
            mAvailableIds.insert(ctrlMsg.fd());
            mFdToWatch[fd]->dontWantRead();
            mFdToWatch.erase(fd);

            // Notify the listeners of a new socket!
            mCv.signal();
        }
    } else {
        // Failed to read the protocol buffer.
        DD("Gargbage on %d", fd);
    }
}

// Closes the given socket.
void ControlSocket::closeByFd(int fd) {
    AutoLock lock(mMapLock);

    auto watch = mFdToWatch.find(fd);
    if (watch != mFdToWatch.end()) {
        // enough please!
        DD("Stopped watching %d..", fd);
        mFdToWatch[fd]->dontWantRead();
    }
    auto it = mFdToId.find(fd);

    if (it == mFdToId.end()) {
        // No longer active..
        DD("Socket %d not active..", fd);
        return;
    }

    DD("Marked %d for closing", fd);
    // Put it up for deletion!

    // 1. Cleanup is guaranteed to happen on looper thread, so
    //   we can safely delete watchers
    // 2. This gives the other threads a chance to read the last
    //   few remaining bytes.
    mLooper->scheduleCallback([&] { closeById(it->second); });
}

bool ControlSocket::isIdOpen(int id) {
    AutoLock lock(mMapLock);
    return mIdToFd.count(id) > 0;
}

void ControlSocket::closeById(int socketId) {
    DD("Closing id %d", socketId);
    AutoLock lock(mMapLock);
    auto it = mIdToFd.find(socketId);

    if (it == mIdToFd.end()) {
        // No longer active..
        return;
    }
    int socket = it->second;
    mIdToFd.erase(it);
    mFdToId.erase(socket);
    mAvailableIds.erase(socket);

    socketClose(socket);
}

void ControlSocket::onAccept() {
    int fd = socketAcceptAny(mDomainSocket);
    if (fd < 0) {
        if (errno != EAGAIN)
            LOG(ERROR) << "Failed to accept socket:" << mDomainSocket
                       << ", err: " << errno << ", " << errno_str;
        return;
    }
    DD("Accepted %d at %d", fd, mDomainSocket);
    mFdToWatch[fd] = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(fd, socket_watcher, this));
    mFdToWatch[fd]->wantRead();
}

bool ControlSocket::initialize() {
    DD("Initializing");
    // Allow the waterfall service to connect to us.
    const char* socketdir = "sockets";
    const char* path = "sockets/h2o";
    android_unix_pipes_add_allowed_path(path);
    SockAddress addr = {};
    path_mkdir_if_needed(socketdir, 0755);
    path_delete_file(path);
    addr.u._unix.path = path;
    addr.family = SOCKET_UNIX;
    mDomainSocket = socket_create(SOCKET_UNIX, SOCKET_STREAM);
    if (socket_bind(mDomainSocket, &addr) != 0) {
        LOG(ERROR) << "Unable to listen on: " << mDomainSocket
                   << ", err: " << errno << ", str: " << errno_str;
        return false;
    };

    if (mDomainSocket < 0) {
        LOG(ERROR) << "Invalid waterfall server socket.";
        return false;
    }

    if (socket_listen(mDomainSocket, 1) < 0) {
        LOG(ERROR) << "Unable to listen on: " << mDomainSocket
                   << ", err: " << errno << ", str: " << errno_str;
        return false;
    }

    socketSetNonBlocking(mDomainSocket);

    mFdWatch = std::unique_ptr<Looper::FdWatch>(
            mLooper->createFdWatch(mDomainSocket, socket_watcher, this));
    mFdWatch->wantRead();
    DD("H2O is ready and watching on %d!", mDomainSocket);
    return true;
};

}  // namespace control
}  // namespace emulation
}  // namespace android

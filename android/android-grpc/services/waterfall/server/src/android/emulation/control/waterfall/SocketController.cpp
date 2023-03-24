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

#include <errno.h>     // for errno, EAGAIN
#include <unistd.h>    // for close
#include <cassert>     // for assert
#include <functional>  // for __base
#include <string>      // for basic_string
#include <utility>     // for pair
#include <vector>      // for vector

#include "aemu/base/Log.h"                        // for LogStream, LOG, Log...
#include "aemu/base/async/ThreadLooper.h"         // for ThreadLooper
#include "aemu/base/sockets/SocketUtils.h"        // for socketAcceptAny
#include "android/base/system/System.h"           // for System, System::Dur...
#include "android/emulation/android_pipe_unix.h"  // for android_unix_pipes_...
#include "android/utils/path.h"                   // for path_delete_file
#include "android/utils/sockets.h"                // for SockAddress, errno_str
#include "control_socket.pb.h"                    // for SocketControl, Sock...
#include "grpcpp/create_channel_posix.h"          // for CreateCustomInsecur...
#include "waterfall.pb.h"                         // for waterfall

/* set to 1 for very verbose debugging */
#define DEBUG 0

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
    return mController->isIdOpen(mId);
}

Stub* ControlSocketLibrary::borrow() {
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

void ControlSocketLibrary::restore(Stub* stub) {
    AutoLock lock(mMapLock);
    // If the stub has not been deleted we make it available.
    if (mStubToSock.count(stub) > 0) {
        DD("Making %p available", stub);
        mAvailableStubs.insert(stub);
    }
}

void ControlSocketLibrary::erase(Stub* stub) {
    AutoLock lock(mMapLock);
    DD("Erasing %p", stub);
    mStubToSock.erase(stub);

    if (mAvailableStubs.count(stub) > 0) {
        DD("Warning! Erasing a borrowed stub!");
    }
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
    // assert(!mMapLock.isLocked());

    // TODO(jansene): Hack attack! We really rely on our messages to
    // be fixed size here. It would be better to do proper decoding.
    // Or use a format (not protobuf) with a guaranteed fixed size.
    msg->set_fd(0);
    msg->set_sort(SocketControl::close);

    std::vector<char> msgBuffer(msg->ByteSizeLong());

    // TODO(jansene): We expect a frame to contain a whole message.
    // which is not really guaranteed..
    // this might not work if you are super aggressive on the control socket.
    // Default read buffers are usually >4kb and our read is 7 bytes.
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

bool ControlSocket::sendMessage(SocketControl_Sort sort, int id) {
    SocketControl ctrlMsg;
    ctrlMsg.set_sort(sort);
    ctrlMsg.set_fd(id);
    auto msg = ctrlMsg.SerializeAsString();

    // TOOD(jansene): We rely on being able to write out the whole frame.
    // this might not work if you are super aggressive on the control socket.
    // Default write buffers are usually >4kb and our message is 7 bytes. so...
    int snd = socketSend(mControlSocket, msg.data(), msg.size());
    DD_BUF(mControlSocket, msg.data(), snd);
    assert(snd == msg.size());
    return snd == msg.size();
}

ControlSocket::WaterfallSocket* ControlSocket::open(int timeoutInMs) {
    sendMessage(SocketControl::open, mFdCounter++);
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
    if (fd == mDomainSocket)
        onAccept();
    else if (fd == mControlSocket)
        readMessageFromControlSocket(fd);
    else {
        readFirstMessage(fd);
    }
}

void ControlSocket::readFirstMessage(int fd) {
    // First message on this fd! Welcome aboard.
    SocketControl ctrlMsg;
    if (!readMessage(fd, &ctrlMsg)) {
        DD("Unable to parse first message.. Got garbage.. Ignoring");
        closeByFd(fd);
        return;
    }

    DD("Incoming message from %d - %s", fd, ctrlMsg.DebugString().c_str());
    if (ctrlMsg.sort() != SocketControl::identity) {
        DD("First message should be the identify message, not this!");
        closeByFd(fd);
        return;
    }

    if (ctrlMsg.fd() == kControlSocketId) {
        registerControlSocket(fd);
    } else {
        registerSocket(fd, ctrlMsg.fd());
    }
}

void ControlSocket::readMessageFromControlSocket(int fd) {
    SocketControl ctrlMsg;
    while (readMessage(fd, &ctrlMsg)) {
        DD("ControlSocket incoming from %d - %s", fd,
           ctrlMsg.DebugString().c_str());

        static_assert(SocketControl::close == SocketControl::Sort_MAX,
                      "You have added a new type of control message, update "
                      "the switch statement below.");
        switch (ctrlMsg.sort()) {
            case SocketControl::open:
                DD("The host side of the connection cannot open up a "
                   "connection to the guest.. Ignoring");
                continue;
            case SocketControl::identity:
                DD("Only the first message send on a channel can be the "
                   "identity message.");
                continue;
            case SocketControl::close:
                int id = ctrlMsg.fd();
                if (id == kControlSocketId) {
                    // Ok, thanks for letting us know you are shutting down the
                    // control socket. Time to say goodbye to everyone.
                    clearConnections();
                    return;
                }

                // Schedule a close operation..
                mLooper->scheduleCallback([&, id] { closeById(id); });
                continue;
        }
    }

    AutoLock lock(mMapLock);
    // It is always possible that someone yanked the socket out under of us.
    auto it = mFdToWatch.find(fd);
    if (it != mFdToWatch.end())
        mFdToWatch[fd]->wantRead();
}

void ControlSocket::registerSocket(int fd, int id) {
    DD("Registered incoming connection fd: %d <-> %d", fd, id);
    AutoLock lock(mMapLock);
    mFdToId[fd] = id;
    mIdToFd[id] = fd;
    mAvailableIds.insert(id);
    mFdToWatch[fd]->dontWantRead();
    mFdToWatch.erase(fd);

    // Notify the listeners of a new socket!
    mCv.signal();
}

void ControlSocket::registerControlSocket(int fd) {
    DD("Registered control socket at %d", fd);
    AutoLock lock(mMapLock);
    clearConnections();
    mFdToId[fd] = kControlSocketId;
    mIdToFd[kControlSocketId] = fd;
    mControlSocket = fd;
    mFdToWatch[fd]->wantRead();
}

void ControlSocket::clearConnections() {
    for (auto idFd : mIdToFd) {
        close(idFd.second);
        mFdToWatch.erase(idFd.second);
    }
    mFdToId.clear();
    mIdToFd.clear();
}

// Closes the given socket.
void ControlSocket::closeByFd(int fd) {
    AutoLock lock(mMapLock);

    auto watch = mFdToWatch.find(fd);
    if (watch != mFdToWatch.end()) {
        // enough please, note that this is always called from the looper
        // thread.
        DD("Stopped watching %d..", fd);
        mFdToWatch[fd]->dontWantRead();
        mFdToWatch.erase(watch);
    }
    auto it = mFdToId.find(fd);

    if (it == mFdToId.end()) {
        // No longer active..
        DD("Socket %d not active..", fd);
        return;
    }
    int id = it->second;
    DD("Marked %d for closing", id);
    if (id == kControlSocketId) {
        clearConnections();
    } else {
        // Put it up for deletion with a callback.
        // This gives the other threads a chance to read the last
        // few remaining bytes.
        mLooper->scheduleCallback([&] { closeById(id); });
    }
}

bool ControlSocket::isIdOpen(int id) {
    AutoLock lock(mMapLock);
    return mIdToFd.count(id) > 0;
}

void ControlSocket::closeById(int socketId) {
    // closeById should only be called on an existing waterfall socket
    assert(socketId != kControlSocketId);
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

    // No watcher can be active since a watcher only exists for
    // the domain socket:
    // control socket
    // a socket which has not received its identity message yet
    assert(mFdToWatch.count(socket) == 0);
    socketClose(socket);

    // Notify the other side.
    if (socketId != kControlSocketId) {
        sendMessage(SocketControl::close, socketId);
    }
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

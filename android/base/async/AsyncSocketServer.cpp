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

#include "android/base/async/AsyncSocketServer.h"

#include "android/base/async/Looper.h"
#include "android/base/async/ScopedSocketWatch.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/Log.h"
#include "android/base/sockets/SocketUtils.h"

#include <memory>

namespace android {
namespace base {

using FdWatch = android::base::Looper::FdWatch;

namespace {

class BaseSocketServer : public AsyncSocketServer {
public:
    BaseSocketServer(Looper* looper, int port, ConnectCallback connectCallback,
               int socket4, int socket6) :
            mLooper(looper), mPort(port), mConnectCallback(connectCallback) {
        if (socket4 >= 0) {
            mBoundSocket4.reset(looper->createFdWatch(socket4, onAccept, this));
            CHECK(mBoundSocket4.get());
        }
        if (socket6 >= 0) {
            mBoundSocket6.reset(looper->createFdWatch(socket6, onAccept, this));
            CHECK(mBoundSocket6.get());
        }
    }

    virtual ~BaseSocketServer() = default;

    virtual int port() const override { return mPort; }

    virtual void startListening() override {
        if (mBoundSocket4) {
            mBoundSocket4->wantRead();
        }
        if (mBoundSocket6) {
            mBoundSocket6->wantRead();
        }
    };

    virtual void stopListening() override {
        if (mBoundSocket4) {
            mBoundSocket4->dontWantRead();
        }
        if (mBoundSocket6) {
            mBoundSocket6->dontWantRead();
        }
    };

private:
    // Called when an i/o event happens on the bound socket. Typically
    // a read event indicates a new client connection.
    static void onAccept(void* opaque, int fd, unsigned events) {
        auto server = reinterpret_cast<BaseSocketServer*>(opaque);
        if (events & FdWatch::kEventRead) {
            int clientFd = socketAcceptAny(fd);
            if (clientFd < 0) {
                // This can happen when the client closed the connection
                // before we could process it. Just exit and try until the
                // next time.
                PLOG(WARNING) << "Error when accepting host connection";
                return;
            }
            if (!server->mConnectCallback(clientFd)) {
                // Assume the callback printed any necessary error message.
                socketClose(clientFd);
                return;
            }
            server->stopListening();
        }
    }

    ScopedSocketWatch mBoundSocket4;
    ScopedSocketWatch mBoundSocket6;
    Looper* mLooper = nullptr;
    int mPort = 0;
    ConnectCallback mConnectCallback;
};

}  // namespace

// static
AsyncSocketServer* AsyncSocketServer::createTcpLoopbackServer(
        int port,
        ConnectCallback connectCallback,
        LoopbackMode mode,
        Looper* looper) {
    if (!looper) {
        looper = ThreadLooper::get();
    }
    int serverFd4 = -1;
    int serverFd6 = -1;

    // If |port| is 0, we let the system find a random free port, but
    // in the case of kIPv4AndIPv6, we need to ensure that it is possible
    // to bind to both ports. Loop a few times to ensure that this is the
    // case.
    int tries = (port != 0 && mode == kIPv4AndIPv6) ? 1 : 5;
    int port0 = port;
    for (; tries > 0; tries--) {
        port = port0;
        if (mode & kIPv4) {
            serverFd4 = socketTcp4LoopbackServer(port);
            if (serverFd4 < 0) {
                continue;
            }
            if (!port) {
                port = socketGetPort(serverFd4);
            }
        }
        if (mode & kIPv6) {
            serverFd6 = socketTcp6LoopbackServer(port);
            if (serverFd6 < 0) {
                if (serverFd4 >= 0) {
                    socketClose(serverFd4);
                    serverFd4 = -1;
                    continue;
                }
            }
            if (!port) {
                port = socketGetPort(serverFd6);
            }
        }
    }

    if (serverFd4 < 0 && serverFd6 < 0) {
        return nullptr;
    }

    return new BaseSocketServer(looper, port, connectCallback,
                                serverFd4, serverFd6);
}

}  // namespace base
}  // namespace android

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
#include "android/base/Log.h"
#include "android/base/async/ThreadLooper.h"
#include "android/base/containers/BufferQueue.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/emulation/android_pipe_unix.h"
#include "android/emulation/control/waterfall/WaterfallServiceForwarder.h"
#include "android/utils/path.h"
#include "android/utils/sockets.h"


/* set to 1 for very verbose debugging */
#define DEBUG 0

#if DEBUG >= 1
#define DD(fmt, ...) \
    printf("SocketStreambuf: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#else
#define DD(...) (void)0
#endif


namespace android {
namespace emulation {
namespace control {

using SocketQueue = base::BufferQueue<int>;
using base::Lock;

class DefaultWaterfallConnection : public WaterfallConnection {
public:
    DefaultWaterfallConnection(std::function<int()>&& serverSockFn)
        : mServerSockFn(serverSockFn),
          mIncomingConnections(10, mConnectionLock) {
        mLooper = android::base::ThreadLooper::get();
    };

    ~DefaultWaterfallConnection() { base::socketClose(mDomainSocket); }

    int openConnection() override {
        int protocolFd = nextSocket();
        int snd = base::socketSend(protocolFd, mWaterfallConnectRequest.data(),
                                   mWaterfallConnectRequest.size());
        if (snd != mWaterfallConnectRequest.size()) {
            LOG(ERROR) << "Unable to send callback request to: " << protocolFd
                       << ", send:" << snd << ", errno:" << errno << ", "
                       << errno_str;
            return -1;
        }

        int fd = nextSocket();
        snd = base::socketSend(fd, "rdy", 3);
        if (snd != 3) {
            LOG(ERROR) << "Unable to send rdy to waterfall: " << errno << ", "
                       << errno_str;
            return -1;
        }

        char buf[4] = {0};
        if (base::socketRecv(fd, buf, 3) != 3 || buf[0] != 'r' ||
            buf[1] != 'd' || buf[2] != 'y') {
            LOG(ERROR) << "Did not receive rdy, but: [" << std::string(buf)
                       << "], err: " << errno << ", " << errno_str;
            return -1;
        };
        DD("Estabslished waterfall connection on %d", fd);
        return fd;
    }

    bool initialize() override {
        mDomainSocket = mServerSockFn();

        if (mDomainSocket < 0) {
            LOG(ERROR) << "Invalid waterfall server socket.";
            return false;
        }

        if (socket_listen(mDomainSocket, 1) < 0) {
            LOG(ERROR) << "Unable to listen on: " << mDomainSocket
                       << ", err: " << errno << ", str: " << errno_str;
            return false;
        }

        base::socketSetNonBlocking(mDomainSocket);
        mFdWatch = std::unique_ptr<Looper::FdWatch>(mLooper->createFdWatch(
                mDomainSocket, DefaultWaterfallConnection::socket_watcher,
                this));
        mFdWatch->wantRead();
        return true;
    };

private:
    // This callback is called whenever an I/O event happens on the
    // mDomainSocket
    // connecting the mDomainSocket to the host waterfall server.
    static void socket_watcher(void* opaque, int fd, unsigned events) {
        const auto service = static_cast<DefaultWaterfallConnection*>(opaque);
        if (!service) {
            return;
        }
        if ((events & Looper::FdWatch::kEventRead) != 0) {
            service->onAccept();
        }
    }

    int nextSocket() {
        int fd = -1;
        base::AutoLock lock(mConnectionLock);
        base::System::Duration blockUntil =
                base::System::get()->getUnixTimeUs() + kTimeoutMs * 1000;
        if (!mIncomingConnections.popLockedBefore(&fd, blockUntil) ==
            base::BufferQueueResult::Ok) {
            LOG(ERROR) << "No waterfall socket after waiting "
                      << kTimeoutMs << ". ms";
            return -1;
        }
        return fd;
    }

    void onAccept() {
        // This seems to create a read event..
        int fd = base::socketAcceptAny(mDomainSocket);
        if (fd < 0) {
            if (errno != EAGAIN)
                LOG(ERROR) << "Failed to accept socket waterfall socket:" << errno << ", "
                           << errno_str;
            return;
        }

        // Ok, we have a socket of interest..
        base::socketSetBlocking(fd);
        socket_read_timeout(fd, kTimeoutMs);
        socket_write_timeout(fd, kTimeoutMs);

        DD("Accepted: ", fd);
        base::AutoLock lock(mConnectionLock);
        mIncomingConnections.pushLocked(std::move(fd));
    }

    std::function<int()> mServerSockFn;
    const std::string mWaterfallConnectRequest = "pipe:unix:sockets/h2o";
    static constexpr uint64_t kTimeoutMs = 500;
    int mDomainSocket = -1;
    Looper* mLooper;
    Lock mConnectionLock;
    SocketQueue mIncomingConnections;
    std::unique_ptr<Looper::FdWatch> mFdWatch;
};

WaterfallConnection* getDefaultWaterfallConnection() {
    auto serverSocket = []() {
        // Allow the waterfall service to connect to us.
        const char* socketdir = "sockets";
        const char* path = "sockets/h2o";
        android_unix_pipes_add_allowed_path(path);
        SockAddress addr = {};
        path_mkdir_if_needed(socketdir, 0755);
        path_delete_file(path);
        addr.u._unix.path = path;
        addr.family = SOCKET_UNIX;
        base::ScopedSocket domainSock =
                socket_create(SOCKET_UNIX, SOCKET_STREAM);
        if (socket_bind(domainSock.get(), &addr) != 0) {
            LOG(ERROR) << "Unable to listen on: " << domainSock.get()
                       << ", err: " << errno << ", str: " << errno_str;
            return -1;
        };

        return domainSock.release();
    };

    return new DefaultWaterfallConnection(std::move(serverSocket));
}

WaterfallConnection* getTestWaterfallConnection(
        std::function<int()>&& serverSockFn) {
    return new DefaultWaterfallConnection(std::move(serverSockFn));
}
}  // namespace control
}  // namespace emulation
}  // namespace android

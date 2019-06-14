#include <sys/socket.h>

#include "android/base/Log.h"
#include "android/base/async/Looper.h"
#include "android/base/sockets/AsyncSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/synchronization/Lock.h"
#include "android/utils/path.h"
#include "android/utils/sockets.h"

namespace android {
namespace emulation {
namespace control {

class WaterfallServerSocket {
public:
    WaterfallForwarder(Looper* looper) : mLooper(looper) {
        SockAddress addr = {};
        path_mkdir_if_needed(mWaterfallPath, 0755);
        unlink(mWaterfallName);
        addr.u._unix.path = mWaterfallName;
        addr.family = SOCKET_UNIX;

        mSocket = socket_create(SOCKET_UNIX, SOCKET_STREAM);
        if (socket_bind(mSocket, &addr) < 0) {
            // Error..
        }
        if (socket_listen(mSocket, 1) < 0; {
            // ERROR
        }
        LOG(INFO) << "listen:" << res;

        base::socketSetNonBlocking(mSocket);

        base::AutoLock watchLock(mWatchLock);
        mFdWatch = std::unique_ptr<Looper::FdWatch>(
                 mLooper->createFdWatch(mSocket, incoming_connection, this));
        mFdWatch->wantRead();
    }

// This callback is called whenever an I/O event happens on the mSocket
// connecting the mSocket to the host ADB server.
static void socket_watcher(void* opaque, int fd, unsigned events) {
    const auto socket = static_cast<WaterfallServerSocket*>(opaque);
    if (!socket) {
        return;
    }

    if ((events & Looper::FdWatch::kEventRead) != 0) {
        socket->onRead();
    } else if ((events & Looper::FdWatch::kEventWrite) != 0) {
        socket->onWrite();
    }
}

    while (mWaterfallFd <= 0) {
            int fd = accept(mSocket, 0, 0);
            if (fd < 0) {
                printf("-> no: %s, %d\n", errno_str, errno);
            } else {
                mWaterfallFd = fd;
                LOG(INFO) << "mWaterfallFd: " << mWaterfallFd;
            }
        }
        // Set socket timeouts to be pretty brief, so we can handshake
        // synchronously.

        // TODO Make a global const
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if (setsockopt(mWaterfallFd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout,
                       sizeof(timeout)) < 0)
            LOG(ERROR) << "setsockopt failed";
        if (setsockopt(mWaterfallFd, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout,
                       sizeof(timeout)) < 0)
            LOG(ERROR) << "setsockopt failed";
    }


    const char* mWaterfallPath = "sockets";
    const char* mWaterfallName = "sockets/h2o";
    int mSocket;
    int mWaterfallFd;

    Looper* mLooper;

    // Queue of message that need to go out over this socket.
    Lock mWatchLock;
    base::FunctorThread mAcceptor;
}
}  // namespace control
}  // namespace emulation
}  // namespace android

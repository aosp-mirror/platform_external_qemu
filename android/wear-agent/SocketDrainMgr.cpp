// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.


#include <android/base/Limits.h>
#include <android/base/String.h>
#include "android/wear-agent/SocketDrainMgr.h"

extern "C" {
#include "android/android.h"
#include "android/async-utils.h"
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/list.h"
#include "android/utils/misc.h"
#ifdef _WIN32
#  define xxWIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else /* !_WIN32 */
#  include <sys/ioctl.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#  include <netdb.h>
#  if HAVE_UNIX_SOCKETS
#    include <sys/un.h>
#    ifndef UNIX_PATH_MAX
#      define  UNIX_PATH_MAX  (sizeof(((struct sockaddr_un*)0)->sun_path)-1)
#    endif
#  endif
#endif /* !_WIN32 */

}
#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//some implementation whys:
//when the looper is running, the sockets are non-blocking and are only
//closed after all data has been read;
//when looper stops, the remaining sockets are changed to blocking and are
//closed after all data has been read;
//when socket is already closed for read (not readable), we only need to 
//shutdown writing and close the socket

static void* s_socket_drain_mgr = 0;

namespace android {
namespace wear {

class Drainer {
public:
    Drainer(Looper* looper, int socket_fd);
    ~Drainer();

public:
    void drain();
    bool done() {return mDone;}
    void onRead();

private:
    Looper* mLooper;
    int     mSocket;
    LoopIo  mIo[1];
    bool    mDone;

    void shutdownRead();
    void shutdownWrite();
    void closeSocket();
    bool isReadable();
};

static void _drainer_read(void* opaque, int fd, unsigned events) {
    Drainer * drainer = (Drainer*)opaque;
    assert(drainer);

    if ((events & LOOP_IO_READ) != 0) {
        drainer->onRead();
    }
    if ((events & LOOP_IO_WRITE) != 0) {
        assert(0);
    }

    if (drainer->done()) {
        ((SocketDrainMgr*)(s_socket_drain_mgr))->remove(drainer);
    }
}

Drainer::Drainer(Looper* looper, int socket_fd) :
        mLooper(looper), mSocket(socket_fd), mIo(), mDone(false) { 
    shutdownWrite();
    if (isReadable() && mLooper) {
        loopIo_init(mIo, mLooper, mSocket, _drainer_read, this);
        loopIo_wantRead(mIo);
        loopIo_dontWantWrite(mIo);
    } else {
        //there is nothing to read
        mLooper=0;
        mDone=true;
    }
}

bool Drainer::isReadable() {
    errno=0;
    char buff[1024];
    int size = socket_recv(mSocket, buff, sizeof(buff));
    if (size > 0) {
        return true;
    } else if (size < 0 && errno == EWOULDBLOCK) {
        return true;
    }
    return false;
}

Drainer::~Drainer() {
    assert(done());
    shutdownRead();

    if (mLooper) {
        loopIo_dontWantRead(mIo);
        loopIo_done(mIo);
        mLooper=0;
    }

    closeSocket();
    mSocket=-1;
}

void Drainer::shutdownWrite() {
#ifdef _WIN32
    shutdown(mSocket, SD_SEND);
#else
    shutdown(mSocket, SHUT_WR);
#endif
}

void Drainer::shutdownRead() {
#ifdef _WIN32
    shutdown(mSocket, SD_RECEIVE);
#else
    shutdown(mSocket, SHUT_RD);
#endif
}

void Drainer::onRead() {
    assert(!done());
    if (isReadable()) {
        return;
    }
    mDone=true;
}

void Drainer::drain() {
    //1. set socket blocking 
    socket_set_blocking(mSocket);
    //2. read all
    char buff[1024];
    while(socket_recv(mSocket, buff, sizeof(buff)) > 0) { 
        ;
    }
    mDone=true;
}

void Drainer::closeSocket() {
#ifdef _WIN32
    closesocket(mSocket);
#else
    IGNORE_EINTR(close(mSocket));
#endif
}

//--------------------------- Socket Drain Mgr -----------------------------

SocketDrainMgr::SocketDrainMgr(Looper* looper) :
        mLooper(looper) {
}

SocketDrainMgr::~SocketDrainMgr() {
    drain();
}

void SocketDrainMgr::add(int socket_fd) {
    assert(socket_fd >= 0);
    Drainer* drainer = new Drainer(mLooper, socket_fd);
    if (drainer->done()) {
        delete drainer;
    } else {
        mDrainers.insert(drainer);
    }
}

void SocketDrainMgr::remove(Drainer* drainer) {
    mDrainers.erase(drainer);
    delete drainer;
}

void SocketDrainMgr::drain() {
    for (DrainSet::iterator it = mDrainers.begin(); it != mDrainers.end(); ++it) {
        Drainer* drainer = *it;
        drainer->drain();
        delete drainer;
    }
    mDrainers.clear();
}

} // end of namespace wear
} // end of namespace android

extern "C" {
void start_socket_drain_mgr(Looper* looper) {
    if (!s_socket_drain_mgr) {
        s_socket_drain_mgr = new android::wear::SocketDrainMgr(looper);
    }
}

void socket_drain_mgr_add(int socket_fd) {
    socket_set_nonblock(socket_fd);
    if (s_socket_drain_mgr) {
        (static_cast<android::wear::SocketDrainMgr*>(s_socket_drain_mgr))->add(socket_fd);
    } else {
        //when drain manager is not started, just do a hard shutdown
        android::wear::Drainer drainer(0, socket_fd);
    }
}

void stop_socket_drain_mgr() {
    if (s_socket_drain_mgr) {
        delete (static_cast<android::wear::SocketDrainMgr*>(s_socket_drain_mgr));
        s_socket_drain_mgr = 0;
    }
}
} // end of  extern C



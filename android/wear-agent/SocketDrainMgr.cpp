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


//have to include following to get rid of "INT64_MAX" definition issue
extern "C" {
#include <android/base/Limits.h>
#include <android/base/String.h>
#include "android/android.h"
#include "android/looper.h"
#include "android/sockets.h"
//bohu: notes: following is for LoopIo* related stuff
#include "android/async-utils.h"
#include "android/utils/debug.h"
#include "android/utils/list.h"
#include "android/utils/misc.h"
#define  QB(b, s)  quote_bytes((const char*)b, (s < 32) ? s : 32)
static void _drainer_read(void* opaque, int fd, unsigned events);
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
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#include "android/wear-agent/SocketDrainMgr.h"

static void* s_socket_drain_mgr = 0;

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


namespace android {
namespace wear {

class Drainer {
public:
    Drainer(Looper*looper, int socket_fd);
    ~Drainer();

public:
    void drain();
    bool done() {return mdone;}
    void onRead();

private:
    Looper* mlooper;
    int     msocket;
    LoopIo* mio;
    bool    mdone;

    void shutdownRead();
    void shutdownWrite();
    void closeSocket();
    bool isReadable();
};

Drainer::Drainer(Looper* looper, int socket_fd)
:mlooper(looper)
,msocket(socket_fd)
,mio(0)
,mdone(false)
{
    shutdownWrite();
    if (isReadable()) {
        mio = (LoopIo*)calloc(sizeof(*mio), 1);
        loopIo_init(mio, mlooper, msocket, _drainer_read, this);
        loopIo_wantRead(mio);
        loopIo_dontWantWrite(mio);
    } else {
        //there is nothing to read
        mdone=true;
    }
}

bool
Drainer::isReadable() {
    errno=0;
    char buff[1024];
    int size = socket_recv(msocket, buff, sizeof(buff));
    if (size > 0)
        return true;
    if (size < 0 && errno == EWOULDBLOCK)
        return true;
    return false;
}

Drainer::~Drainer() {
    assert(done());
    shutdownRead();

    if (mio) {
        loopIo_dontWantRead(mio);
        loopIo_done(mio);
        AFREE(mio);
        mio=0;
    }

    closeSocket();
    msocket=-1;

    mlooper=0;
}

void
Drainer::shutdownWrite() {
#ifdef _WIN32
    shutdown(msocket, SD_SEND);
#else
    shutdown(msocket, SHUT_WR);
#endif
}

void
Drainer::shutdownRead() {
#ifdef _WIN32
    shutdown(msocket, SD_RECEIVE);
#else
    shutdown(msocket, SHUT_RD);
#endif
}

void
Drainer::onRead() {

    assert(!done());

    if (isReadable())
        return;
    mdone=true;
}

void
Drainer::drain() {
    //1. set socket blocking 
    socket_set_blocking(msocket);
    //2. read all
    char buff[1024];
    while(socket_recv(msocket, buff, sizeof(buff)) > 0)
        ;
    mdone=true;
}

void
Drainer::closeSocket() {
#ifdef _WIN32
    closesocket(msocket);
#else
    //IGNORE_EINTR(close(msocket));
    close(msocket);
#endif
}




//--------------------------- Socket Drain Mgr -----------------------------

SocketDrainMgr::SocketDrainMgr(Looper* looper)
:mlooper(looper) {
    assert(looper);
}

SocketDrainMgr::~SocketDrainMgr() {
    drain();
}

void
SocketDrainMgr::add(int socket_fd) {
    assert(socket_fd >= 0);
    Drainer* drainer = new Drainer(mlooper, socket_fd);
    if (drainer->done())
        delete drainer;
    else
        mdrainers.insert(drainer);
}

void
SocketDrainMgr::remove(Drainer* drainer) {
    mdrainers.erase(drainer);
    delete drainer;
}

void
SocketDrainMgr::drain() {
    for (DrainSet::iterator it = mdrainers.begin(); it != mdrainers.end(); ++it) {
        Drainer* drainer = *it;
        drainer->drain();
        delete drainer;
    }
    mdrainers.clear();
}

}
}

extern "C" {
void start_socket_drain_mgr(Looper* looper) {
    if (!s_socket_drain_mgr) {
        s_socket_drain_mgr = new android::wear::SocketDrainMgr(looper);
    }
}

void socket_drain_mgr_add(int socket_fd) {
    if (s_socket_drain_mgr) {
        ((android::wear::SocketDrainMgr*)(s_socket_drain_mgr))->add(socket_fd);
    }
}

void stop_socket_drain_mgr() {
    if (s_socket_drain_mgr) {
        delete ((android::wear::SocketDrainMgr*)(s_socket_drain_mgr));
        s_socket_drain_mgr = 0;
    }
}

static void
_drainer_read(void* opaque, int fd, unsigned events) {
    android::wear::Drainer * drainer = 
        (android::wear::Drainer*)opaque;
    assert(drainer);

    if ((events & LOOP_IO_READ) != 0) {
        drainer->onRead();
    }
    if ((events & LOOP_IO_WRITE) != 0) {
        assert(0);
    }

    if (drainer->done())
        ((android::wear::SocketDrainMgr*)(s_socket_drain_mgr))->remove(drainer);
}

}












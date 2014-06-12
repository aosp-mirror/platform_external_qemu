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
#include "android/wear-agent/SocketDrainer.h"

extern "C" {
#include "android/looper.h"
#include "android/sockets.h"
#include "android/utils/eintr_wrapper.h"
#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#endif
}

#include <assert.h>
#include <unistd.h>
#include <set>

// Some implementation whys:
// When the looper is running, the sockets are non-blocking and are only
// closed after all data has been read.
// When looper stops, the remaining sockets are still non-blocking and are
// closed after all available data has been read (cannot set to blocking, because
// doing that could put the program sleeping forever).
// When socket is already closed for read, we only need to shutdown writing
// and close the socket.

static android::wear::SocketDrainer *s_socket_drainer = 0;


namespace android {
namespace wear {

class Drainer;

class SocketDrainerImpl {

public:
    SocketDrainerImpl(Looper* looper);
    ~SocketDrainerImpl();

public:

    // Add a socket to be properly closed
    void add(int socket_fd);

    void remove(Drainer* drainer);

private:

    Looper*  mLooper;
    typedef std::set<Drainer*> DrainSet;

    DrainSet mDrainers;

    // Drain all the remaining sockets
    void drain();

};

class Drainer {
public:
    Drainer(Looper* looper, int socket_fd, SocketDrainerImpl* parent = 0);
    ~Drainer();

public:
    void drain();
    bool done() const { return mDone; }
    void onRead();
    void removeFromParent() {
        assert(mParent);
        mParent->remove(this);
    }
private:
    Looper* mLooper;
    int     mSocket;
    LoopIo  mIo[1];
    bool    mDone;
    SocketDrainerImpl* mParent;

    void shutdownRead();
    void shutdownWrite();
    void closeSocket();

    // return true if can read data
    bool readData();
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
        drainer->removeFromParent();
        delete drainer;
    }
}

Drainer::Drainer(Looper* looper, int socket_fd, SocketDrainerImpl* parent) :
        mLooper(looper), mSocket(socket_fd), mIo(), mDone(false), mParent(parent) {
    shutdownWrite();
    if (readData() && mLooper) {
        loopIo_init(mIo, mLooper, mSocket, _drainer_read, this);
        loopIo_wantRead(mIo);
        loopIo_dontWantWrite(mIo);
    } else {
        // there is nothing to read
        mLooper=0;
        mDone=true;
    }
}

bool Drainer::readData() {
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
    mParent = 0;
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
    if (readData()) {
        return;
    }
    mDone=true;
}

void Drainer::drain() {
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

SocketDrainerImpl::SocketDrainerImpl(Looper* looper) :
        mLooper(looper) {
    assert(looper);
}

SocketDrainerImpl::~SocketDrainerImpl() {
    drain();
}

void SocketDrainerImpl::add(int socket_fd) {
    assert(socket_fd >= 0);
    Drainer* drainer = new Drainer(mLooper, socket_fd, this);
    if (drainer->done()) {
        delete drainer;
    } else {
        mDrainers.insert(drainer);
    }
}

void SocketDrainerImpl::remove(Drainer* drainer) {
    mDrainers.erase(drainer);
}

void SocketDrainerImpl::drain() {
    for (DrainSet::iterator it = mDrainers.begin(); it != mDrainers.end(); ++it) {
        Drainer* drainer = *it;
        drainer->drain();
        delete drainer;
    }
    mDrainers.clear();
}

SocketDrainer::SocketDrainer(Looper* looper) :
    mSocketDrainerImpl(new SocketDrainerImpl(looper)) {
}

SocketDrainer::~SocketDrainer() {
    delete mSocketDrainerImpl;
}

void
SocketDrainer::add(int socket_fd) {
    assert(mSocketDrainerImpl);
    mSocketDrainerImpl->add(socket_fd);
}

} // namespace wear
} // namespace android

extern "C" {
void start_socket_drainer(Looper* looper) {
    if (!s_socket_drainer) {
        s_socket_drainer = new android::wear::SocketDrainer(looper);
    }
}

void socket_drainer_add(int socket_fd) {
    socket_set_nonblock(socket_fd);
    if (s_socket_drainer) {
        s_socket_drainer->add(socket_fd);
    } else {
        // when drain manager is not started, just do a hard shutdown
        android::wear::Drainer drainer(0, socket_fd);
    }
}

void stop_socket_drainer() {
    if (s_socket_drainer) {
        delete s_socket_drainer;
        s_socket_drainer = 0;
    }
}

} // extern "C"

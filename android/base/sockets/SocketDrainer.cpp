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
#include "android/base/sockets/SocketDrainer.h"

#include "android/sockets.h"
#include "android/utils/eintr_wrapper.h"
#ifdef _WIN32
#  include <winsock2.h>
#else
#  include <sys/socket.h>
#endif

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

static android::base::SocketDrainer *s_socket_drainer = 0;

namespace android {
namespace base {

class DrainerObject;

// SocketDrainImpl implements the SocketDrainer and manages all the DrainerObjects
class SocketDrainerImpl {
public:
    SocketDrainerImpl(Looper* looper) : mLooper(looper), mDrainerObjects() { }
    ~SocketDrainerImpl() { drainAndCloseAllSockets(); }
public:
    void addSocketToDrain(int socket_fd);
    void removeDrainerObject(DrainerObject* drainer) { mDrainerObjects.erase(drainer); }
private:
    Looper*  mLooper;
    typedef std::set<DrainerObject*> DrainSet;
    DrainSet mDrainerObjects;

    void drainAndCloseAllSockets();
};

// DrainerObject drains and closes socket
class DrainerObject {
public:
    DrainerObject(int socket_fd, Looper* looper, SocketDrainerImpl* parent);
    ~DrainerObject();
public:
    // drain socket and return true if there is still more data to drain
    bool drainSocket();

    // return true when there is no data to drain
    bool socketIsDrained() const { return mSocketIsDrained; }

    // remove from SocketDrainerImpl
    void removeFromParent() { if(mParent) mParent->removeDrainerObject(this); }
private:
    int     mSocket;
    Looper* mLooper;
    SocketDrainerImpl* mParent;
    LoopIo  mIo[1];
    bool    mSocketIsDrained;

    void shutdownRead();
    void shutdownWrite();
    void closeSocket();
};

// callback from looper when the socket_fd has some data ready to read
static void _on_read_socket_fd(void* opaque, int fd, unsigned events) {
    DrainerObject * drainerObject = (DrainerObject*)opaque;
    if (!drainerObject) return;
    if ((events & LOOP_IO_READ) != 0) {
        drainerObject->drainSocket();
    }
    if (drainerObject->socketIsDrained()) {
        drainerObject->removeFromParent();
        delete drainerObject;
    }
}

DrainerObject::DrainerObject(int socket_fd, Looper* looper, SocketDrainerImpl* parent) :
        mSocket(socket_fd), mLooper(looper), mParent(parent), mIo(), mSocketIsDrained(false) {
    shutdownWrite();
    if (drainSocket() && mLooper && mParent) {
        loopIo_init(mIo, mLooper, mSocket, _on_read_socket_fd, this);
        loopIo_wantRead(mIo);
        loopIo_dontWantWrite(mIo);
    } else {
        // there is nothing to read, the drainer object is done
        mLooper = 0;
    }
}

DrainerObject::~DrainerObject() {
    if (!mSocketIsDrained) {
        char buff[1024];
        while(socket_recv(mSocket, buff, sizeof(buff)) > 0) { ; }
        mSocketIsDrained = true;
    }
    shutdownRead();
    if (mLooper) {
        loopIo_dontWantRead(mIo);
        loopIo_done(mIo);
        mLooper = 0;
    }
    closeSocket();
    mSocket = -1;
    mParent = 0;
}

bool DrainerObject::drainSocket() {
    errno = 0;
    char buff[1024];
    int size = socket_recv(mSocket, buff, sizeof(buff));
    if (size > 0) {
        return true;
    } else if (size < 0 && errno == EWOULDBLOCK) {
        return true;
    }
    mSocketIsDrained = true;
    return false;
}

void DrainerObject::shutdownWrite() {
#ifdef _WIN32
    shutdown(mSocket, SD_SEND);
#else
    shutdown(mSocket, SHUT_WR);
#endif
}

void DrainerObject::shutdownRead() {
#ifdef _WIN32
    shutdown(mSocket, SD_RECEIVE);
#else
    shutdown(mSocket, SHUT_RD);
#endif
}

void DrainerObject::closeSocket() {
#ifdef _WIN32
    closesocket(mSocket);
#else
    IGNORE_EINTR(close(mSocket));
#endif
}

//--------------------------- SocketDrainerImpl Implementation -------------------------

void SocketDrainerImpl::addSocketToDrain(int socket_fd) {
    DrainerObject* drainer = new DrainerObject(socket_fd, mLooper, this);
    if (drainer->socketIsDrained()) {
        delete drainer;
    } else {
        mDrainerObjects.insert(drainer);
    }
}

void SocketDrainerImpl::drainAndCloseAllSockets() {
    for (DrainSet::iterator it = mDrainerObjects.begin(); it != mDrainerObjects.end(); ++it) {
        DrainerObject* drainer = *it;
        delete drainer;
    }
    mDrainerObjects.clear();
}

//--------------------------- SocketDrainer Implementation -----------------------------

SocketDrainer::SocketDrainer(Looper* looper) :
    mSocketDrainerImpl(new SocketDrainerImpl(looper)) {
}

SocketDrainer::~SocketDrainer() {
    delete mSocketDrainerImpl;
    mSocketDrainerImpl = 0;
}

void
SocketDrainer::drainAndClose(int socket_fd) {
    if (socket_fd < 0) return;
    mSocketDrainerImpl->addSocketToDrain(socket_fd);
}

} // namespace base
} // namespace android

// -------------------- extern C functions ---------------------------------------------

void socket_drainer_start(Looper* looper) {
    if (!looper) return;
    if (!s_socket_drainer) s_socket_drainer = new android::base::SocketDrainer(looper);
}

void socket_drainer_drain_and_close(int socket_fd) {
    if (socket_fd < 0) return;
    socket_set_nonblock(socket_fd);
    if (s_socket_drainer) {
        s_socket_drainer->drainAndClose(socket_fd);
    } else {
        android::base::DrainerObject drainerObject(socket_fd, 0, 0);
    }
}

void socket_drainer_stop() {
    if (s_socket_drainer) {
        delete s_socket_drainer;
        s_socket_drainer = 0;
    }
}

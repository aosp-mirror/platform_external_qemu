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

#ifndef ANDROID_BASE_SOCKETS_SCOPED_SOCKET_H
#define ANDROID_BASE_SOCKETS_SCOPED_SOCKET_H

#include "android/base/Compiler.h"
#include "android/base/sockets/SocketUtils.h"

namespace android {
namespace base {

class ScopedSocket {
public:
    ScopedSocket() : mSocket(-1) {}

    ScopedSocket(int socket) : mSocket(socket) {}

    ~ScopedSocket() {
        close();
    }

    bool valid() const { return mSocket >= 0; }

    int get() const { return mSocket; }

    void close() {
        if (mSocket >= 0) {
            socketClose(mSocket);
            mSocket = -1;
        }
    }

    int release() {
        int result = mSocket;
        mSocket = -1;
        return result;
    }

    void reset(int socket) {
        int oldSocket = mSocket;
        mSocket = socket;
        if (oldSocket >= 0) {
            socketClose(oldSocket);
        }
    }

    void swap(ScopedSocket* other) {
        int tmp = mSocket;
        mSocket = other->mSocket;
        other->mSocket = tmp;
    }

private:
    int mSocket;
};

}  // namespace base
}  // namespace android

#endif  // ANDROID_BASE_SOCKETS_SCOPED_SOCKET_H

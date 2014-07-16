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

#ifndef ANDROID_BASE_SOCKETS_SOCKET_DRAINER_H
#define ANDROID_BASE_SOCKETS_SOCKET_DRAINER_H

#include <android/utils/compiler.h>

/*
This class is used to manage graceful closure of sockets, as described by
"The ultimate SO_LINGER page", which requires several steps to do properly.

To use this class from C code, call the following after creating a looper:
   socket_drainer_start(looper);

When a socket needs to be gracefully closed, call the following:
   socket_drainer_drain_and_close(socket_fd);

After looper_run completes, call the following:
   socket_drainer_stop();

Note: the "socket_drainer_drain_and_close" function can be called before
"socket_drainer_start" or after "socket_drainer_stop" . In those cases,
the socket is simply shut down and closed without receiving all the pending data.

For C++ code, simply create an object as follows:
   SocketDrainer socketDrainer(looper);

When a socket needs to be gracefully closed, call the following:
   socketDrainer.drainAndClose(socket_fd);

When the socketDrainer is destroyed (for example, falling out of scope), all remaining
sockets will be shut down and closed without futher draining.

*/

ANDROID_BEGIN_HEADER

#include "android/looper.h"

void socket_drainer_start(Looper* looper);
void socket_drainer_drain_and_close(int socket_fd);
void socket_drainer_stop();

ANDROID_END_HEADER

#ifdef __cplusplus
namespace android {
namespace base {

class SocketDrainerImpl;
class SocketDrainer {
public:
    SocketDrainer(Looper* looper);
    ~SocketDrainer();
    // Add a socket to be drained and closed
    void drainAndClose(int socket_fd);
private:
    SocketDrainerImpl*  mSocketDrainerImpl;
};

} // namespace base
} // namespace android
#endif
#endif

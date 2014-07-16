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

#ifndef ANDROID_SOCKET_DRAINER_H
#define ANDROID_SOCKET_DRAINER_H

/*

This class is used to manage graceful closure of sockets, as described by
"The ultimate SO_LINGER page", which requires several steps to do properly.

To use this class from C code, call following after creating a looper:
   start_socket_drainer(looper);

When a socket needs to be gracefully closed, call following:
   socket_drainer_add (socket_fd);

Note: the above function can be called before calling start_socket_drainer(looper),
or after calling stop_socket_drainer(). In those cases, the socket is simply shut down
and closed without receiving all the pending data.

After looper_run completes, call following:
   stop_socket_drainer();

For C++ code, simply create an object as follows:
   SocketDrainer drainManager(looper);

When a socket needs to be gracefully closed, call following:
   drainManager.add(socket_fd);

*/


#ifdef __cplusplus

struct Looper;

namespace android {
namespace base {
class SocketDrainerImpl;

class SocketDrainer {

public:
    SocketDrainer(Looper* looper);
    ~SocketDrainer();

    // Add a socket to be properly closed
    void add(int socket_fd);
private:
    SocketDrainerImpl*  mSocketDrainerImpl;
};

} // namespace base
} // namespace android

extern "C" {
#endif

#include "android/looper.h"

void start_socket_drainer(Looper* looper);
void socket_drainer_add(int socket_fd);
void stop_socket_drainer();

#ifdef __cplusplus
}
#endif


#endif



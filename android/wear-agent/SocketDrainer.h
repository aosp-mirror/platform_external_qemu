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

#ifndef ANDROID_WEAR_AGENT_SOCKET_DRAINER_H
#define ANDROID_WEAR_AGENT_SOCKET_DRAINER_H

struct Looper;

#include <set>

namespace android {
namespace wear {

class Drainer;

/*

This class is used to manage graceful closure of sockets, as described by
"The ultimate SO_LINGER page", which requires several steps to do properly.

To use this class from C code:
// After creating a looper, call following
   start_socket_drainer(looper);
// After looper_run completes, call following
   stop_socket_drainer();

// For C++ code, simply create an object as follows:

 SocketDrainer drainManager(looper);

*/

class SocketDrainer {

public:
    SocketDrainer(Looper* looper);
    ~SocketDrainer();

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

} // namespace wear
} // end of namespace android

#endif



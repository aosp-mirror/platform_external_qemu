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

#ifndef ANDROID_SOCKET_DRAIN_MGR_H
#define ANDROID_SOCKET_DRAIN_MGR_H

struct Looper;

#include <set>

namespace android {
namespace wear {

class Drainer;

//this object will stop sending data to socket, then, read all the remaining data 
//in the socket, and finally, close the socket
//to use this class from C code:
//
//--after creating a looper, call following
//
// start_socket_drain_mgr(looper);
//
//--after looper_run completes, call following
//
// stop_socket_drain_mgr();
//
//--for C++ code, simply create an object as follows:
//
// SocketDrainMgr drainMgr(looper);
//
//
class SocketDrainMgr {

public:
    SocketDrainMgr(Looper* looper);
    ~SocketDrainMgr();

public:

    //add a socket to be properly closed
    void add(int socke_fd);
    
    void remove(Drainer* drainer);

private:

    Looper*     mLooper;
    typedef std::set<Drainer*> DrainSet;

    DrainSet        mDrainers;

    //drain all the remaining sockets
    void drain();

}; 

} //namespace wear
}

#endif



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

#ifndef ANDROID_WEAR_AGENT_H
#define ANDROID_WEAR_AGENT_H

//#include <stdint.h>
//#include <stddef.h>

struct Looper;
struct LoopIo;
struct LoopTimer;


namespace android {
namespace wear {

class PairUpWearPhone;
// This class establishes connection between one wear and one phone,
// so that the wear can get notifications automatically from the phone.
// The phone could be emulated phone or usb connected physical phone.
//
// To use this class, simply create an instance as follows
// and it runs right away.
//
// WearAgent agent(looper);
//
class WearAgent {
public:
    WearAgent(Looper*  looper, int adbhostport=5037);
    ~WearAgent();

public:
    //TODO: hide following methods: they are public
    //because LoopIo and LoopTimer callback functions
    //have to invoke them.
    void registerForDevicesUpdate();
    void onDevicesUpdateFromHost();
    void makeConnection();

private:
    //listening socket for devices update
    int         mport;
    int         mso;
    Looper*     mlooper;
    LoopIo*     mio;
    LoopTimer*  mtimer;
    int         msec;

    PairUpWearPhone     *mpairup;
private:
    void cleanupConnection();
    void connect_later();
};

}  // namespace wear
}  // namespace android

#endif  // ANDROID_WEAR_AGENT_H

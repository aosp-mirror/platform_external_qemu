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

#ifndef ANDROID_PAIR_UP_WEAR_PHONE_H
#define ANDROID_PAIR_UP_WEAR_PHONE_H

#include <string>
#include <vector>

struct Looper;
struct LoopIo;
struct LoopTimer;

namespace android {
namespace wear {

class PairUpWearPhoneImpl;

// This class parses the 'devices' message and queries the properties of 
// all the wears and phones. Then, it tries to setup ip redirection on 
// the first phone so that the first wear can get notifications automatically 
// from the first phone. The phone could be emulated phone or usb connected 
// physical phone.
//
// To use this class, simply create an instance as follows
//
// PairUpWearPhone pairup (looper);
// 
// whenever there is a new list of devices from adb host server, 
// simply call following to re-pairup wear and phone
//
// pairup.connectWearToPhone(devices);
//

class PairUpWearPhone {
public:
    PairUpWearPhone(Looper* looper, int adbHostPort=5037);
    ~PairUpWearPhone();

    void connectWearToPhone(const char* devices);

private:
    PairUpWearPhoneImpl*    mPairUpWearPhoneImpl;
};


} //namespace wear
}

#endif

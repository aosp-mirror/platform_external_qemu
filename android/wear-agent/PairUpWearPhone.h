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

#ifndef ANDROID_WEAR_AGENT_PAIR_UP_WEAR_PHONE_H
#define ANDROID_WEAR_AGENT_PAIR_UP_WEAR_PHONE_H

struct Looper;

namespace android {
namespace wear {

/*
    When both a phone and a wear are listed at the same time, this class will
automatically setup a network redirection to ensure the wear can receive notifications
automatically.
    For simplicity, this only connects the first compatible phone on the list,
with wear in it. The 'phone' could be emulator or connected through USB.
    To use this class, simply create an instance as follows:
PairUpWearPhone pairup (looper, devicelist);

*/

class PairUpWearPhone {
public:
    // devicelist comes from adb server, for example, in the following format:
    // "emulator-5554 device 070fe93a0b2c1fdb device"
    PairUpWearPhone(Looper* looper, const char* devicelist, int adbHostPort=5037);
    ~PairUpWearPhone();
};


} // namespace wear
} // namespace android

#endif

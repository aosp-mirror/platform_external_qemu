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

#include <string>
#include <vector>

struct Looper;

namespace android {
namespace wear {

/*
   This object will set up connection between the first pair of compatible
wear and phone on the "deviceList" (which comes from "adb devices" command),
so that the wear can receive notification from the phone.

    To use this class, simply create an instance as follows:

PairUpWearPhone pairup = new PairUpWearPhone(looper, devices);

    Once the pairup->isDone() is true, pairup can be safely deleted; otherwise,
the pair up might not be completed.

*/

class PairUpWearPhoneImpl;

class PairUpWearPhone {
public:
    // device could be emulator or real device, for example,
    // emulator-5554  0123456789abcdef ...
    PairUpWearPhone(Looper* looper, const std::vector<std::string>& devices, int adbHostPort);
    ~PairUpWearPhone();

    // returns true if the object has completed its task and can be safely deleted.
    bool isDone() const;
private:
    PairUpWearPhoneImpl*    mPairUpWearPhoneImpl;
};


} // namespace wear
} // namespace android

#endif

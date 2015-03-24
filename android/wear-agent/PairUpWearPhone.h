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

#include "android/base/Compiler.h"

namespace android {

namespace base {
class Looper;
class StringVector;
}  // namespace base

namespace wear {

/*
 *  This object sets up connection between the first pair of compatible
 *  wear and phone in the "devices" (which comes from "adb devices" command),
 *  so that the wear can receive notification from the phone.
 *
 *  To use this class, simply create an instance as follows:
 *
 *  PairUpWearPhone *pairup = new PairUpWearPhone(looper, devices);
 *
 *  It is advised to delete the pairup object only after pairup->isDone()
 *  returns true to avoid aborting the pairing up process.
 */

class PairUpWearPhoneImpl;

class PairUpWearPhone {
public:
    // "devices" is a list of serial-ids, could contain both emulators and real devices.
    PairUpWearPhone(::android::base::Looper* looper,
                    const ::android::base::StringVector& devices,
                    int adbHostPort);

    ~PairUpWearPhone();

    // returns true if the object has completed its task and can be safely deleted.
    bool isDone() const;
private:
    DISALLOW_COPY_AND_ASSIGN(PairUpWearPhone);

    PairUpWearPhoneImpl* mPairUpWearPhoneImpl;
};


} // namespace wear
} // namespace android

#endif

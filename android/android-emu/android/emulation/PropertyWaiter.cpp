// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/PropertyWaiter.h"
#include "android/utils/debug.h"

#define D(...) VERBOSE_TID_DPRINT(aprops, __VA_ARGS__)
#define DF(...) VERBOSE_TID_FUNCTION_DPRINT(aprops, __VA_ARGS__)

using android::base::AutoLock;
using android::base::StringView;
using android::base::System;

namespace android {
namespace emulation {

PropertyWaiter::PropertyWaiter(StringView key, StringView value) :
        mKey(key),
        mValue(value) {
    DF("Created");
    mWaitSignal.done = false;
}

PropertyWaiter::~PropertyWaiter() {
    DF("Destroyed");
}

const std::string& PropertyWaiter::getKey() {
    return mKey;
}

const std::string& PropertyWaiter::getValue() {
    return mValue;
}

void PropertyWaiter::setKey(StringView key) {
    mKey = key;
}

void PropertyWaiter::setValue(StringView value) {
    mValue = value;
}

bool PropertyWaiter::wait() {
    DF("Waiting for key=%s, val=%s", mKey.c_str(), mValue.c_str());
    AutoLock lock(mWaitSignal.lock);
    while (!mWaitSignal.done) {
        mWaitSignal.cvDone.wait(&mWaitSignal.lock);
    }
    DF("Unblocked, got key=%s, value=%s", mKey.c_str(), mValue.c_str());
    mWaitSignal.done = false;
    return mWaitSignal.found;
}

void PropertyWaiter::signal(bool found) {
    AutoLock lock(mWaitSignal.lock);
    mWaitSignal.found = found;
    mWaitSignal.done = true;
    DF("Sending signal...");
    mWaitSignal.cvDone.signalAndUnlock(&lock);
}

}  // namespace emulation
}  // namespace android

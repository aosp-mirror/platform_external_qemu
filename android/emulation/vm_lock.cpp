// Copyright 2016 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/vm_lock.h"

#include "android/emulation/VmLock.h"

#include <stddef.h>

using android::base::VmLock;

namespace {

class CustomVmLock : public VmLock {
public:
    CustomVmLock(AndroidVmLockFunc lock, AndroidVmLockFunc unlock) :
            mLock(lock), mUnlock(unlock) {}

    virtual void lock() override {
        if (mLock) mLock();
    }

    virtual void unlock() override {
        if (mUnlock) mUnlock();
    }
private:
    AndroidVmLockFunc mLock;
    AndroidVmLockFunc mUnlock;
};

}  // namespace

void android_vm_lock(void) {
    VmLock::get()->lock();
}

void android_vm_unlock(void) {
    VmLock::get()->unlock();
}

void android_vm_set_lock_funcs(AndroidVmLockFunc lock,
                               AndroidVmLockFunc unlock) {
    VmLock* old = VmLock::set(new CustomVmLock(lock, unlock));
    delete old;
}

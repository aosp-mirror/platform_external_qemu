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

#include "android-qemu2-glue/emulation/VmLock.h"

#include "android/base/Log.h"

extern "C" {
#include "qemu/main-loop.h"
}  // extern "C"

namespace qemu2 {

// TECHNICAL NOTE:
//
// This implementation needs to protect against recursive lock() calls
// which can happen because some code in AndroidEmu calls it, without
// knowing whether it's running in the thread that holds the BQL or not.
//
// What we want is to ensure that qemu_mutex_lock_iothread() is always true
// when we leave ::lock(), and that it will be false when ::unlock() has
// been called as often as ::lock() was.

void VmLock::lock() {
    qemu_mutex_lock_iothread();
}

void VmLock::unlock() {
    qemu_mutex_unlock_iothread();
}

bool VmLock::isLockedBySelf() const {
    return qemu_mutex_check_iothread();
}

}  // namespace qemu2

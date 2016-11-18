// Copyright 2015 The Android Open Source Project
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

#include "android-qemu1-glue/emulation/VmLock.h"

extern "C" {
#include "qemu-common.h"
}  // extern "C"

namespace qemu {

// TECHNICAL NOTE:
// There is no need to protect against recursive mutex locks since
// everything of interest happens in the main loop thread.

void VmLock::lock() { qemu_mutex_lock_iothread(); }

void VmLock::unlock() { qemu_mutex_unlock_iothread(); }

bool VmLock::isLockedBySelf() const { return qemu_mutex_check_iothread(); }

}  // namespace qemu

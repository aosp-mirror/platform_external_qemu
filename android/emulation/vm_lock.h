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

#pragma once

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// In QEMU2, each virtual CPU runs on its own host threads, but all these
// threads are synchronized through a global mutex, which allows the virtual
// device code to not care about them.
//
// However, if you have to call, from any other thread, a low-level QEMU
// function that operate on virtual devices (e.g. some Android pipe-related
// functions), you must acquire the global mutex before doing so, and release
// it after that.

// This header provides android_vm_lock() and android_vm_unlock() to do just
// that. The default implementation doesn't do anything, but glue code can
// provide its own callback by using android_vm_set_lock_funcs().

// Lock the global VM mutex.
void android_vm_lock(void);

// Unlock the global VM mutex.
void android_vm_unlock(void);

// Type of a VM lock/unlock function.
typedef void (*AndroidVmLockFunc)(void);

// Set the global mutex lock/unlock functions, must be called at initialization
// time by the glue code before any virtual CPU runs. |lock| and |unlock| are
// simple functions that acquire and release the VM-specific mutex. NULL values
// indicate there is nothing to do.
void android_vm_set_lock_funcs(AndroidVmLockFunc lock,
                               AndroidVmLockFunc unlock);

ANDROID_END_HEADER

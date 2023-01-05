// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/test/checkboot.h"

#include <stdio.h>                                    // for printf
#include <functional>                                 // for __base

#include "aemu/base/async/ThreadLooper.h"          // for ThreadLooper
#include "android/base/system/System.h"               // for System
#include "aemu/base/threads/ParallelTask.h"        // for runParallelTask
#include "android/console.h"                          // for getConsoleAgents
#include "host-common/vm_operations.h"  // for QAndroidVmOpera...

static int s_SleepTime = 0;

static void myTask(int * outResult) {
    *outResult = 0;
    android::base::System::get()->sleepMs(s_SleepTime * 1000);
    *outResult = 1;
}

static void myTaskDone(const int& outResult) {
    // quit emulator if it is not dead yet
    printf("emulator: ERROR: fail to boot after %d seconds, quit\n", s_SleepTime);
    getConsoleAgents()->vm->vmShutdown();
}

bool android_test_start_boot_complete_timer(int time_out_seconds) {
    s_SleepTime = time_out_seconds;
    return android::base::runParallelTask<int>(android::base::ThreadLooper::get(),
            &myTask, &myTaskDone);
}



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

#include <chrono>
#include <thread>

#include "aemu/base/async/ThreadLooper.h"
#include "aemu/base/logging/CLog.h"
#include "aemu/base/threads/ParallelTask.h"
#include "android/console.h"

static int s_SleepTime = 0;

static void myTask(int* outResult) {
    *outResult = 0;
    std::this_thread::sleep_for(std::chrono::seconds(s_SleepTime));
    *outResult = 1;
}

static void myTaskDone(const int& outResult) {
    // quit emulator if it is not dead yet
    derror("Failed to boot after %d seconds, shutting down", s_SleepTime);
    getConsoleAgents()->vm->vmShutdown();
}

bool android_test_start_boot_complete_timer(int time_out_seconds) {
    s_SleepTime = time_out_seconds;
    return android::base::runParallelTask<int>(
            android::base::ThreadLooper::get(), &myTask, &myTaskDone);
}

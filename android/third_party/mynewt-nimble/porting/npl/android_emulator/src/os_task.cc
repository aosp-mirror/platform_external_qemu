// Copyright (C) 2021 The Android Open Source Project
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

// This contains abstractions to run NimBLE against rootcanal (emulated
// bluetooth chip)

// These must come first, or win32 will not compile.
#include "aemu/base/async/Looper.h"
#include "aemu/base/async/ThreadLooper.h"

#include <stddef.h>  // for NULL
#include <stdint.h>  // for uint8_t, uint16_t
#include <thread>    // for yield, thread

#include "aemu/base/threads/Thread.h"  // for getCurrentThreadId
#include "nimble/nimble_npl.h"            // for ble_npl_get_current_task_id
#include "nimble/os_types.h"              // for ble_npl_task_func_t, ble_np...
#include "os/os.h"                        // for OS_INVALID_PARM

#ifdef _WIN32
#include "aemu/base/system/Win32UnicodeString.h"
#else
#include <pthread.h>  // for pthread_setname_np
#endif



using android::base::Looper;
using android::base::ThreadLooper;

extern "C" {

/**
 * Initialize a task.
 *
 * This function initializes the task structure pointed to by t,
 * clearing and setting it's stack pointer, provides sane defaults
 * and sets the task as ready to run, and inserts it into the operating
 * system scheduler.
 *
 * @param t The task to initialize
 * @param name The name of the task to initialize
 * @param func The task function to call
 * @param arg The argument to pass to this task function
 * @param prio The priority at which to run this task
 * @param sanity_itvl The time at which this task should check in with the
 *                    sanity task.  OS_WAIT_FOREVER means never check in
 *                    here. (IGNORED)
 * @param stack_bottom A pointer to the bottom of a task's stack
 * @param stack_size The overall size of the task's stack.
 *
 * @return 0 on success, non-zero on failure.
 */
int ble_npl_task_init(struct ble_npl_task* t,
                      const char* name,
                      ble_npl_task_func_t func,
                      void* arg,
                      uint8_t prio,
                      ble_npl_time_t sanity_itvl,
                      ble_npl_stack_t* stack_bottom,
                      uint16_t stack_size) {
    if ((t == NULL) || (func == NULL)) {
        return OS_INVALID_PARM;
    }

    auto looper = ThreadLooper::get();

    t->handle = new std::thread([looper, name, func, arg]() {
        ThreadLooper::setLooper(looper);
#ifdef __APPLE__
        pthread_setname_np(name);
#elif defined(__linux__)
        pthread_setname_np(pthread_self(), name);
#elif defined(_WIN32)
        const android::base::Win32UnicodeString threadName(name);
        SetThreadDescription(GetCurrentThread(), threadName.c_str());
#endif
        func(arg);
    });
    return 0;
}

/*
 * Removes specified task
 * XXX (Unused!)
 * NOTE: This interface is currently experimental and not ready for common use
 */
int ble_npl_task_remove(struct ble_npl_task* t) {
    // std::thread* thread = reinterpret_cast<std::thread*>(t->handle);
    return 0;  // pthread_cancel(t->handle);
}

/**
 * Return the number of tasks initialized.
 *
 * @return number of tasks initialized
 */
uint8_t ble_npl_task_count(void) {
    return 0;
}

void* ble_npl_get_current_task_id(void) {
    return (void*)android::base::getCurrentThreadId();
}

bool ble_npl_os_started(void) {
    return true;
}

void ble_npl_task_yield(void) {
    std::this_thread::yield();
}
}

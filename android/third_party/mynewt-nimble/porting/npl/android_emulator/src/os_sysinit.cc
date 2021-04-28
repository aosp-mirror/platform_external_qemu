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

// This contains the system initialization routines.

#include "android/base/async/RecurrentTask.h"
#include "android/base/async/ThreadLooper.h"
extern "C" {
#include "nimble/nimble_npl.h"
#include "nimble/nimble_port.h"
#include "nimble/os_types.h"
#include "socket/ble_hci_socket.h"
}

using android::base::Looper;
using android::base::RecurrentTask;
using android::base::ThreadLooper;
#define TASK_DEFAULT_PRIORITY 1
#define TASK_DEFAULT_STACK NULL
#define TASK_DEFAULT_STACK_SIZE 400

static struct ble_npl_task s_task_hci;
static struct ble_npl_task s_task_looper;

void* ble_hci_sock_task(void* param) {
    ble_hci_sock_ack_handler(param);
    return NULL;
}

void* ble_looper_task(void* param) {
    auto heartbeat = RecurrentTask(
            ThreadLooper::get(), []() { return true; }, 1000);
    heartbeat.start();
    while (true) {
        ThreadLooper::get()->run();
    }
}

extern "C" void sysinit(void) {
    // Make sure we have a looper object, we will run the looper ourselves.

    ThreadLooper::get();
    nimble_port_init();


    /* Setup the socket to rootcanal */
    // TODO(jansene): Make this properly configurable vs the hardcoded socket.
    ble_hci_sock_init();

    // We need a thread to push data into the nimble stack.
    ble_npl_task_init(&s_task_hci, "hci_sock", ble_hci_sock_task, NULL,
                      TASK_DEFAULT_PRIORITY, BLE_NPL_TIME_FOREVER,
                      TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);

    // Let's setup our own looper.
    // TODO(jansene): QEMU provides its own looper..
    ble_npl_task_init(&s_task_looper, "looper thread", ble_looper_task, NULL,
                      TASK_DEFAULT_PRIORITY, BLE_NPL_TIME_FOREVER,
                      TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);
}

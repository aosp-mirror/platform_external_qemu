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

#include "aemu/base/async/RecurrentTask.h"
#include "aemu/base/async/ThreadLooper.h"
#include <atomic>
extern "C" {
#include "nimble/nimble_npl.h"
#include "nimble/nimble_port.h"
#include "nimble/os_types.h"
#include "host/ble_hs.h"

extern void os_msys_init(void);
extern void os_mempool_module_init(void);
extern void ble_store_config_init(void);
extern int ble_hs_reset(void);
extern void ble_hci_transport_init(void);
}



using android::base::Looper;
using android::base::RecurrentTask;
using android::base::ThreadLooper;
#define TASK_DEFAULT_PRIORITY 1
#define TASK_DEFAULT_STACK NULL
#define TASK_DEFAULT_STACK_SIZE 400

static struct ble_npl_task s_task_hci;
static struct ble_npl_task s_task_looper;

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

    static std::atomic_bool initialized{false};
    bool expected = false;
    if (!initialized.compare_exchange_strong(expected, true)) {
        dinfo("System already initialized..");
        ble_hs_init();
        return;
    }

    ThreadLooper::get();
    nimble_port_init();

    /* Setup the socket to rootcanal */
    // TODO(jansene): Make this properly configurable vs the hardcoded socket.
    ble_hci_transport_init();

    // Let's setup our own looper.
    // TODO(jansene): QEMU provides its own looper..
    ble_npl_task_init(&s_task_looper, "looper thread", ble_looper_task, NULL,
                      TASK_DEFAULT_PRIORITY, BLE_NPL_TIME_FOREVER,
                      TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);
}


extern "C" void sysdown_reset(void) {

}

extern "C"  void
nimble_port_init(void)
{
    /* Initialize default event queue */
   // ble_npl_eventq_init(&g_eventq_dflt);
    /* Initialize the global memory pool */
    os_mempool_module_init();
    os_msys_init();
    /* Initialize the host */
    ble_hs_init();


    /* Setup the store */
    ble_store_config_init();

#if NIMBLE_CFG_CONTROLLER
    ble_hci_ram_init();
    hal_timer_init(5, NULL);
    os_cputime_init(32768);
    ble_ll_init();
#endif
}

// You can call this to pump the default message pump.
extern "C"  void
nimble_port_run(void)
{
    struct ble_npl_event *ev;

    while (1) {
        ev = ble_npl_eventq_get(os_eventq_dflt_get(), BLE_NPL_TIME_FOREVER);
        ble_npl_event_run(ev);
    }
}

extern "C"  struct ble_npl_eventq *
nimble_port_get_dflt_eventq(void)
{
    return os_eventq_dflt_get();
}

#if NIMBLE_CFG_CONTROLLER
extern "C"  void
nimble_port_ll_task_func(void *arg)
{
    extern void ble_ll_task(void *);

    ble_ll_task(arg);
}
#endif

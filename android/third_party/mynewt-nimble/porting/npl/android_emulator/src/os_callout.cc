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

// This implementation relies on the android-emu-base loopers.
// You must have an active thread looper available.

#include <stdint.h>                           // for uint32_t
#include <stdio.h>                            // for NULL
#include <string.h>                           // for memset

#include "aemu/base/async/Looper.h"        // for Looper, Looper::Timer
#include "aemu/base/async/ThreadLooper.h"  // for ThreadLooper
#include "nimble/nimble_npl.h"                // for ble_npl_eventq_put, ble...
#include "nimble/os_types.h"                  // for ble_npl_callout, ble_np...

using android::base::Looper;
using android::base::ThreadLooper;

void ble_npl_callout_init(struct ble_npl_callout* c,
                          struct ble_npl_eventq* evq,
                          ble_npl_event_fn* ev_cb,
                          void* ev_arg) {
    /* Initialize the callout. */
    memset(c, 0, sizeof(*c));
    c->c_ev.ev_cb = ev_cb;
    c->c_ev.ev_arg = ev_arg;
    c->c_evq = evq;
    c->c_active = false;
    c->c_timer = ThreadLooper::get()->createTimer(
            [](void* ev, Looper::Timer*) {
                ble_npl_callout* c = reinterpret_cast<ble_npl_callout*>(ev);
                if (c->c_evq) {
                    ble_npl_eventq_put(c->c_evq, &c->c_ev);
                } else {
                    c->c_ev.ev_cb(&c->c_ev);
                }
            },
            c);
}

bool ble_npl_callout_is_active(struct ble_npl_callout* c) {
    return c->c_active;
}

int ble_npl_callout_inited(struct ble_npl_callout* c) {
    return (c->c_timer != NULL);
}

ble_npl_error_t ble_npl_callout_reset(struct ble_npl_callout* c,
                                      ble_npl_time_t ticks) {
    if (ticks < 0) {
        return BLE_NPL_EINVAL;
    }

    if (ticks == 0) {
        ticks = 1;
    }

    c->c_ticks = ble_npl_time_get() + ticks;
    c->c_active = true;
    Looper::Timer* timer = reinterpret_cast<Looper::Timer*>(c->c_timer);
    timer->startRelative(ticks);
    return BLE_NPL_OK;
}

int ble_npl_callout_queued(struct ble_npl_callout* c) {
    Looper::Timer* timer = reinterpret_cast<Looper::Timer*>(c->c_timer);
    return timer->isActive();

    // return 0;
}

void ble_npl_callout_stop(struct ble_npl_callout* c) {
    if (!ble_npl_callout_inited(c)) {
        return;
    }
    c->c_ticks = 0;
    Looper::Timer* timer = reinterpret_cast<Looper::Timer*>(c->c_timer);
    timer->stop();
}

ble_npl_time_t ble_npl_callout_get_ticks(struct ble_npl_callout* co) {
    return co->c_ticks;
}

void ble_npl_callout_set_arg(struct ble_npl_callout* co, void* arg) {
    co->c_ev.ev_arg = arg;
}

uint32_t ble_npl_callout_remaining_ticks(struct ble_npl_callout* co,
                                         ble_npl_time_t now) {
    ble_npl_time_t rt;
    auto exp = co->c_ticks;
    if (exp > now) {
        rt = exp - now;
    } else {
        rt = 0;
    }

    return rt;
}

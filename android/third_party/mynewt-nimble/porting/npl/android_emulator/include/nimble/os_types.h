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
//


// This header contains a series of definitions that NimBLE requires.

// This implementation relies on the android-emu-base loopers.
// You must have an active thread looper available.
#ifndef _NPL_OS_TYPES_H
#define _NPL_OS_TYPES_H
#include <stdint.h>



/* The highest and lowest task priorities */
#define OS_TASK_PRI_HIGHEST (sched_get_priority_max(SCHED_RR))
#define OS_TASK_PRI_LOWEST  (sched_get_priority_min(SCHED_RR))

typedef uint32_t ble_npl_time_t;
typedef int32_t ble_npl_stime_t;

//typedef int os_sr_t;
typedef int ble_npl_stack_t;


struct ble_npl_event {
    uint8_t                 ev_queued;
    ble_npl_event_fn       *ev_cb;
    void                   *ev_arg;
};

struct ble_npl_eventq {
    void               *q;
};

struct ble_npl_callout {
    struct ble_npl_event    c_ev;
    struct ble_npl_eventq  *c_evq;
    uint32_t                c_ticks;
    void*                   c_timer;  // android::base::Looper::Timer*
    bool                    c_active;
};

struct ble_npl_mutex {
    void*             lock;
    int32_t           attr; // Spec out attributes (Enum)
    uint64_t          wait;
};

struct ble_npl_sem {
    void*                   lock; // std::timed_mutex*
    int                     count;
};

struct ble_npl_task {
    void* /* pthread_t */               handle; // std::thread
    int64_t /* pthread_attr_t */         attr;
    void* /* struct sched_param */      param;
    const char*             name;
};

typedef void *(*ble_npl_task_func_t)(void *);

int ble_npl_task_init(struct ble_npl_task *t, const char *name, ble_npl_task_func_t func,
		 void *arg, uint8_t prio, ble_npl_time_t sanity_itvl,
		 ble_npl_stack_t *stack_bottom, uint16_t stack_size);

int ble_npl_task_remove(struct ble_npl_task *t);

uint8_t ble_npl_task_count(void);

void ble_npl_task_yield(void);


// Create OS --> Internal mappings
#define os_eventq_dflt_get ble_npl_eventq_dflt_get
#define os_eventq_run ble_npl_eventq_run
#define os_callout ble_npl_callout
#define os_callout_init ble_npl_callout_init
#define os_callout_reset  ble_npl_callout_reset
#define os_callout_stop ble_npl_callout_stop
#define os_event ble_npl_event

// We simulate a tick to be 1 ms
#define OS_TICKS_PER_SEC 1000
#endif // _NPL_OS_TYPES_H

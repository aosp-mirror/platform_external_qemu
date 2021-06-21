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
#include <stdbool.h>


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


struct ble_npl_mutex {
    void*             lock;
    int32_t           attr; // Spec out attributes (Enum)
    uint64_t          wait;
    void*              mu_owner; // Thread id.
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
#define os_stack_t ble_npl_stack_t
#define os_task ble_npl_task
#define os_task_init ble_npl_task_init

#define os_started ble_npl_os_started
#define os_task_func_t ble_npl_task_func_t

#define os_sem ble_npl_sem
#define os_sem_init ble_npl_sem_init
#define os_sem_release ble_npl_sem_release
#define os_sem_pend ble_npl_sem_pend
// We simulate a tick to be 1 ms

#define os_time_ms_to_ticks ble_npl_time_ms_to_ticks
#define os_time_ms_to_ticks32 ble_npl_time_ms_to_ticks32
#define os_time_t ble_npl_time_t
#define os_time_delay ble_npl_time_delay

#define os_start()  printf("Starting os\n");
#define os_time_advance  ble_npl_time_delay

#define OS_TICKS_PER_SEC 1000
#define os_get_current_task_id ble_npl_get_current_task_id


// Timeouts we don't even really use
#define OS_TIMEOUT_NEVER 2^31
#define OS_WAIT_FOREVER 2^31
#define OS_STACK_ALIGN(x)              __ALIGN_MASK(x,(typeof(x))(2)-1)
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define os_malloc malloc

#define MYNEWT 1
#endif // _NPL_OS_TYPES_H

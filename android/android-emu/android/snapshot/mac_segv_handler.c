// Copyright 2017 The Android Open Source Project
// Copyright (c) 2013-2016 Ravenbrook Limited.  See end of file for license.
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// Based on code from this post:
// https://lists.apple.com/archives/Darwin-dev//2006/Oct/msg00122.html
// And:
// info.ravenbrook.com/project/mps/master/code/protxc.c#17
// Copyright (c) 2013-2016 Ravenbrook Limited.

#include "mac_segv_handler.h"

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/thread_status.h>
#include <mach/i386/thread_status.h>
#include <mach/mach_error.h>
#include <mach/exc.h>

#include <sys/types.h>
#include <sys/mman.h>

#define DEBUG 0

#if DEBUG
#define D(fmt,...) fprintf(stderr, "%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define D(fmt,...)
#endif

typedef void (*mac_bad_access_callback_t)(void* faultvaddr);
static void default_bad_access_callback(void* faultvaddr) { D("faultvaddr: %p", faultvaddr); }
static mac_bad_access_callback_t curr_bad_access_callback = default_bad_access_callback;

// obtained from:
// mig $OSX_SDK_DIR/usr/include/mach/mach_exc.defs
// Purpose:
// Define all Mach structs for current OS X version (10.12)
#ifdef  __MigPackStructs
#pragma pack(4)
#endif
typedef struct {
    mach_msg_header_t Head;
    /* start of the kernel processed data */
    mach_msg_body_t msgh_body;
    mach_msg_port_descriptor_t thread;
    mach_msg_port_descriptor_t task;
    /* end of the kernel processed data */
    NDR_record_t NDR;
    exception_type_t exception;
    mach_msg_type_number_t codeCnt;
    int64_t code[2];
    int flavor;
    mach_msg_type_number_t old_stateCnt;
    natural_t old_state[224];
} __Request__mach_exception_raise_state_identity_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
    typedef struct {
        mach_msg_header_t Head;
        NDR_record_t NDR;
        kern_return_t RetCode;
        int flavor;
        mach_msg_type_number_t new_stateCnt;
        natural_t new_state[224];
    } __Reply__mach_exception_raise_state_identity_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

typedef __Request__mach_exception_raise_state_identity_t exception_request_t;
typedef __Reply__exception_raise_state_identity_t exception_reply_t;
static mach_port_name_t my_exception_port = MACH_PORT_NULL;
static mach_port_t my_task_port = MACH_PORT_NULL;
    
exception_request_t my_request;
exception_reply_t reply;

static void suspend_all_except(thread_port_t thread) {
  mach_port_t task;
  thread_act_port_array_t thread_list;
  mach_msg_type_number_t thread_count;

  kern_return_t ret =
      task_threads(my_task_port, &thread_list, &thread_count);

  for (mach_msg_type_number_t i = 0;
       i < thread_count; i++) {
      if (thread_list[i] == thread)
          continue;
      thread_suspend(thread_list[i]);
  }
}

static void resume_all_except(thread_port_t thread) {
  mach_port_t task;
  thread_act_port_array_t thread_list;
  mach_msg_type_number_t thread_count;

  kern_return_t ret =
      task_threads(my_task_port, &thread_list, &thread_count);

  for (mach_msg_type_number_t i = 0;
       i < thread_count; i++) {
      if (thread_list[i] == thread)
          continue;
      thread_resume(thread_list[i]);
  }
}

void
my_catch_exception_raise_state_identity(void) {
    mach_msg_return_t mr;

    mr = mach_msg(&my_request.Head,
                  MACH_RCV_MSG, 0,

                  sizeof(my_request),
                  my_exception_port,

                  MACH_MSG_TIMEOUT_NONE,

                  MACH_PORT_NULL);

    // suspend_all_except(mach_thread_self());

    if (mr == MACH_MSG_SUCCESS) {
        D("success got request");
    } else {
        D("error in getting request 0x%x", mr);
    }

    if (my_request.exception == EXC_BAD_ACCESS) {
        D("yup is bad access");
    } else {
        fprintf(stderr, "%s: not a bad access!\n", __func__);
    }

    if (my_request.codeCnt != 2) {
        D("our mach msg is missing its address :(");
    }

    mach_msg_size_t state_size;
    reply.Head.msgh_bits =
        MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(my_request.Head.msgh_bits), 0);
    reply.Head.msgh_remote_port = my_request.Head.msgh_remote_port;
    reply.Head.msgh_local_port = MACH_PORT_NULL;
    reply.Head.msgh_reserved = 0;
    reply.Head.msgh_id = my_request.Head.msgh_id + 100;
    reply.NDR = my_request.NDR;
    reply.RetCode = KERN_SUCCESS;
    reply.flavor = my_request.flavor;
    reply.new_stateCnt = my_request.old_stateCnt;
    state_size = reply.new_stateCnt * sizeof(natural_t);
    memcpy(&reply.new_state, &my_request.old_state, state_size);
    /* If you use sizeof(reply) for reply->Head.msgh_size then the state
       gets ignored. */
    reply.Head.msgh_size = offsetof(exception_reply_t, new_state) + state_size;

    void* faultvaddr = (void*)(uintptr_t)my_request.code[1];
    void* faultpage = (void*)(((uintptr_t)my_request.code[1]) & ~0xFFF);
    D("bad access at vaddr %p (page %p)", faultvaddr, faultpage);

    mprotect(faultpage, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
    curr_bad_access_callback(faultpage);

    // resume_all_except(mach_thread_self());
    kern_return_t kr;
    kr = mach_msg(&reply.Head,
            MACH_SEND_MSG,
            reply.Head.msgh_size,
            /* recv_size */ 0,
            MACH_PORT_NULL,
            MACH_MSG_TIMEOUT_NONE,
            MACH_PORT_NULL);

    if (kr != KERN_SUCCESS) {
        D("o shit, failed to send mach msg.");
    }
}

static void* exception_handling_thread(void * unused)
{
    (void)unused;
    for (;;) {
        my_catch_exception_raise_state_identity();
    }
}

static void setupHandlerThread(void) {
    D("call");
    kern_return_t kr;
    int pr;
    pthread_t excThread;
    mach_port_t myThreadPort;

    /* Create a port to send and receive exceptions. */
    myThreadPort = mach_task_self();
    my_task_port = myThreadPort;
    assert(MACH_PORT_VALID(myThreadPort));
    kr = mach_port_allocate(myThreadPort,
            MACH_PORT_RIGHT_RECEIVE,
            &my_exception_port);
    assert(kr == KERN_SUCCESS);
    if (kr != KERN_SUCCESS)
        mach_error("ERROR: MPS mach_port_allocate", kr); /* .trans.must */
    assert(MACH_PORT_VALID(my_exception_port));

    /* Allow me to send exceptions on this port. */
    /* TODO: Find out why this is necessary. */
    myThreadPort = mach_task_self();
    assert(MACH_PORT_VALID(myThreadPort));
    kr = mach_port_insert_right(
            myThreadPort,
            my_exception_port, my_exception_port,
            MACH_MSG_TYPE_MAKE_SEND);
    assert(kr == KERN_SUCCESS);
    if (kr != KERN_SUCCESS)
        mach_error("ERROR: MPS mach_port_insert_right", kr); /* .trans.must */

    {
        kern_return_t kr;
        mach_msg_type_number_t old_exception_count = 1;
        exception_mask_t old_exception_masks;
        exception_behavior_t behavior;
        mach_port_t old_exception_ports;
        exception_behavior_t old_behaviors;
        thread_state_flavor_t old_flavors;
        mach_port_t myThreadPort;
        static mach_port_t setupThread = MACH_PORT_NULL;

        myThreadPort = mach_thread_self();

        /* Ask to receive EXC_BAD_ACCESS exceptions on our port, complete
           with thread state and identity information in the message.
           The MACH_EXCEPTION_CODES flag causes the code fields to be
           passed 64-bits wide, matching our struct definition obtained from mig. */
        behavior = (exception_behavior_t)(EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES);
        assert(MACH_PORT_VALID(my_exception_port));
        kr = task_set_exception_ports(mach_task_self(), EXC_MASK_BAD_ACCESS, my_exception_port, behavior, MACHINE_THREAD_STATE);
        // kr = thread_swap_exception_ports(myThreadPort,
        //         EXC_MASK_BAD_ACCESS,
        //         my_exception_port,
        //         behavior,
        //         MACHINE_THREAD_STATE,
        //         &old_exception_masks,
        //         &old_exception_count,
        //         &old_exception_ports,
        //         &old_behaviors,
        //         &old_flavors);
        if (kr != KERN_SUCCESS) {
            mach_error("ERROR: MPS thread_swap_exception_ports", kr); /* .trans.must */
        } else {
            D("exception port successfuly setup");
        }
    } 

    /* Launch the exception handling thread.  We use pthread_create because
       it's much simpler than setting up a thread from scratch using Mach,
       and that's basically what it does.  See [Libc]
       <http://www.opensource.apple.com/source/Libc/Libc-825.26/pthreads/pthread.c> */
    pr = pthread_create(&excThread, NULL, exception_handling_thread, NULL);
    if (pr != 0)
        fprintf(stderr, "ERROR: MPS pthread_create: %d\n", pr); /* .trans.must */
}

static void install_bad_access_callback(mac_bad_access_callback_t f) {
    if (f) curr_bad_access_callback = f;
}

void setup_mac_segv_handler(mac_bad_access_callback_t bad_access_callback) {
    fprintf(stderr, "%s: call\n", __func__);
    install_bad_access_callback(bad_access_callback);

    int pr;
    static pthread_once_t prot_setup_once = PTHREAD_ONCE_INIT;

    pr = pthread_once(&prot_setup_once, setupHandlerThread);
    if (pr != 0)
        fprintf(stderr, "ERROR: MPS pthread_once: %d\n", pr); /* .trans.must */
}

void teardown_mac_segv_handler() {
    fprintf(stderr, "%s: call\n", __func__);
    exception_behavior_t behavior =
        (exception_behavior_t)(EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES);
    task_set_exception_ports(mach_task_self(), EXC_MASK_BAD_ACCESS, MACH_PORT_NULL, behavior, MACHINE_THREAD_STATE);
}

// Ravenbrook Software-derived work. Ravenbrook Software license text:
/* C. COPYRIGHT AND LICENSE
 *
 * Copyright (C) 2013-2016 Ravenbrook Limited <http://www.ravenbrook.com/>.
 * All rights reserved.  This is an open source license.  Contact
 * Ravenbrook for commercial licensing options.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * 3. Redistributions in any form must be accompanied by information on how
 * to obtain complete source code for this software and any accompanying
 * software that uses this software.  The source code must either be
 * included in the distribution or be available for no more than the cost
 * of distribution plus a nominal fee, and must be freely redistributable
 * under reasonable conditions.  For an executable file, complete source
 * code means the source code for all modules it contains. It does not
 * include source code for modules or files that typically accompany the
 * major components of the operating system on which the executable file
 * runs.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, OR NON-INFRINGEMENT, ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

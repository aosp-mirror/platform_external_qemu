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
#include "mach_exception_defs.h"

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
#include <sys/types.h>
#include <sys/mman.h>

#define DEBUG 0

#if DEBUG

#define D(fmt,...) fprintf(stderr, "%s:%d " fmt "\n",          \
                           __func__, __LINE__, ##__VA_ARGS__); \

#else
#define D(fmt,...)
#endif

typedef void (*mac_bad_access_callback_t)(void* faultvaddr);
static void default_bad_access_callback(void* faultvaddr) {
    D("faultvaddr: %p", faultvaddr);
}

static const exception_mask_t kExceptionHandlingMask =
    EXC_MASK_BAD_ACCESS;
static mac_bad_access_callback_t curr_bad_access_callback =
    default_bad_access_callback;

static mach_port_name_t my_exception_port = MACH_PORT_NULL;

// Use raise_state_identity request/reply, with a all_requests_union for
// forwarding requests.
typedef __Request__mach_exception_raise_state_identity_t exception_request_t;
typedef __Reply__exception_raise_state_identity_t exception_reply_t;
typedef AnyRequest forwarded_exception_t;

static exception_request_t current_request;
static exception_reply_t current_reply;
static forwarded_exception_t request_to_forward;
static exception_port_settings old_settings;

// Function to detect whether there was an old handler installed.
// Assumes that |old_settings| has been populated with the old handler's
// settings, and that if there wasn't one, |old_settings.count| == 0.
static bool previous_handler_exists() {
    return old_settings.count > 0;
}

/* Build a reply message based on a request. */

static void build_reply(__Reply__exception_raise_state_identity_t *reply,
                        __Request__mach_exception_raise_state_identity_t *request,
                        kern_return_t ret_code) {
    mach_msg_size_t state_size;
    reply->Head.msgh_bits = MACH_MSGH_BITS(MACH_MSGH_BITS_REMOTE(request->Head.msgh_bits), 0);
    reply->Head.msgh_remote_port = request->Head.msgh_remote_port;
    reply->Head.msgh_local_port = MACH_PORT_NULL;
    reply->Head.msgh_reserved = 0;
    reply->Head.msgh_id = request->Head.msgh_id + 100;
    reply->NDR = request->NDR;
    reply->RetCode = ret_code;
    reply->flavor = request->flavor;
    reply->new_stateCnt = request->old_stateCnt;
    state_size = reply->new_stateCnt * sizeof(natural_t);
    assert(sizeof(reply->new_state) >= state_size);
    memcpy(reply->new_state, request->old_state, state_size);
    /* If you use sizeof(reply) for reply->Head.msgh_size then the state
       gets ignored. */
    reply->Head.msgh_size =
        offsetof(__Reply__exception_raise_state_identity_t,
                 new_state) + state_size;
}

static void must_send(mach_msg_header_t *head) {
    kern_return_t kr;
    kr = mach_msg(head,
                  MACH_SEND_MSG,
                  head->msgh_size,
                  /* recv_size */ 0,
                  MACH_PORT_NULL,
                  MACH_MSG_TIMEOUT_NONE,
                  MACH_PORT_NULL);
    if (kr != KERN_SUCCESS) {
            mach_error("mach_msg send", kr);
            exit(1);
    }
}

// TODO: Figure out why forwarding exceptions directly
// doesn't seem to work.
#if 0

// Based on:
/* machtest/main.c -- Mac OS X / Mach exception handling prototype
 *
 * Created by Richard Brooksby on 2013-06-24.
 * Copyright 2013 Ravenbrook Limited.
 *
 * REFERENCES
 * http://www.mikeash.com/pyblog/friday-qa-2013-01-11-mach-exception-handlers.html
 */
static mach_msg_return_t forward_to_old_exception_handler(
    uint32_t index, 
    __Request__mach_exception_raise_state_identity_t* request) {
    fprintf(stderr, "%s: call\n", __func__);

    /* Forward
       the exception message to the previously installed exception port.
       That port might've been expecting a different behavior and
       flavour, unfortunately, so we have to cope.
       Note that we leave the reply port intact, so we don't have to
       handle and foward replies. */

    exception_handler_t old_port =
        old_settings.ports[index];
    exception_behavior_t old_behavior =
        old_settings.behaviors[index];
    thread_state_flavor_t old_thread_state_flavor =
        old_settings.flavors[index];
    thread_state_t old_thread_state;
    mach_msg_type_number_t old_thread_state_count;
    kern_return_t kr;

    memset(&request_to_forward, 0x0, sizeof(request_to_forward));
    switch (old_behavior) {
        case EXCEPTION_DEFAULT:
            fprintf(stderr, "%s: exception default\n", __func__);
        case EXCEPTION_DEFAULT | MACH_EXCEPTION_CODES:
            fprintf(stderr, "%s: exception default (start copy)\n", __func__);
            COPY_REQUEST(&request_to_forward.raise,
                         request, 64);
            fprintf(stderr, "%s: exception default (done copy)\n", __func__);
            break;
        default:
            /* Get the flavour of thread state that the old handler expects. */
            old_thread_state_count = THREAD_STATE_MAX;
            kr = thread_get_state(
                request->thread.name,
                old_thread_state_flavor,
                old_thread_state,
                &old_thread_state_count);
            if (kr != KERN_SUCCESS) {
                mach_error("thread_get_state", kr);
                exit(1);
            }

            switch (old_behavior) {
                case EXCEPTION_STATE:
                    fprintf(stderr, "%s: exception state\n", __func__);
                case EXCEPTION_STATE | MACH_EXCEPTION_CODES:
                    COPY_REQUEST_STATE(
                        &request_to_forward.raise_state,
                        request,
                        64,
                        old_thread_state_flavor,
                        old_thread_state,
                        old_thread_state_count);
                    break;
                case EXCEPTION_STATE_IDENTITY:
                    fprintf(stderr, "%s: exception state identity\n", __func__);
                case EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES:
                    COPY_REQUEST_STATE_IDENTITY(
                        &request_to_forward.raise_state_identity,
                        request,
                        64,
                        old_thread_state_flavor,
                        old_thread_state,
                        old_thread_state_count);
                    break;
                default:
                    fprintf(stderr, "unknown old behaviour %x\n",
                            old_behavior);
                    abort();
            }
            break;
    }

    /* Forward the message to the old port. */
    fprintf(stderr, "%s: send msg. old port %p\n", __func__, old_port);
    request_to_forward.Head.msgh_local_port = old_port;
    // request_to_forward.Head.msgh_local_port = MACH_PORT_NULL;
    mach_msg_return_t mr =
        mach_msg(&request_to_forward.Head,
                 MACH_SEND_MSG,
                 request_to_forward.Head.msgh_size,
                 /* rcv_size */ 0,
                 /* rcv_name */ MACH_PORT_NULL,
                 MACH_MSG_TIMEOUT_NONE,
                 /* notify */ MACH_PORT_NULL);
    fprintf(stderr, "%s: msg sent with return code 0x%x\n", __func__, mr);
    return mr;
}
#endif

void my_catch_exception_raise_state_identity(void) {
    mach_msg_return_t mr;

    mr = mach_msg(&current_request.Head,
                  MACH_RCV_MSG, 0,
                  sizeof(current_request),
                  my_exception_port,
                  MACH_MSG_TIMEOUT_NONE,
                  MACH_PORT_NULL);

    if (mr == MACH_MSG_SUCCESS) {
        D("success got request");
    } else {
        D("error in getting request 0x%x", mr);
    }

    if (current_request.exception == EXC_BAD_ACCESS) {
        D("yup is bad access");
    } else {
        fprintf(stderr, "%s: ERROR: handled exception that is not EXC_BAD_ACCESS!\n", __func__);
    }

    if (current_request.codeCnt != 2) {
        D("our mach msg is missing its address :(");
    }

    void* faultvaddr = (void*)(uintptr_t)current_request.code[1];
    void* faultpage = (void*)(((uintptr_t)current_request.code[1]) & ~0xFFF);
    D("bad access at vaddr %p (page %p)", faultvaddr, faultpage);

    // Only unprotect / callback for registered ranges
    if (is_ptr_registered(faultpage)) {
        mprotect(faultpage, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
        curr_bad_access_callback(faultpage);
        build_reply(&current_reply, &current_request, KERN_SUCCESS);
        must_send(&current_reply.Head);
    } else {
#if 0
        // TODO: Figure out why this doesn't work.
        // Forward this exception to the previous exception handler.
        if (!previous_handler_exists() ||
            forward_to_old_exception_handler(0, &current_request) !=
            MACH_MSG_SUCCESS) {
            fprintf(stderr, "%s: failed forward or no exc handle rinstall\n", __func__);
            // If there wasn't a previous handler, or the exception forwarding
            // failed, build and send the current reply with KERN_FAILURE.
            build_reply(&current_reply, &current_request, KERN_FAILURE);
            must_send(&current_reply.Head);
        }
#else
        // For now:
        // If the page is not one we registered, forward to the previous
        // exception handler by simply saying we handled this one,
        // then tearing down the handler, which installs the previous handler,
        // and causes another exception to trip (since we didn't really
        // handle this one).
        build_reply(&current_reply, &current_request, KERN_SUCCESS);
        must_send(&current_reply.Head);
        teardown_mac_segv_handler();
#endif
    }
}

static void* exception_handling_thread(void * unused) {
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
    mach_port_t port;

    /* Create a port to send and receive exceptions. */
    port = mach_task_self();
    assert(MACH_PORT_VALID(port));
    kr = mach_port_allocate(port,
                            MACH_PORT_RIGHT_RECEIVE,
                            &my_exception_port);
    assert(kr == KERN_SUCCESS);
    if (kr != KERN_SUCCESS)
        mach_error("ERROR: mach_port_allocate", kr);
    assert(MACH_PORT_VALID(my_exception_port));

    /* Allow me to send exceptions on this port. */
    kr = mach_port_insert_right(port,
                                my_exception_port, my_exception_port,
                                MACH_MSG_TYPE_MAKE_SEND);
    assert(kr == KERN_SUCCESS);
    if (kr != KERN_SUCCESS)
        mach_error("ERROR: mach_port_insert_right", kr);

    /* Ask to receive EXC_BAD_ACCESS exceptions on our port, complete
       with thread state and identity information in the message.
       The MACH_EXCEPTION_CODES flag causes the code fields to be
       passed 64-bits wide, matching our struct definition obtained from mig. */
    assert(MACH_PORT_VALID(my_exception_port));

    memset(&old_settings, 0x0, sizeof(exception_port_settings));
    kr = task_get_exception_ports(
            mach_task_self(),
            kExceptionHandlingMask,
            &old_settings.masks,
            &old_settings.count,
            &old_settings.ports,
            &old_settings.behaviors,
            &old_settings.flavors);
    if (kr != KERN_SUCCESS) {
        // Forget any result where we failed to get the previous
        // exception port settings.
        old_settings.count = 0;
    }

    kr = task_set_exception_ports(
            mach_task_self(),    // For the entire process
            kExceptionHandlingMask, // Capture exceptions of type kExceptionHandlingMask
            my_exception_port,   // Use the exception port we created
            // Give us the entire exception state with Mach exception codes
            // Also see:
            // https://stackoverflow.com/questions/2824105/handling-mach-exceptions-in-64bit-os-x-application
            (exception_behavior_t)(EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES),
            // The type of exception state to return
            MACHINE_THREAD_STATE);
    if (kr != KERN_SUCCESS) {
        mach_error("ERROR: in task_set_exception_ports: 0x%x", kr); /* .trans.must */
    } else {
        D("exception port successfuly setup");
    }

    /* Launch the exception handling thread.  We use pthread_create because
       it's much simpler than setting up a thread from scratch using Mach,
       and that's basically what it does.  See [Libc]
       <http://www.opensource.apple.com/source/Libc/Libc-825.26/pthreads/pthread.c> */
    pr = pthread_create(&excThread, NULL, exception_handling_thread, NULL);
    if (pr != 0)
        fprintf(stderr, "ERROR: pthread_create: %d\n", pr); /* .trans.must */
}

static void install_bad_access_callback(mac_bad_access_callback_t f) {
    if (f) curr_bad_access_callback = f;
}

void setup_mac_segv_handler(mac_bad_access_callback_t bad_access_callback) {
    install_bad_access_callback(bad_access_callback);

    int pr;
    static pthread_once_t prot_setup_once = PTHREAD_ONCE_INIT;

    pr = pthread_once(&prot_setup_once, setupHandlerThread);
    if (pr != 0)
        fprintf(stderr, "ERROR: pthread_once: %d\n", pr); /* .trans.must */
}

void teardown_mac_segv_handler() {
    clear_segv_handling_ranges();
    exception_behavior_t behavior =
        (exception_behavior_t)(EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES);
    D("used to have %llu handlers", (uint64_t)old_settings.count);
    for (uint32_t i = 0; i < old_settings.count; i++) {
        D("mask %d 0x%llx", i, old_settings.masks[i]);
        D("ports %d 0x%llx", i, old_settings.ports[i]);
        D("behaviors %d 0x%llx", i, old_settings.behaviors[i]);
        D("flavors %d 0x%llx", i, old_settings.flavors[i]);
        task_set_exception_ports(mach_task_self(),
                                 old_settings.masks[i],
                                 old_settings.ports[i],
                                 old_settings.behaviors[i],
                                 old_settings.flavors[i]);
    }
    D("done restoring old handlers");
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

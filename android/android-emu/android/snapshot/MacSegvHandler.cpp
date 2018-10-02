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

#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/threads/FunctorThread.h"
#include "android/base/threads/Thread.h"
#include "android/crashreport/crash-handler.h"
#include "android/snapshot/MacSegvHandler.h"
#include "android/snapshot/mach_exception_defs.h"

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/mman.h>

#define DEBUG 0

#if DEBUG

#define D(fmt,...) fprintf(stderr, "%s:%d " fmt "\n",          \
                           __func__, __LINE__, ##__VA_ARGS__); \

#else
#define D(fmt,...)
#endif

// Mach-specific settings / typedefs
static const exception_mask_t kExceptionHandlingMask =
    EXC_MASK_BAD_ACCESS;

static void default_bad_access_callback(void* faultvaddr) {
    D("faultvaddr: %p", faultvaddr);
    abort();
}

// Use raise_state_identity request/reply, with a all_requests_union for
// forwarding requests.
typedef __Request__mach_exception_raise_state_identity_t exception_request_t;
typedef __Reply__exception_raise_state_identity_t exception_reply_t;
typedef AnyRequest forwarded_exception_t;

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

static void must_send(mach_msg_header_t *head);

// MachExceptionHandler class to track Mach-specific structures
class MachExceptionHandler {
public:
    // Constructor sets up the handler thread
    MachExceptionHandler() : mHandlerThread([this]() { exceptionCatchWorker(); }) {
        D("call");

        kern_return_t kr;
        mach_port_t port;

        /* Create a port to send and receive exceptions. */
        port = mach_task_self();
        assert(MACH_PORT_VALID(port));
        kr = mach_port_allocate(port,
                MACH_PORT_RIGHT_RECEIVE,
                &exceptionPort);
        assert(kr == KERN_SUCCESS);
        if (kr != KERN_SUCCESS) {
            crashhandler_die_format(
                "%s: Error 0x%llx in mach_port_allocate", __func__,
                (uint64_t)kr);
        }
        assert(MACH_PORT_VALID(exceptionPort));

        /* Allow me to send exceptions on this port. */
        kr = mach_port_insert_right(port,
                exceptionPort, exceptionPort,
                MACH_MSG_TYPE_MAKE_SEND);
        assert(kr == KERN_SUCCESS);
        if (kr != KERN_SUCCESS) {
            crashhandler_die_format(
                "%s: Error 0x%llx in mach_port_insert_right", __func__,
                (uint64_t)kr);
        }

        install();

        /* Launch the exception handling thread.  We use pthread_create because
           it's much simpler than setting up a thread from scratch using Mach,
           and that's basically what it does.  See [Libc]
           <http://www.opensource.apple.com/source/Libc/Libc-825.26/pthreads/pthread.c> */
        mHandlerThread.start();
    }

    bool isInstalled() const {
        return mInstalled;
    }

    void install() {
        kern_return_t kr;

        /* Ask to receive EXC_BAD_ACCESS exceptions on our port, complete
           with thread state and identity information in the message.
           The MACH_EXCEPTION_CODES flag causes the code fields to be
           passed 64-bits wide, matching our struct definition obtained from mig. */
        assert(MACH_PORT_VALID(exceptionPort));

        memset(&oldSettings, 0x0, sizeof(exception_port_settings));
        kr = task_get_exception_ports(
                mach_task_self(),
                kExceptionHandlingMask,
                oldSettings.masks,
                &oldSettings.count,
                oldSettings.ports,
                oldSettings.behaviors,
                oldSettings.flavors);
        if (kr != KERN_SUCCESS) {
            // Forget any result where we failed to get the previous
            // exception port settings.
            oldSettings.count = 0;
        }

        kr = task_set_exception_ports(
                mach_task_self(),    // For the entire process
                kExceptionHandlingMask, // Capture exceptions of type kExceptionHandlingMask
                exceptionPort,   // Use the exception port we created
                // Give us the entire exception state with Mach exception codes
                // Also see:
                // https://stackoverflow.com/questions/2824105/handling-mach-exceptions-in-64bit-os-x-application
                (exception_behavior_t)(EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES),
                // The type of exception state to return
                MACHINE_THREAD_STATE);
        if (kr != KERN_SUCCESS) {
            crashhandler_die_format(
                "%s: Error 0x%llx in task_set_exception_ports", __func__, kr);
        } else {
            D("exception port successfuly setup");
        }
        mInstalled = true;
    }

    // Function to detect whether there was an old handler installed.
    // Assumes that |oldSettings| has been populated with the old handler's
    // settings, and that if there wasn't one, |oldSettings.count| == 0.
    // TODO: Unused because we don't use direct forwarding for now.
    // bool hasPreviousHandler() const {
    //     android::base::AutoLock lock(mLock);
    //     return oldSettings.count > 0;
    // }
    void teardown() {
        android::base::AutoLock lock(mLock);
        mInstalled = false;
        D("used to have %llu handlers", (uint64_t)oldSettings.count);
        for (uint32_t i = 0; i < oldSettings.count; i++) {
            D("mask %d 0x%llx", i, oldSettings.masks[i]);
            D("ports %d 0x%llx", i, oldSettings.ports[i]);
            D("behaviors %d 0x%llx", i, oldSettings.behaviors[i]);
            D("flavors %d 0x%llx", i, oldSettings.flavors[i]);
            task_set_exception_ports(mach_task_self(),
                    oldSettings.masks[i],
                    oldSettings.ports[i],
                    oldSettings.behaviors[i],
                    oldSettings.flavors[i]);
        }
        D("done restoring old handlers");
    }

    void teardown_crash(const char* format, ...) {
        // Teardown (install previous exception handler) before crashing,
        // to be less confusing in crash reports, especially if triggered
        // in the custon exception handling thread.
        teardown();
        va_list args;
        va_start(args, format);
        crashhandler_die_format_v(format, args);
    }

    void exceptionCatchWorker() {
        for (;;) {
            mach_msg_return_t mr;

            exception_request_t& current_request =
                currentRequest;
            exception_reply_t& current_reply =
                currentReply;

            mr = mach_msg(&current_request.Head,
                    MACH_RCV_MSG, 0,
                    sizeof(current_request),
                    exceptionPort,
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
                teardown_crash(
                    "%s: Error handled exception in custom handler "
                    "that is not EXC_BAD_ACCESS! 0x%llx\n", __func__,
                    (uint64_t)current_request.exception);
            }

            if (current_request.codeCnt != 2) {
                D("our mach msg is missing its address :(");
            }

            void* faultvaddr = (void*)(uintptr_t)current_request.code[1];
            void* faultpage = (void*)(((uintptr_t)current_request.code[1]) & ~0xFFF);
            D("bad access at vaddr %p (page %p)", faultvaddr, faultpage);
            (void)faultvaddr;

            // Only unprotect / callback for registered ranges
            if (macHandler->isRegistered(faultpage)) {
                mprotect(faultpage, 4096, PROT_READ | PROT_WRITE | PROT_EXEC);
                accessCallback(faultpage);
                build_reply(&current_reply, &current_request, KERN_SUCCESS);
                must_send(&current_reply.Head);
            } else {
#if 0
                // TODO: Figure out why this doesn't work.
                // Forward this exception to the previous exception handler.
                if (!previous_handler_exists() ||
                        forward_to_old_exception_handler(0, &current_request) !=
                        MACH_MSG_SUCCESS) {
                    fprintf(stderr, "%s: failed forward or no exc handler installed\n", __func__);
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
                teardown();
#endif
            }
        }
    }

    MacSegvHandler::BadAccessCallback accessCallback =
        default_bad_access_callback;
    mach_port_name_t exceptionPort = MACH_PORT_NULL;
    exception_request_t currentRequest;
    exception_reply_t currentReply;
    forwarded_exception_t requestToForward;
    exception_port_settings oldSettings;

    // Back pointer to our class
    MacSegvHandler* macHandler = nullptr;

    // Thread on which the exception handling occurs
    android::base::FunctorThread mHandlerThread;
    // Lock concurrent access
    android::base::Lock mLock;

    // Whether or not the custom exception handler is active.
    bool mInstalled = false;
};

static android::base::LazyInstance<MachExceptionHandler> sMachExcHandler =
    LAZY_INSTANCE_INIT;

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
        sMachExcHandler->teardown_crash(
            "%s: Error 0x%llx in mach msg", __func__, (uint64_t)kr);
    }
}

MacSegvHandler::MacSegvHandler(BadAccessCallback f) {
    // Also constructs and initializes our MachExceptionHandler instance,
    // and starts the exception handling thread.
    sMachExcHandler->macHandler = this;
    sMachExcHandler->accessCallback = f;
    if (!sMachExcHandler->isInstalled()) {
        sMachExcHandler->install();
    }
}

MacSegvHandler::~MacSegvHandler() {
    clearRegistered();
    sMachExcHandler->teardown(); // Also gets rid of this
}

void MacSegvHandler::registerMemoryRange(void* ptr, uint64_t size) {
    mRanges.emplace_back((char*)ptr, size);
}

bool MacSegvHandler::isRegistered(void* ptr) const {
    const auto it =
        std::find_if(mRanges.begin(), mRanges.end(),
                     [ptr](const MemoryRange& r) {
                        char* begin = r.first;
                        uint64_t sz = r.second;
                        return (ptr >= begin) && ptr < (begin + sz);
                     });
    return it != mRanges.end();
}

void MacSegvHandler::clearRegistered() {
    // Some ranges can still be protected. Unprotect everything explicitly here.
    for (const auto& range: mRanges) {
        char* begin = range.first;
        uint64_t sz = range.second;
        mprotect(begin, sz, PROT_READ | PROT_WRITE | PROT_EXEC);
    }
    mRanges.clear();
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
        oldSettings.ports[index];
    uint64_t old_behavior =
        oldSettings.behaviors[index];
    thread_state_flavor_t old_thread_state_flavor =
        oldSettings.flavors[index];
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
    request_to_forward.Head.msgh_remote_port = old_port;
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

// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/snapshot/MemoryWatch.h"

#include "android/snapshot/MacSegvHandler.h"

#include <Hypervisor/hv.h>

#include <stdio.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/mman.h>

namespace android {
namespace snapshot {

class MemoryAccessWatch::Impl {};

bool MemoryAccessWatch::isSupported() {
    fprintf(stderr, "%s: mac lazy loading not supported\n", __func__);
    return true;
}

static MemoryAccessWatch* current = nullptr;

static void access_callback(void* ptr) {
    fprintf(stderr, "%s: call %p\n", __func__, ptr);
    current->doAccessCallback(ptr);
    fprintf(stderr, "%s: call %p (exit)\n", __func__, ptr);
}


void MemoryAccessWatch::doAccessCallback(void* ptr) {
    accessCallback(ptr);
}

MemoryAccessWatch::MemoryAccessWatch(AccessCallback&& _ac,
                                     IdleCallback&& idleCallback)
    : mImpl(new Impl()), accessCallback(_ac) {

    current = this;

    fprintf(stderr, "%s: new impl\n", __func__);
    setup_mac_segv_handler();
    install_bad_access_callback(access_callback);

    // char* data = (char*)malloc(4096 * 2);
    // data = (char*)(4096 * ((((uintptr_t)data) + 4096 - 1) / 4096));
    // fprintf(stderr, "%s: data: %p\n", __func__, data);
    // mprotect(data, 4096, PROT_NONE);
    // data[234] = 0xff;
    // fprintf(stderr, "%s: survived!\n", __func__);
    
}

MemoryAccessWatch::~MemoryAccessWatch() {}

bool MemoryAccessWatch::valid() const {
    return true;
}

bool MemoryAccessWatch::registerMemoryRange(void* start, size_t length, uint64_t gpa, bool found) {
    // if (found && gpa > 0x20000000) {
        // fprintf(stderr, "%s: register [%p %p] (gpa 0x%llx)\n", __func__, start, ((char*)start) + length, gpa);
        hv_return_t hvret = hv_vm_protect(gpa, length, 0);
        if (hvret == HV_SUCCESS) {
            // fprintf(stderr, "%s: successfully protected!\n", __func__);
        } else {
            // fprintf(stderr, "%s: OOPS\n", __func__);
        }
    // }

    // if (gpa >= 0x21400000 && gpa < 0x21800000) {
        int ret = mprotect(start, length, PROT_NONE);
        if (ret < 0) {
            // fprintf(stderr, "%s: could not mprotect %d\n", __func__, errno);
        }
    // }
    return true;
}

void MemoryAccessWatch::doneRegistering() {
    // fprintf(stderr, "%s: signal handler install\n", __func__);
    // struct sigaction sa;
    // struct sigaction old;
    // sa.sa_flags = SA_SIGINFO;
    // sigemptyset(&sa.sa_mask);
    // sa.sa_sigaction = [](int sig, siginfo_t *si, void *unused) { mprotect((void*)((uintptr_t)si->si_addr & ~0xFFF), 4096, PROT_READ | PROT_WRITE | PROT_EXEC);  fprintf(stderr, "%s: segv HANDLER!!!!!!!!!!!!!!!!! %p\n", __func__, si->si_addr); };
    // int act_ret = ::sigaction(SIGSEGV, &sa, &old);
    // act_ret = ::sigaction(SIGTRAP, &sa, &old);
    // act_ret = ::sigaction(SIGBUS, &sa, &old);
    // if (act_ret)  {
    //     fprintf(stderr, "%s: can't set handler! errno %d\n", __func__, errno);
    // }
}

bool MemoryAccessWatch::fillPage(void* ptr, size_t length, const void* data, uint64_t gpa) {
    // fprintf(stderr, "%s: for [%p %p] [0x%llx 0x%llx]\n", __func__, ptr, ((char*)ptr) + length, gpa, gpa + length);
    // if (gpa >= 0x20000000) {
        hv_return_t ret = hv_vm_protect(gpa, length, HV_MEMORY_READ | HV_MEMORY_WRITE | HV_MEMORY_EXEC);
        if (ret == HV_SUCCESS) {
            // fprintf(stderr, "%s: success unprotect\n", __func__);
        }
    // }
    memcpy(ptr, data, length);
//     fprintf(stderr, "%s: success memcpy\n", __func__);
    // if (gpa >= 0x21400000 && gpa <= 0x21800000) {
    //     mprotect(ptr, length, PROT_READ | PROT_WRITE | PROT_EXEC);
    // }
    return true;
}

}  // namespace snapshot
}  // namespace android

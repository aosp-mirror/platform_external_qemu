// Copyright (C) 2016 The Android Open Source Project
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

#include "android/base/synchronization/Lock.h"

#include "android/base/memory/LazyInstance.h"
#include "android/base/system/System.h"

#define DEBUG 0

#if DEBUG
#include <stdio.h>
#define DPRINT(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DPRINT(...) (void)0
#endif

// Based on a similar setup for condition variables in Windows
// (ConditionVariable_win32.cpp), + chromium webrtc.

// We need to check at runtime if there is support for
// SRWLock's (WinXP doesn't have it).

// Function pointers for locking operations are set
// depending on support. If there aren't any, we use a fallback
// implementation.

static bool sSRWSupported = false;
static bool sModuleLoaded = false;

typedef void (WINAPI *initialize_srw_lock_t)(SRWLock*);

typedef void (WINAPI *acquire_srw_lock_exclusive_t)(SRWLock*);
typedef void (WINAPI *release_srw_lock_exclusive_t)(SRWLock*);

typedef void (WINAPI *acquire_srw_lock_shared_t)(SRWLock*);
typedef void (WINAPI *release_srw_lock_shared_t)(SRWLock*);

typedef void (WINAPI *destroy_srw_lock_t)(SRWLock*);

namespace android {
namespace base {

namespace {

// Fallback implementation for Windows versions prior to Vista
// Have two locks, read and write lock.
void WINAPI default_initialize_srw_lock(SRWLock* lock) {
    FallbackLockObj* fallback_lock = (FallbackLockObj*)lock;
    fallback_lock->num_readers = 0;
    InitializeCriticalSection(&fallback_lock->write_lock);
    InitializeCriticalSection(&fallback_lock->read_lock);
}

void WINAPI default_acquire_srw_lock_exclusive(SRWLock* lock) {
    FallbackLockObj* fallback_lock = (FallbackLockObj*)lock;
    EnterCriticalSection(&fallback_lock->write_lock);
}

void WINAPI default_release_srw_lock_exclusive(SRWLock* lock) {
    FallbackLockObj* fallback_lock = (FallbackLockObj*)lock;
    LeaveCriticalSection(&fallback_lock->write_lock);
}

void WINAPI default_acquire_srw_lock_shared(SRWLock* lock) {
    FallbackLockObj* fallback_lock = (FallbackLockObj*)lock;
    EnterCriticalSection(&fallback_lock->read_lock);
    fallback_lock->num_readers++;
    if (fallback_lock->num_readers == 1) {
        EnterCriticalSection(&fallback_lock->write_lock);
    }
    LeaveCriticalSection(&fallback_lock->read_lock);
}

void WINAPI default_release_srw_lock_shared(SRWLock* lock) {
    FallbackLockObj* fallback_lock = (FallbackLockObj*)lock;
    EnterCriticalSection(&fallback_lock->read_lock);
    fallback_lock->num_readers--;
    if (fallback_lock->num_readers == 0) {
        LeaveCriticalSection(&fallback_lock->write_lock);
    }
    LeaveCriticalSection(&fallback_lock->read_lock);
}

void WINAPI default_destroy_srw_lock(SRWLock* lock) {
    FallbackLockObj* fallback_lock = (FallbackLockObj*)lock;
    DeleteCriticalSection(&fallback_lock->write_lock);
    DeleteCriticalSection(&fallback_lock->read_lock);
}

void WINAPI srwsupported_destroy_srw_lock(SRWLock* lock) { }

initialize_srw_lock_t initialize_srw_lock = default_initialize_srw_lock;
acquire_srw_lock_exclusive_t acquire_srw_lock_exclusive = default_acquire_srw_lock_exclusive;
release_srw_lock_exclusive_t release_srw_lock_exclusive = default_release_srw_lock_exclusive;
acquire_srw_lock_shared_t acquire_srw_lock_shared = default_acquire_srw_lock_shared;
release_srw_lock_shared_t release_srw_lock_shared = default_release_srw_lock_shared;
destroy_srw_lock_t destroy_srw_lock = default_destroy_srw_lock;

// Helper class which attempts to load
// SRWLOCK related functions.
class SRWLockSupportLoader {
public:
    SRWLockSupportLoader();
};

bool isSrwLockAWineStub(initialize_srw_lock_t init,
                        acquire_srw_lock_exclusive_t lock,
                        release_srw_lock_exclusive_t unlock) {
    if (!System::get()->isRunningUnderWine()) {
        return false;
    }
    SRWLock lock1, lock2;
    memset(&lock1, 0x12, sizeof(lock1));
    memcpy(&lock2, &lock1, sizeof(lock1));
    init(&lock1);
    if (memcmp(&lock1, &lock2, sizeof(lock1))==0) {
        DPRINT("InitializeSRWLock is a stub\n");
        return true;
    }
    memcpy(&lock2, &lock1, sizeof(lock1));
    lock(&lock1);
    if (memcmp(&lock1, &lock2, sizeof(lock1))==0) {
        DPRINT("AcquireSRWLockExclusive is a stub\n");
        return true;
    }
    unlock(&lock1);
    return false;
}

SRWLockSupportLoader::SRWLockSupportLoader() {
    if (sModuleLoaded) return;

    HMODULE library = LoadLibraryW(L"kernel32.dll");
    if (!library) { sModuleLoaded = true; return; }

    initialize_srw_lock_t initsrwlock =
        (initialize_srw_lock_t)GetProcAddress(library, "InitializeSRWLock");

    acquire_srw_lock_exclusive_t acqlockex =
        (acquire_srw_lock_exclusive_t)GetProcAddress(library, "AcquireSRWLockExclusive");
    release_srw_lock_exclusive_t rellockex =
        (release_srw_lock_exclusive_t)GetProcAddress(library, "ReleaseSRWLockExclusive");

    acquire_srw_lock_shared_t acqlocksh =
        (acquire_srw_lock_shared_t)GetProcAddress(library, "AcquireSRWLockShared");
    release_srw_lock_shared_t rellocksh =
        (release_srw_lock_shared_t)GetProcAddress(library, "ReleaseSRWLockShared");

    if (initsrwlock &&
        acqlockex && rellockex &&
        acqlocksh && rellocksh &&
        !isSrwLockAWineStub(initsrwlock, acqlockex, rellockex)) {
        DPRINT("SRWLOCK supported.\n");
        sSRWSupported = true;

        initialize_srw_lock = initsrwlock;

        acquire_srw_lock_exclusive = acqlockex;
        release_srw_lock_exclusive = rellockex;

        acquire_srw_lock_shared = acqlocksh;
        release_srw_lock_shared = rellocksh;

        destroy_srw_lock = srwsupported_destroy_srw_lock;
    } else {
        DPRINT("SRWLOCK not supported.\n");
        sSRWSupported = false;
    }
    sModuleLoaded = true;
}

LazyInstance<SRWLockSupportLoader> sLockSupportLoader = LAZY_INSTANCE_INIT;

}  // namespace

ReadWriteLock::ReadWriteLock() {
    if (!sModuleLoaded) { sLockSupportLoader.ptr(); }
    initialize_srw_lock((SRWLock*)&mLock);
}

ReadWriteLock::~ReadWriteLock() {
    destroy_srw_lock((SRWLock*)&mLock);
}

void ReadWriteLock::lockWrite() { acquire_srw_lock_exclusive((SRWLock*)(&mLock)); }
void ReadWriteLock::unlockWrite() { release_srw_lock_exclusive((SRWLock*)(&mLock)); }

void ReadWriteLock::lockRead() { acquire_srw_lock_shared((SRWLock*)(&mLock)); }
void ReadWriteLock::unlockRead() { release_srw_lock_shared((SRWLock*)(&mLock)); }

}  // namespace base
}  // namespace android


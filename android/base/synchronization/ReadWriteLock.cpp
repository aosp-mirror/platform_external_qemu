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
#include "android/base/synchronization/ThreadLockMap.h"

namespace android {
namespace base {

ReadWriteLock::ReadWriteLock() {
    pthread_rwlock_init(&mRWLock, NULL);
    addLockMapEntry((void*)&mRWLock);
    unsetRecursiveLock();
}

ReadWriteLock::~ReadWriteLock() {
    removeLockMapEntry((void*)&mRWLock);
}

void ReadWriteLock::lockRead() {
    if (!threadHasLock()) {
        ::pthread_rwlock_rdlock(&mRWLock);
        setRecursiveLock();
    }
}

void ReadWriteLock::unlockRead() {
    if (threadHasLock()) {
        ::pthread_rwlock_unlock(&mRWLock);
        unsetRecursiveLock();
    }
}

void ReadWriteLock::lockWrite() {
    if (!threadHasLock()) {
        ::pthread_rwlock_wrlock(&mRWLock);
        setRecursiveLock();
    }
}
void ReadWriteLock::unlockWrite() {
    if (threadHasLock() > 0) {
        ::pthread_rwlock_unlock(&mRWLock);
        unsetRecursiveLock();
    }
}

void ReadWriteLock::setRecursiveLock() { getLockMap()[(void*)&mRWLock] = 1; }
void ReadWriteLock::unsetRecursiveLock() { getLockMap()[(void*)&mRWLock] = 0; }
bool ReadWriteLock::threadHasLock() { return getLockMap()[(void*)&mRWLock]; }

} // namespace base
} // namespace android

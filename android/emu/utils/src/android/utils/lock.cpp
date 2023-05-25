// Copyright 2016 The Android Open Source Project
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

#include "android/utils/lock.h"

#include "aemu/base/synchronization/Lock.h"

using android::base::Lock;
using android::base::ReadWriteLock;

static auto asBaseLock(CLock* l) -> Lock* {
    return reinterpret_cast<Lock*>(l);
}

auto android_lock_new() -> CLock* {
    return reinterpret_cast<CLock*>(new Lock());
}

void android_lock_acquire(CLock* l) {
    if (l != nullptr) {
        asBaseLock(l)->lock();
    }
}

void android_lock_release(CLock* l) {
    if (l != nullptr) {
        asBaseLock(l)->unlock();
    }
}

void android_lock_free(CLock* l) {
    delete asBaseLock(l);
}

static auto asReadWriteLock(CReadWriteLock* l) -> ReadWriteLock* {
    return reinterpret_cast<ReadWriteLock*>(l);
}

auto android_rw_lock_new() -> CReadWriteLock* {
    return reinterpret_cast<CReadWriteLock*>(new ReadWriteLock());
}

void android_rw_lock_read_acquire(CReadWriteLock* l) {
    if (l != nullptr) {
        asReadWriteLock(l)->lockRead();
    }
}

void android_rw_lock_read_release(CReadWriteLock* l) {
    if (l != nullptr) {
        asReadWriteLock(l)->unlockRead();
    }
}

void android_rw_lock_write_acquire(CReadWriteLock* l) {
    if (l != nullptr) {
        asReadWriteLock(l)->lockWrite();
    }
}

void android_rw_lock_write_release(CReadWriteLock* l) {
    if (l != nullptr) {
        asReadWriteLock(l)->unlockWrite();
    }
}

void android_rw_lock_free(CReadWriteLock* l) {
    delete asReadWriteLock(l);
}

// Copyright (C) 2014 The Android Open Source Project
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

#include "android/base/AlignedBuf.h"
#include "android/base/Debug.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <assert.h>

namespace android {
namespace base {

class StaticLock::Impl {
public:
    constexpr Impl() = default;

    ~Impl() {
#ifndef _WIN32
        // The only difference is that POSIX requires a deallocation function call
        // for its mutexes.
        ::pthread_mutex_destroy(&mLock);
#endif
    }

    // Acquire the lock.
    void lock() {
#ifdef _WIN32
        ::AcquireSRWLockExclusive(&mLock);
#else
        ::pthread_mutex_lock(&mLock);
#endif
        ANDROID_IF_DEBUG(mIsLocked = true;)
    }

    bool tryLock() {
        bool ret = false;
#ifdef _WIN32
        ret = ::TryAcquireSRWLockExclusive(&mLock);
#else
        ret = ::pthread_mutex_trylock(&mLock) == 0;
#endif
        ANDROID_IF_DEBUG(mIsLocked = ret;)
        return ret;
    }

    ANDROID_IF_DEBUG(bool isLocked() const { return mIsLocked; })

    // Release the lock.
    void unlock() {
        ANDROID_IF_DEBUG(mIsLocked = false;)
#ifdef _WIN32
        ::ReleaseSRWLockExclusive(&mLock);
#else
        ::pthread_mutex_unlock(&mLock);
#endif
    }

#ifdef _WIN32
    // Benchmarks show that on Windows SRWLOCK performs a little bit better than
    // CRITICAL_SECTION for uncontended mode and much better in case of
    // contention.
    SRWLOCK mLock = SRWLOCK_INIT;
#else
    pthread_mutex_t mLock = PTHREAD_MUTEX_INITIALIZER;
#endif
    ANDROID_IF_DEBUG(bool mIsLocked = false;)
};

class ReadWriteLock::Impl {
public:
    Impl() = default;
    ~Impl() {
#ifndef _WIN32
        // The only difference is that POSIX requires a deallocation function call
        // for its mutexes.
        ::pthread_rwlock_destroy(&mLock);
#endif
    }
#ifdef _WIN32
    void lockRead() { ::AcquireSRWLockShared(&mLock); }
    void unlockRead() { ::ReleaseSRWLockShared(&mLock); }
    void lockWrite() { ::AcquireSRWLockExclusive(&mLock); }
    void unlockWrite() { ::ReleaseSRWLockExclusive(&mLock); }
#else   // !_WIN32
    void lockRead() { ::pthread_rwlock_rdlock(&mLock); }
    void unlockRead() { ::pthread_rwlock_unlock(&mLock); }
    void lockWrite() { ::pthread_rwlock_wrlock(&mLock); }
    void unlockWrite() { ::pthread_rwlock_unlock(&mLock); }
#endif

#ifdef _WIN32
    // Benchmarks show that on Windows SRWLOCK performs a little bit better than
    // CRITICAL_SECTION for uncontended mode and much better in case of
    // contention.
    SRWLOCK mLock = SRWLOCK_INIT;
#else
    pthread_rwlock_t mLock = PTHREAD_RWLOCK_INITIALIZER;
#endif
    ANDROID_IF_DEBUG(bool mIsLocked = false;)
};

StaticLock::StaticLock() {
    // Allocate all locks on separate cache lines
    // to prevent cache flushes when two different locks
    // operate on two different physical threads.
    void* storage =
        aligned_buf_alloc(64, sizeof(StaticLock::Impl));
    mImpl = new(storage) StaticLock::Impl;
}

void StaticLock::lock() { mImpl->lock(); }
bool StaticLock::tryLock() { return mImpl->tryLock(); }
void StaticLock::unlock() { mImpl->unlock(); }

void* StaticLock::getPlatformLock() {
    return (void*)&mImpl->mLock;
}

Lock::Lock() : StaticLock() {}
Lock::~Lock() {
    mImpl->StaticLock::Impl::~Impl();
    aligned_buf_free(mImpl);
}

AutoLock::AutoLock(StaticLock& lock) :
    mLock(lock) { mLock.lock(); }

AutoLock::AutoLock(AutoLock&& other) :
    mLock(other.mLock), mLocked(other.mLocked) {
    other.mLocked = false;
}

void AutoLock::lock() {
    assert(!mLocked);
    mLock.lock();
    mLocked = true;
}

void AutoLock::unlock() {
    assert(mLocked);
    mLock.unlock();
    mLocked = false;
}

bool AutoLock::isLocked() const { return mLocked; }

AutoLock::~AutoLock() {
    if (mLocked) {
        mLock.unlock();
    }
}

ReadWriteLock::ReadWriteLock() {
    void* storage =
        aligned_buf_alloc(64, sizeof(ReadWriteLock::Impl));
    mImpl = new(storage) ReadWriteLock::Impl;
}

ReadWriteLock::~ReadWriteLock() {
    mImpl->ReadWriteLock::Impl::~Impl();
    aligned_buf_free(mImpl);
}

void ReadWriteLock::lockRead() { mImpl->lockRead(); }
void ReadWriteLock::unlockRead() { mImpl->unlockRead(); }
void ReadWriteLock::lockWrite() { mImpl->lockWrite(); }
void ReadWriteLock::unlockWrite() { mImpl->unlockWrite(); }

void* ReadWriteLock::getPlatformLock() {
    return (void*)&mImpl->mLock;
}

AutoWriteLock::AutoWriteLock(ReadWriteLock& lock) :
    mLock(lock) { mLock.lockWrite(); }

void AutoWriteLock::lockWrite() {
    assert(!mWriteLocked);
    mLock.lockWrite();
    mWriteLocked = true;
}

void AutoWriteLock::unlockWrite() {
    assert(mWriteLocked);
    mLock.unlockWrite();
    mWriteLocked = false;
}

AutoWriteLock::~AutoWriteLock() {
    if (mWriteLocked) {
        mLock.unlockWrite();
    }
}

AutoReadLock::AutoReadLock(ReadWriteLock& lock) :
    mLock(lock) { mLock.lockRead(); }

void AutoReadLock::lockRead() {
    assert(!mReadLocked);
    mLock.lockRead();
    mReadLocked = true;
}

void AutoReadLock::unlockRead() {
    assert(mReadLocked);
    mLock.unlockRead();
    mReadLocked = false;
}

AutoReadLock::~AutoReadLock() {
    if (mReadLocked) {
        mLock.unlockRead();
    }
}

}  // namespace base
}  // namespace android

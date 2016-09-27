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

#pragma once

#include "android/base/Compiler.h"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN 1
#  include <windows.h>
#else
#  include <pthread.h>
#endif

#include <assert.h>

#ifdef _WIN32
// Declarations for shared reader/writer lock objects.
// For the ReadWriteLock class.
struct SRWLock {
// Note: this is a copy of the definition of the SRWLOCK struct
// from Windows.h.
    void* ptr;
};

struct FallbackLockObj {
    CRITICAL_SECTION write_lock;
    CRITICAL_SECTION read_lock;
    size_t num_readers;
};

union SRWLockObj {
    SRWLock srw_lock;
    FallbackLockObj fallback_lock_obj;
};
#endif

namespace android {
namespace base {

class AutoLock;
class AutoWriteLock;
class AutoReadLock;

// Simple wrapper class for mutexes.
class Lock {
public:
    using AutoLock = android::base::AutoLock;

    // Constructor.
    Lock() {
#ifdef _WIN32
        ::InitializeCriticalSection(&mLock);
#else
        ::pthread_mutex_init(&mLock, NULL);
#endif
    }

    // Destructor.
    ~Lock() {
#ifdef _WIN32
        ::DeleteCriticalSection(&mLock);
#else
        ::pthread_mutex_destroy(&mLock);
#endif
    }

    // Acquire the lock.
    void lock() {
#ifdef _WIN32
      ::EnterCriticalSection(&mLock);
#else
      ::pthread_mutex_lock(&mLock);
#endif
    }

    // Release the lock.
    void unlock() {
#ifdef _WIN32
       ::LeaveCriticalSection(&mLock);
#else
       ::pthread_mutex_unlock(&mLock);
#endif
    }

private:
    friend class ConditionVariable;

#ifdef _WIN32
public: // Currently ConditionVariable has a hidden implementation class, and
        // that's the one requiring access to mLock - so let it be public.
    CRITICAL_SECTION mLock;
private:
#else
    pthread_mutex_t mLock;
#endif
    // POSIX threads don't allow move (undefined behavior)
    // so we also disallow this on Windows to be consistent.
    DISALLOW_COPY_ASSIGN_AND_MOVE(Lock);
};


#ifdef _WIN32
class ReadWriteLock {
public:
    ReadWriteLock();
    ~ReadWriteLock();
    void lockRead();
    void lockWrite();
    void unlockRead();
    void unlockWrite();
private:
    friend class ConditionVariable;
    SRWLockObj mLock;
    // POSIX threads don't allow move,
    // so we don't allow move on Windows as wll,
    // to be consistent.
    DISALLOW_COPY_ASSIGN_AND_MOVE(ReadWriteLock);
};
#else
class ReadWriteLock {
public:
    using AutoWriteLock = android::base::AutoWriteLock;
    using AutoReadLock = android::base::AutoReadLock;
    ReadWriteLock() {
        ::pthread_rwlock_init(&mRWLock, NULL);
    }
    void lockRead() {
        ::pthread_rwlock_rdlock(&mRWLock);
    }
    void unlockRead() {
        ::pthread_rwlock_unlock(&mRWLock);
    }
    void lockWrite() {
        ::pthread_rwlock_wrlock(&mRWLock);
    }
    void unlockWrite() {
        ::pthread_rwlock_unlock(&mRWLock);
    }
private:
    friend class ConditionVariable;
    pthread_rwlock_t mRWLock;
    // POSIX threads don't allow move
    // (Undefined behavior for what happens when
    // mutexes are copied)
    DISALLOW_COPY_ASSIGN_AND_MOVE(ReadWriteLock);
};
#endif

// Helper class to lock / unlock a mutex automatically on scope
// entry and exit.
// NB: not thread-safe (as opposed to the Lock class)
class AutoLock {
public:
    AutoLock(Lock& lock) : mLock(lock) {
        mLock.lock();
    }

    void lock() {
        assert(!mLocked);
        mLock.lock();
        mLocked = true;
    }

    void unlock() {
        assert(mLocked);
        mLock.unlock();
        mLocked = false;
    }

    ~AutoLock() {
        if (mLocked) {
            mLock.unlock();
        }
    }

private:
    Lock& mLock;
    bool mLocked = true;
    // Don't allow move because this class
    // has a non-movable object.
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoLock);
};

class AutoWriteLock {
public:
    AutoWriteLock(ReadWriteLock& lock) : mRWLock(lock) {
        mRWLock.lockWrite();
    }

    void lockWrite() {
        assert(!mWriteLocked);
        mRWLock.lockWrite();
        mWriteLocked = true;
    }

    void unlockWrite() {
        assert(mWriteLocked);
        mRWLock.unlockWrite();
        mWriteLocked = false;
    }

    ~AutoWriteLock() {
        if (mWriteLocked) {
            mRWLock.unlockWrite();
        }
    }

private:
    ReadWriteLock& mRWLock;
    bool mWriteLocked = true;
    // This class has a non-movable object.
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoWriteLock);
};

class AutoReadLock {
public:
    AutoReadLock(ReadWriteLock& lock) : mRWLock(lock) {
        mRWLock.lockRead();
    }

    void lockRead() {
        assert(!mReadLocked);
        mRWLock.lockRead();
        mReadLocked = true;
    }

    void unlockRead() {
        assert(mReadLocked);
        mRWLock.unlockRead();
        mReadLocked = false;
    }

    ~AutoReadLock() {
        if (mReadLocked) {
            mRWLock.unlockRead();
        }
    }

private:
    ReadWriteLock& mRWLock;
    bool mReadLocked = true;
    // This class has a non-movable object.
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoReadLock);
};

}  // namespace base
}  // namespace android

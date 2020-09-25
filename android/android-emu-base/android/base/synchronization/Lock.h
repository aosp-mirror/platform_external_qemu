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
#include "android/base/Debug.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#else
#include <pthread.h>
#endif

#include <atomic>

#include <assert.h>

namespace android {
namespace base {

class AutoLock;
class AutoWriteLock;
class AutoReadLock;

// A wrapper class for mutexes only suitable for using in static context,
// where it's OK to leak the underlying system object. Use Lock for scoped or
// member locks.
class StaticLock {
public:
    using AutoLock = android::base::AutoLock;

    constexpr StaticLock() = default;

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

protected:
    friend class ConditionVariable;

#ifdef _WIN32
    // Benchmarks show that on Windows SRWLOCK performs a little bit better than
    // CRITICAL_SECTION for uncontended mode and much better in case of
    // contention.
    SRWLOCK mLock = SRWLOCK_INIT;
#else
    pthread_mutex_t mLock = PTHREAD_MUTEX_INITIALIZER;
#endif
    // Both POSIX threads and WinAPI don't allow move (undefined behavior).
    DISALLOW_COPY_ASSIGN_AND_MOVE(StaticLock);

    ANDROID_IF_DEBUG(bool mIsLocked = false;)
};

// Simple wrapper class for mutexes used in non-static context.
class Lock : public StaticLock {
public:
    using StaticLock::AutoLock;

    constexpr Lock() = default;
#ifndef _WIN32
    // The only difference is that POSIX requires a deallocation function call
    // for its mutexes.
    ~Lock() { ::pthread_mutex_destroy(&mLock); }
#endif
};

class ReadWriteLock {
public:
    using AutoWriteLock = android::base::AutoWriteLock;
    using AutoReadLock = android::base::AutoReadLock;

#ifdef _WIN32
    constexpr ReadWriteLock() = default;
    ~ReadWriteLock() = default;
    void lockRead() { ::AcquireSRWLockShared(&mLock); }
    void unlockRead() { ::ReleaseSRWLockShared(&mLock); }
    void lockWrite() { ::AcquireSRWLockExclusive(&mLock); }
    void unlockWrite() { ::ReleaseSRWLockExclusive(&mLock); }

private:
    SRWLOCK mLock = SRWLOCK_INIT;
#else   // !_WIN32
    ReadWriteLock() { ::pthread_rwlock_init(&mLock, NULL); }
    ~ReadWriteLock() { ::pthread_rwlock_destroy(&mLock); }
    void lockRead() { ::pthread_rwlock_rdlock(&mLock); }
    void unlockRead() { ::pthread_rwlock_unlock(&mLock); }
    void lockWrite() { ::pthread_rwlock_wrlock(&mLock); }
    void unlockWrite() { ::pthread_rwlock_unlock(&mLock); }

private:
    pthread_rwlock_t mLock;
#endif  // !_WIN32

    friend class ConditionVariable;
    DISALLOW_COPY_ASSIGN_AND_MOVE(ReadWriteLock);
};

// Helper class to lock / unlock a mutex automatically on scope
// entry and exit.
// NB: not thread-safe (as opposed to the Lock class)
class AutoLock {
public:
    AutoLock(StaticLock& lock) : mLock(lock) { mLock.lock(); }

    AutoLock(AutoLock&& other) : mLock(other.mLock), mLocked(other.mLocked) {
        other.mLocked = false;
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

    bool isLocked() const { return mLocked; }

    ~AutoLock() {
        if (mLocked) {
            mLock.unlock();
        }
    }

private:
    StaticLock& mLock;
    bool mLocked = true;

    friend class ConditionVariable;
    // Don't allow move because this class has a non-movable object.
    DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

class AutoWriteLock {
public:
    AutoWriteLock(ReadWriteLock& lock) : mLock(lock) { mLock.lockWrite(); }

    void lockWrite() {
        assert(!mWriteLocked);
        mLock.lockWrite();
        mWriteLocked = true;
    }

    void unlockWrite() {
        assert(mWriteLocked);
        mLock.unlockWrite();
        mWriteLocked = false;
    }

    ~AutoWriteLock() {
        if (mWriteLocked) {
            mLock.unlockWrite();
        }
    }

private:
    ReadWriteLock& mLock;
    bool mWriteLocked = true;
    // This class has a non-movable object.
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoWriteLock);
};

class AutoReadLock {
public:
    AutoReadLock(ReadWriteLock& lock) : mLock(lock) { mLock.lockRead(); }

    void lockRead() {
        assert(!mReadLocked);
        mLock.lockRead();
        mReadLocked = true;
    }

    void unlockRead() {
        assert(mReadLocked);
        mLock.unlockRead();
        mReadLocked = false;
    }

    ~AutoReadLock() {
        if (mReadLocked) {
            mLock.unlockRead();
        }
    }

private:
    ReadWriteLock& mLock;
    bool mReadLocked = true;
    // This class has a non-movable object.
    DISALLOW_COPY_ASSIGN_AND_MOVE(AutoReadLock);
};

// Seqlock (cross platform)
// Based on:
// https://lwn.net/Articles/21812/
// https://github.com/rigtorp/Seqlock
//
// A seqlock is meant to address performance issues with using reader/writer
// locks to protect data structures where the time spent performing operations
// while the lock is held is very short or even comparable to the time spent
// locking/unlocking in the first place. This is very common in situations
// where we have some globally accessible array of objects and multiple threads
// performing short little read/write operations on them (i.e., pretty much
// anything that uses entity component system architecture that needs to be
// accessed by multiple threads).
//
// The basic idea of a seqlock is to store a sequence number (like a version
// number) that writers increment, but readers only read. When beginning write
// access, the sequence number is incremented, and after write access ends, the
// sequence number is incremented again. This way, when a reader is trying to
// read and it notices a change in the sequence number (or, as an optimization,
// that the number is odd (because writes should always end up incrementing the
// sequence number by 2 if they complete)), it can try again until there is no
// change.
//
// The problem, however, is that we need to be very careful about how we set
// and compare the sequence numbers, because compilers/hardware easily reorder
// instructions involving what seems to be just simple integer arithmetic.
// (see https://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf) Atomic
// primitives need to be used for all accesses to the sequence number.
//
// In particular, the atomic updates to the sequence number and the actual
// non-atomic data accesses are allowed to be reordered by the compiler, which
// introduces problems when accessing the data (still allowing reads of an
// update in progress); we need smp_rmb.
// https://elixir.bootlin.com/linux/latest/source/tools/arch/arm64/include/asm/barrier.h#L25
//
// arm64: memory barrier instruction
// asm volatile("dmb ishld" ::: "memory")
// x86: compiler barrier
// std::atomic_signal_fence(std::memory_order_acq_rel);
//
// This smp_rmb needs to be added before and after the read operation.
//
// On the write side, we use 
// arm64: memory barrier instruction
// asm volatile("dmb ishst" ::: "memory")
// x86: compiler barrier
// std::atomic_signal_fence(std::memory_order_acq_rel);
// 
// https://github.com/rigtorp/Seqlock has a version that seems to address these issues, while
// https://elixir.bootlin.com/linux/latest/source/include/linux/seqlock.h shows how to implement in the kernel.
//
static inline __attribute__((always_inline)) void SmpWmb() {
#if defined(__aarch64__)
        asm volatile("dmb ishst" ::: "memory");
#elif defined(__x86_64__)
        std::atomic_thread_fence(std::memory_order_release);
#else
#error "Unimplemented SmpWmb for current CPU architecture"
#endif
}

static inline __attribute__((always_inline)) void SmpRmb() {
#if defined(__aarch64__)
        asm volatile("dmb ishld" ::: "memory");
#elif defined(__x86_64__)
        std::atomic_thread_fence(std::memory_order_acquire);
#else
#error "Unimplemented SmpRmb for current CPU architecture"
#endif
}

class SeqLock {
public:
    void beginWrite() {
        mWriteLock.lock();
        mSeq.fetch_add(1, std::memory_order_release);
        SmpWmb();
    }

    void endWrite() {
        SmpWmb();
        mSeq.fetch_add(1, std::memory_order_release);
        mWriteLock.unlock();
    }

#ifdef __cplusplus
#   define SEQLOCK_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#   define SEQLOCK_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))
#else
#   define SEQLOCK_LIKELY( exp )    (__builtin_expect( !!(exp), 1 ))
#   define SEQLOCK_UNLIKELY( exp )  (__builtin_expect( !!(exp), 0 ))
#endif

    uint32_t beginRead() {
        uint32_t res;

        // see https://elixir.bootlin.com/linux/latest/source/include/linux/seqlock.h#L128; if odd we definitely know there's a write in progress, and shouldn't proceed any further.
repeat:
        res = mSeq.load(std::memory_order_acquire);
        if (SEQLOCK_UNLIKELY(res & 1)) {
            goto repeat;
        }

        SmpRmb();
        return res;
    }

    bool shouldRetryRead(uint32_t prevSeq) {
        SmpRmb();
        uint32_t res = mSeq.load(std::memory_order_acquire);
        return (res != prevSeq);
    }

    // Convenience class for write
    class ScopedWrite {
    public:
        ScopedWrite(SeqLock* lock) : mLock(lock) {
            mLock->beginWrite();
        }
        ~ScopedWrite() {
            mLock->endWrite();
        }
    private:
        SeqLock* mLock;
    };

    // Convenience macro for read (no std::function due to its considerable overhead)
#define AEMU_SEQLOCK_READ_WITH_RETRY(lock, readStuff) { uint32_t aemu_seqlock_curr_seq; do { \
    aemu_seqlock_curr_seq = (lock)->beginRead(); \
    readStuff; \
    } while ((lock)->shouldRetryRead(aemu_seqlock_curr_seq)); }

private:
    std::atomic<uint32_t> mSeq { 0 }; // The sequence number
    Lock mWriteLock; // Just use a normal mutex to protect writes
};

}  // namespace base
}  // namespace android

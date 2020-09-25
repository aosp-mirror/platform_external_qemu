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

#include "android/base/testing/TestThread.h"
#include "android/base/threads/FunctorThread.h"

#include <gtest/gtest.h>

namespace android {
namespace base {

// Check that construction and destruction doesn't crash.
TEST(Lock, ConstructionDestruction) {
    Lock lock;
}

// Check that a simple lock + unlock works.
TEST(Lock, LockUnlock) {
    Lock lock;
    lock.lock();
    lock.unlock();
}

// Check that AutoLock compiles and doesn't crash.
TEST(Lock, AutoLock) {
    Lock mutex;
    AutoLock lock(mutex);
}

// Wrapper class for threads. Does not use emugl::Thread intentionally.
// Common data type used by the following tests below.
struct ThreadParams {
    ThreadParams() : mutex(), counter(0) {}

    Lock mutex;
    int counter;
};

// This thread function uses Lock::lock/unlock to synchronize a counter
// increment operation.
static void* threadFunction(void* param) {
    ThreadParams* p = static_cast<ThreadParams*>(param);

    p->mutex.lock();
    p->counter++;
    p->mutex.unlock();
    return NULL;
}

TEST(Lock, Synchronization) {
    const size_t kNumThreads = 2000;
    TestThread* threads[kNumThreads];
    ThreadParams p;

    // Create and launch all threads.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n] = new TestThread(threadFunction, &p);
    }

    // Wait until their completion.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n]->join();
        delete threads[n];
    }

    EXPECT_EQ(static_cast<int>(kNumThreads), p.counter);
}

// This thread function uses a AutoLock to protect the counter.
static void* threadAutoLockFunction(void* param) {
    ThreadParams* p = static_cast<ThreadParams*>(param);

    AutoLock lock(p->mutex);
    p->counter++;
    return NULL;
}

TEST(Lock, AutoLockSynchronization) {
    const size_t kNumThreads = 2000;
    TestThread* threads[kNumThreads];
    ThreadParams p;

    // Create and launch all threads.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n] = new TestThread(threadAutoLockFunction, &p);
    }

    // Wait until their completion.
    for (size_t n = 0; n < kNumThreads; ++n) {
        threads[n]->join();
        delete threads[n];
    }

    EXPECT_EQ(static_cast<int>(kNumThreads), p.counter);
}

// Seqlock tests
static constexpr size_t kSeqLockTestDataArraySize = 100;

struct SeqLockTestData {
    // 100 numbers, make partial reads very obvious
    int nums[kSeqLockTestDataArraySize];
};

static void setTestData(struct SeqLockTestData* data, int val) {
    for (uint32_t i = 0; i < kSeqLockTestDataArraySize; ++i) {
        data->nums[i] = val;
    }
}

static bool checkTestData(const struct SeqLockTestData* data, int val) {
    for (uint32_t i = 0; i < kSeqLockTestDataArraySize; ++i) {
        if (val != data->nums[i]) {
            return false;
        }
    }
    return true;
}

static bool checkTestDataConsistency(const struct SeqLockTestData* data) {
    int currVal = data->nums[0];
    for (uint32_t i = 1; i < kSeqLockTestDataArraySize; ++i) {
        if (currVal != data->nums[i]) {
            fprintf(stderr, "%s: error: currVal %d vs %d %d\n", __func__, currVal, data->nums[i], i);
            return false;
        }
    }
    return true;
}

// Very basic single thread.
TEST(SeqLock, BasicSingleThread) {
    SeqLockTestData data;

    SeqLock sl;

    {
        SeqLock::ScopedWrite write(&sl);
        setTestData(&data, 1);
    }

    SeqLockTestData dataCopy;

    AEMU_SEQLOCK_READ_WITH_RETRY(&sl,
        memcpy(&dataCopy, &data, sizeof(data)));

    EXPECT_TRUE(checkTestData(&dataCopy, 1));
}

class SeqLock2 {
public:
    void beginWrite() {
        mWriteLock.lock();
        SmpWmb();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        mSeq.fetch_add(1, std::memory_order_seq_cst);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        SmpWmb();
    }

    void endWrite() {
        SmpWmb();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        mSeq.fetch_add(1, std::memory_order_seq_cst);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        SmpWmb();
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
        SmpRmb();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        res = mSeq.load(std::memory_order_seq_cst);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        SmpRmb();
        if (res & 1) {
            goto repeat;
        }

        std::atomic_thread_fence(std::memory_order_seq_cst);
        SmpRmb();
        return res;
    }

    bool shouldRetryRead(uint32_t prevSeq) {
        SmpRmb();
        std::atomic_thread_fence(std::memory_order_seq_cst);
        uint32_t res = mSeq.load(std::memory_order_seq_cst);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        SmpRmb();
        return (res != prevSeq);
    }

    // Convenience class for write
    class ScopedWrite {
    public:
        ScopedWrite(SeqLock2* lock) : mLock(lock) {
            mLock->beginWrite();
        }
        ~ScopedWrite() {
            mLock->endWrite();
        }
    private:
        SeqLock2* mLock;
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


struct SeqLockSharedState {
    SeqLock2* sl;
    uint32_t* counter;
    SeqLockTestData* data;
};

// SeqLock test with 1 writer and 1 reader.
TEST(SeqLock, WriterReader) {
    uint32_t counter = 0;
    SeqLockTestData data;
    setTestData(&data, 0);
    SeqLock2 sl;
    SeqLockSharedState state;
    state.sl = &sl;
    state.data = &data;

    static constexpr uint32_t kNumIters = 100000;

    FunctorThread writer([this, &state]() {
        for (uint32_t i = 0; i < kNumIters; ++i) {
            SeqLock2::ScopedWrite(state.sl);
            *(state.counter) = *(state.counter) + 1;
            // setTestData(state.data, *(state.counter));
                for (int j = 0; j < kSeqLockTestDataArraySize; ++j) {
                    __atomic_store_n(&state.data->nums[j], *(state.counter), __ATOMIC_SEQ_CST);
                }
        }
    });

    FunctorThread reader([this, &state]() {
        SeqLockTestData readerCopy;
        for (uint32_t i = 0; i < kNumIters; ++i) {
            uint32_t seq = 0;
            do {
                seq = state.sl->beginRead();
                // memcpy(&readerCopy, state.data, sizeof(readerCopy));
                for (int j = 0; j < kSeqLockTestDataArraySize; ++j) {
                    int num = __atomic_load_n(&state.data->nums[j], __ATOMIC_SEQ_CST);
                    readerCopy.nums[j] = num;
                }
            } while(state.sl->shouldRetryRead(seq));
            EXPECT_TRUE(checkTestDataConsistency(&readerCopy));
        }
    });

    writer.start();
    reader.start();

    writer.wait();
    reader.wait();

    EXPECT_TRUE(checkTestData(&data, kNumIters));
}

}  // namespace base
}  // namespace android

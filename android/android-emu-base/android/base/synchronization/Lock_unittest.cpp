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

#include "aemu/base/synchronization/Lock.h"

#include "android/base/testing/TestThread.h"
#include "aemu/base/threads/FunctorThread.h"

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
static constexpr size_t kSeqLockTestDataArraySize = 200;

struct SeqLockTestData {
    // Make partial reads very obvious
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

struct SeqLockSharedState {
    SeqLock* sl;
    uint32_t* counter;
    SeqLockTestData* data;
};

// SeqLock test with 1 writer and some number of readers.
TEST(SeqLock, WriterReaders) {
    uint32_t counter = 0;
    SeqLockTestData data;
    setTestData(&data, 0);
    SeqLock sl;
    SeqLockSharedState state;
    state.sl = &sl;
    state.data = &data;
    state.counter = &counter;

    static constexpr uint32_t kNumIters = 100000;
    static constexpr uint32_t kReaderCount = 16;

    FunctorThread writer([this, &state]() {
        for (uint32_t i = 0; i < kNumIters; ++i) {
            SeqLock::ScopedWrite write(state.sl);
            *(state.counter) = *(state.counter) + 1;
            setTestData(state.data, *(state.counter));
        }
    });

    auto readerFunc = [this, &state]() {
        SeqLockTestData readerCopy;
        for (uint32_t i = 0; i < kNumIters; ++i) {
            uint32_t seq = 0;
            do {
                seq = state.sl->beginRead();
                memcpy(&readerCopy, state.data, sizeof(readerCopy));
            } while(state.sl->shouldRetryRead(seq));
            EXPECT_TRUE(checkTestDataConsistency(&readerCopy));
        }
    };

    std::vector<FunctorThread*> readers;

    for (uint32_t i = 0; i < kReaderCount; ++i) {
        readers.push_back(new FunctorThread(readerFunc));
    }

    for (uint32_t i = 0; i < kReaderCount; ++i) {
        readers[i]->start();
    }

    writer.start();
    writer.wait();

    for (uint32_t i = 0; i < kReaderCount; ++i) {
        readers[i]->wait();
        delete readers[i];
    }

    EXPECT_TRUE(checkTestData(&data, kNumIters));
}

}  // namespace base
}  // namespace android

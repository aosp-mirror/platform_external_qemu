// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/base/ring_buffer.h"

#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"

#include <gtest/gtest.h>

#include <random>

#include <errno.h>
#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

namespace android {
namespace base {

TEST(ring_buffer, Init) {
    ring_buffer r;
    ring_buffer_init(&r);
}

static constexpr size_t kNumElts = 65536;

// Tests that a large buffer can be produced and consumed,
// in a single thread.
TEST(ring_buffer, ProduceConsume) {
    std::default_random_engine generator;
    generator.seed(0);

    std::vector<uint8_t> elements(kNumElts);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    for (size_t i = 0; i < kNumElts; ++i) {
        elements[i] =
            static_cast<uint8_t>(
                eltDistribution(generator));
    }

    std::vector<uint8_t> result(kNumElts);

    ring_buffer r;
    ring_buffer_init(&r);

    size_t written = 0;
    size_t read = 0;

    int i = 0;
    while (written < kNumElts) {
        ++i;

        // Safety factor; we do not expect the ring buffer
        // implementation to be this hangy if used this way.
        if (i > kNumElts * 10) {
            FAIL() << "Error: too many iterations. Hanging?";
            return;
        }

        uint32_t toWrite = kNumElts - written;
        long writtenThisTime =
            ring_buffer_write(&r, elements.data() + written, 1, toWrite);
        written += writtenThisTime;

        if (writtenThisTime < toWrite) {
            EXPECT_EQ(-EAGAIN, errno);
        }

        uint32_t toRead = kNumElts - read;
        long readThisTime =
            ring_buffer_read(&r, result.data() + read, 1, toRead);
        read += readThisTime;

        if (readThisTime < toRead) {
            EXPECT_EQ(-EAGAIN, errno);
        }
    }

    EXPECT_EQ(elements, result);
}

// General function to pass to FunctorThread to read/write
// data completely to/from a ring buffer.
static void writeTest(ring_buffer* r, const uint8_t* data, size_t stepSize, size_t numSteps) {
    size_t stepsWritten = 0;
    size_t bytes = stepSize * numSteps;
    int i = 0;
    while (stepsWritten < numSteps) {
        ++i;

        // Safety factor; we do not expect the ring buffer
        // implementation to be this hangy if used this way.
        if (i > bytes * 10) {
            FAIL() << "Error: too many iterations. Hanging?";
            return;
        }

        uint32_t stepsRemaining = numSteps - stepsWritten;
        long stepsWrittenThisTime =
            ring_buffer_write(r,
                data + stepSize * stepsWritten,
                stepSize, stepsRemaining);
        stepsWritten += stepsWrittenThisTime;

        if (stepsWrittenThisTime < stepsRemaining) {
            EXPECT_EQ(-EAGAIN, errno);
        }
    }
}

static void readTest(ring_buffer* r, uint8_t* data, size_t stepSize, size_t numSteps) {
    size_t stepsRead = 0;
    size_t bytes = stepSize * numSteps;
    int i = 0;
    while (stepsRead < numSteps) {
        ++i;

        // Safety factor; we do not expect the ring buffer
        // implementation to be this hangy if used this way.
        if (i > bytes * 10) {
            FAIL() << "Error: too many iterations. Hanging?";
            return;
        }

        uint32_t stepsRemaining = numSteps - stepsRead;
        long stepsReadThisTime =
            ring_buffer_read(r,
                data + stepSize * stepsRead,
                stepSize, stepsRemaining);
        stepsRead += stepsReadThisTime;

        if (stepsReadThisTime < stepsRemaining) {
            EXPECT_EQ(-EAGAIN, errno);
        }
    }
}

// Tests transmission of a large buffer where
// the producer is in one thread
// while the consumer is in another thread.
TEST(ring_buffer, ProduceConsumeMultiThread) {
    std::default_random_engine generator;
    generator.seed(0);

    std::vector<uint8_t> elements(kNumElts);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    for (size_t i = 0; i < kNumElts; ++i) {
        elements[i] =
            static_cast<uint8_t>(
                eltDistribution(generator));
    }

    std::vector<uint8_t> result(kNumElts, 0);

    ring_buffer r;
    ring_buffer_init(&r);

    FunctorThread producer([&r, &elements]() {
        writeTest(&r, (uint8_t*)elements.data(), 1, kNumElts);
    });

    FunctorThread consumer([&r, &result]() {
        readTest(&r, (uint8_t*)result.data(), 1, kNumElts);
    });

    producer.start();
    consumer.start();

    consumer.wait();

    EXPECT_EQ(elements, result);
}

// Tests various step sizes of ring buffer transmission.
TEST(ring_buffer, ProduceConsumeMultiThreadVaryingStepSize) {
    std::default_random_engine generator;
    generator.seed(0);

    std::vector<uint8_t> elements(kNumElts);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    for (size_t i = 0; i < kNumElts; ++i) {
        elements[i] =
            static_cast<uint8_t>(
                eltDistribution(generator));
    }

    static constexpr size_t kStepSizesToTest[] = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,
    };

    for (auto stepSize : kStepSizesToTest) {
        size_t numSteps = kNumElts / stepSize;

        std::vector<uint8_t> result(kNumElts, 0);

        ring_buffer r;
        ring_buffer_init(&r);

        FunctorThread producer([&r, &elements, stepSize, numSteps]() {
            writeTest(&r, (uint8_t*)elements.data(), stepSize, numSteps);
        });

        FunctorThread consumer([&r, &result, stepSize, numSteps]() {
            readTest(&r, (uint8_t*)result.data(), stepSize, numSteps);
        });

        producer.start();
        consumer.start();

        consumer.wait();

        EXPECT_EQ(elements, result);
    }
}

static void viewWriteTest(ring_buffer* r, ring_buffer_view* v, const uint8_t* data, size_t stepSize, size_t numSteps) {
    size_t stepsWritten = 0;
    size_t bytes = stepSize * numSteps;
    int i = 0;
    while (stepsWritten < numSteps) {
        ++i;

        // Safety factor; we do not expect the ring buffer
        // implementation to be this hangy if used this way.
        if (i > bytes * 10) {
            FAIL() << "Error: too many iterations. Hanging?";
            return;
        }

        uint32_t stepsRemaining = numSteps - stepsWritten;
        long stepsWrittenThisTime =
            ring_buffer_view_write(r, v,
                data + stepSize * stepsWritten,
                stepSize, stepsRemaining);
        stepsWritten += stepsWrittenThisTime;

        if (stepsWrittenThisTime < stepsRemaining) {
            EXPECT_EQ(-EAGAIN, errno);
        }
    }
}

static void viewReadTest(ring_buffer* r, ring_buffer_view* v, uint8_t* data, size_t stepSize, size_t numSteps) {
    size_t stepsRead = 0;
    size_t bytes = stepSize * numSteps;
    int i = 0;
    while (stepsRead < numSteps) {
        ++i;

        // Safety factor; we do not expect the ring buffer
        // implementation to be this hangy if used this way.
        if (i > bytes * 10) {
            FAIL() << "Error: too many iterations. Hanging?";
            return;
        }

        uint32_t stepsRemaining = numSteps - stepsRead;
        long stepsReadThisTime =
            ring_buffer_view_read(r, v,
                data + stepSize * stepsRead,
                stepSize, stepsRemaining);
        stepsRead += stepsReadThisTime;

        if (stepsReadThisTime < stepsRemaining) {
            EXPECT_EQ(-EAGAIN, errno);
        }
    }
}

// Tests ring_buffer_calc_shift.
TEST(ring_buffer, CalcShift) {
    EXPECT_EQ(0, ring_buffer_calc_shift(1));
    EXPECT_EQ(1, ring_buffer_calc_shift(2));
    EXPECT_EQ(1, ring_buffer_calc_shift(3));
    EXPECT_EQ(2, ring_buffer_calc_shift(4));
    EXPECT_EQ(2, ring_buffer_calc_shift(5));
    EXPECT_EQ(2, ring_buffer_calc_shift(6));
    EXPECT_EQ(2, ring_buffer_calc_shift(7));
    EXPECT_EQ(3, ring_buffer_calc_shift(8));
}

// Tests usage of ring buffer with view.
TEST(ring_buffer, ProduceConsumeMultiThreadVaryingStepSizeWithView) {
    std::default_random_engine generator;
    generator.seed(0);

    std::vector<uint8_t> elements(kNumElts);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    for (size_t i = 0; i < kNumElts; ++i) {
        elements[i] =
            static_cast<uint8_t>(
                eltDistribution(generator));
    }

    static constexpr size_t kStepSizesToTest[] = {
        1, 2, 4, 8, 16, 32, 64,
        1024, 2048, 4096,
    };

    for (auto stepSize : kStepSizesToTest) {
        size_t numSteps = kNumElts / stepSize;

        std::vector<uint8_t> result(kNumElts, 0);

        // non power of 2
        std::vector<uint8_t> buf(8193, 0);

        ring_buffer r;
        ring_buffer_view v;
        ring_buffer_view_init(&r, &v, buf.data(), buf.size());

        FunctorThread producer([&r, &v, &elements, stepSize, numSteps]() {
            viewWriteTest(&r, &v, (uint8_t*)elements.data(), stepSize, numSteps);
        });

        FunctorThread consumer([&r, &v, &result, stepSize, numSteps]() {
            viewReadTest(&r, &v, (uint8_t*)result.data(), stepSize, numSteps);
        });

        producer.start();
        consumer.start();

        consumer.wait();

        EXPECT_EQ(elements, result);
    }
}

// Tests that wait works as expected
TEST(ring_buffer, Wait) {
    ring_buffer r;
    ring_buffer_init(&r);

    EXPECT_TRUE(ring_buffer_wait_write(&r, nullptr, 1, 0));
    EXPECT_FALSE(ring_buffer_wait_read(&r, nullptr, 1, 0));

    EXPECT_TRUE(ring_buffer_wait_write(&r, nullptr, 1, 100));
    EXPECT_FALSE(ring_buffer_wait_read(&r, nullptr, 1, 100));
}

// Tests the read/write fully operations
TEST(ring_buffer, FullReadWrite) {
    ring_buffer r;
    ring_buffer_init(&r);

    std::default_random_engine generator;
    generator.seed(0);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        testSizeDistribution(1, 8192);

    std::uniform_int_distribution<int>
        bufSizeDistribution(256, 8192);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    size_t trials = 1000;

    for (size_t i = 0; i < trials; ++i) {
        size_t testSize =
            testSizeDistribution(generator);
        size_t bufSize =
            bufSizeDistribution(generator);

        std::vector<uint8_t> elements(testSize);
        std::vector<uint8_t> result(testSize);
        std::vector<uint8_t> buf(bufSize, 0);

        ring_buffer r;
        ring_buffer_view v;
        ring_buffer_view_init(&r, &v, buf.data(), buf.size());

        FunctorThread producer([&r, &v, &elements]() {
            ring_buffer_write_fully(&r, &v, elements.data(), elements.size());
        });

        FunctorThread consumer([&r, &v, &result]() {
            ring_buffer_read_fully(&r, &v, result.data(), result.size());
        });

        producer.start();
        consumer.start();

        consumer.wait();

        EXPECT_EQ(elements, result);
    }
}

// Tests synchronization with producer driving most things along with
// consumer hangup.
// The test: A producer thread runs and spawns consumer threads on demand. Once
// each consumer thread is done with a bit of traffic, they hang up.
// Currently disabled due to it hanging on Windows.
// TODO(lfy@): figure out why it hangs on windows
TEST(ring_buffer, DISABLED_ProducerDrivenSync) {
    std::default_random_engine generator;
    generator.seed(0);
    std::vector<uint8_t> elements(kNumElts);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    for (size_t i = 0; i < kNumElts; ++i) {
        elements[i] =
            static_cast<uint8_t>(
                eltDistribution(generator));
    }

    std::vector<uint8_t> result(kNumElts);

    ring_buffer r;
    ring_buffer_init(&r);
    ring_buffer_sync_init(&r);
    size_t read = 0;
    const size_t totalTestLength = kNumElts * 64;

    FunctorThread consumer([&r, &result, &read]() {
        while (read < totalTestLength) {
            if (ring_buffer_wait_read(&r, nullptr, 1, 1)) {
                ring_buffer_read_fully(
                    &r, nullptr, result.data() + (read % result.size()), 1);
                ++read;
            } else {
                if (!ring_buffer_consumer_hangup(&r)) {
                    EXPECT_NE(RING_BUFFER_SYNC_CONSUMER_HANGING_UP, r.state);
                    ring_buffer_consumer_wait_producer_idle(&r);
                    while (ring_buffer_can_read(&r, 1)) {
                        ring_buffer_read_fully(
                            &r, nullptr, result.data() + (read % result.size()), 1);
                        ++read;
                    }
                }
                ring_buffer_consumer_hung_up(&r);
            }
        }
    });

    consumer.start();

    FunctorThread producer([&r, &elements, &result]() {
        size_t written = 0;
        while (written < totalTestLength) {
            if (!ring_buffer_producer_acquire(&r)) {
                EXPECT_TRUE(
                    r.state == RING_BUFFER_SYNC_CONSUMER_HANGING_UP ||
                    r.state == RING_BUFFER_SYNC_CONSUMER_HUNG_UP);
                ring_buffer_producer_idle(&r);
                ring_buffer_producer_wait_hangup(&r);
                EXPECT_TRUE(ring_buffer_producer_acquire_from_hangup(&r));
            }
            ring_buffer_write_fully(
                &r, nullptr,
                elements.data() + (written % elements.size()), 1);
            ++written;
            ring_buffer_producer_idle(&r);
        }
    });

    producer.start();
    consumer.wait();

    EXPECT_EQ(elements, result);
}

// Tests the read/write fully operations
TEST(ring_buffer, SpeedTest) {
    std::default_random_engine generator;
    generator.seed(0);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    size_t testSize = 1048576 * 8;
    size_t bufSize = 16384;

    std::vector<uint8_t> elements(testSize);

    for (size_t i = 0; i < testSize; ++i) {
        elements[i] = static_cast<uint8_t>(eltDistribution(generator));
    }

    std::vector<uint8_t> result(testSize);

    std::vector<uint8_t> buf(bufSize, 0);

    ring_buffer r;
    ring_buffer_view v;
    ring_buffer_view_init(&r, &v, buf.data(), buf.size());

    size_t bytesSent = 0;
    size_t totalCycles = 5;

    float mbPerSec = 0.0f;

    struct timeval tv;

    for (size_t i = 0; i < totalCycles; ++i) {

        ring_buffer_view_init(&r, &v, buf.data(), buf.size());

        uint64_t start_us = System::get()->getHighResTimeUs();

        FunctorThread producer([&r, &v, &elements]() {
            ring_buffer_write_fully(&r, &v, elements.data(), elements.size());
        });

        FunctorThread consumer([&r, &v, &result]() {
            ring_buffer_read_fully(&r, &v, result.data(), result.size());
        });

        producer.start();
        consumer.start();
        consumer.wait();

        uint64_t end_us = System::get()->getHighResTimeUs();

        if (i % 10 == 0) {
            fprintf(stderr, "%s: ring stats: live yield sleep %lu %lu %lu\n", __func__,
                (unsigned long)r.read_live_count,
                (unsigned long)r.read_yield_count,
                (unsigned long)r.read_sleep_us_count);
        }
        mbPerSec += (float(testSize) / (end_us - start_us));
    }

    mbPerSec = mbPerSec / totalCycles;

    fprintf(stderr, "%s: avg mb per sec: %f\n", __func__, mbPerSec);
}

// Tests copying out the contents available for read
// without incrementing the read index.
TEST(ring_buffer, CopyContents) {
    std::vector<uint8_t> elements = {
        0x1, 0x2, 0x3, 0x4,
        0x5, 0x6, 0x7, 0x8,
    };

    std::vector<uint8_t> buf(4, 0);

    std::vector<uint8_t> recv(elements.size(), 0);

    ring_buffer r;
    ring_buffer_view v;
    ring_buffer_view_init(&r, &v, buf.data(), buf.size());

    EXPECT_EQ(true, ring_buffer_view_can_write(&r, &v, 3));
    EXPECT_EQ(0, ring_buffer_available_read(&r, &v));

    uint8_t* elementsPtr = elements.data();
    uint8_t* recvPtr = recv.data();

    EXPECT_EQ(1, ring_buffer_view_write(&r, &v, elementsPtr, 1, 1));
    EXPECT_FALSE(ring_buffer_view_can_write(&r, &v, 3));
    EXPECT_TRUE(ring_buffer_view_can_write(&r, &v, 2));
    EXPECT_EQ(1, ring_buffer_available_read(&r, &v));
    EXPECT_EQ(0, ring_buffer_copy_contents(&r, &v, 1, recvPtr));
    EXPECT_EQ(0x1, *recvPtr);
    EXPECT_EQ(1, ring_buffer_available_read(&r, &v));
    EXPECT_EQ(1, ring_buffer_view_read(&r, &v, recvPtr, 1, 1));
    EXPECT_EQ(0, ring_buffer_available_read(&r, &v));
    EXPECT_TRUE(ring_buffer_view_can_write(&r, &v, 3));

    ++elementsPtr;
    ++recvPtr;

    EXPECT_EQ(1, ring_buffer_view_write(&r, &v, elementsPtr, 3, 1));
    EXPECT_FALSE(ring_buffer_view_can_write(&r, &v, 3));
    EXPECT_EQ(3, ring_buffer_available_read(&r, &v));
    EXPECT_EQ(0, ring_buffer_copy_contents(&r, &v, 3, recvPtr));
    EXPECT_EQ(0x2, recvPtr[0]);
    EXPECT_EQ(0x3, recvPtr[1]);
    EXPECT_EQ(0x4, recvPtr[2]);
    EXPECT_EQ(3, ring_buffer_available_read(&r, &v));
    EXPECT_EQ(1, ring_buffer_view_read(&r, &v, recvPtr, 3, 1));
    EXPECT_EQ(0, ring_buffer_available_read(&r, &v));
    EXPECT_TRUE(ring_buffer_view_can_write(&r, &v, 3));

    elementsPtr += 3;
    recvPtr += 3;

    EXPECT_EQ(1, ring_buffer_view_write(&r, &v, elementsPtr, 3, 1));
    EXPECT_FALSE(ring_buffer_view_can_write(&r, &v, 3));
    EXPECT_EQ(3, ring_buffer_available_read(&r, &v));
    EXPECT_EQ(0, ring_buffer_copy_contents(&r, &v, 3, recvPtr));
    EXPECT_EQ(0x5, recvPtr[0]);
    EXPECT_EQ(0x6, recvPtr[1]);
    EXPECT_EQ(0x7, recvPtr[2]);
    EXPECT_EQ(3, ring_buffer_available_read(&r, &v));
    EXPECT_EQ(1, ring_buffer_view_read(&r, &v, recvPtr, 3, 1));
    EXPECT_EQ(0, ring_buffer_available_read(&r, &v));
    EXPECT_TRUE(ring_buffer_view_can_write(&r, &v, 3));
}

} // namespace android
} // namespace base

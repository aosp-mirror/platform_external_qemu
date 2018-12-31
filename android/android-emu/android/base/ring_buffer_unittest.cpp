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

#include "android/base/threads/FunctorThread.h"

#include <gtest/gtest.h>

#include <random>

#include <errno.h>

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

// Tests usage of ring buffer with view.
TEST(ring_buffer, ProduceConsumeMultiThreadVaryingStepSizeWithView) {
    std::default_random_engine generator;
    generator.seed(0);

    size_t testSize = 65536;
    std::vector<uint8_t> elements(testSize);

    // int for toolchain compatibility
    std::uniform_int_distribution<int>
        eltDistribution(0, 255);

    for (size_t i = 0; i < testSize; ++i) {
        elements[i] =
            static_cast<uint8_t>(
                eltDistribution(generator));
    }

    static constexpr size_t kStepSizesToTest[] = {
        1, 2, 4, 8, 16, 32, 64,
        1024, 2048, 4096,
    };

    for (auto stepSize : kStepSizesToTest) {
        size_t numSteps = testSize / stepSize;

        std::vector<uint8_t> result(testSize, 0);

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

// TEST(ring_buffer, Wait) {
//     ring_buffer r;
//     ring_buffer_init(&r);
//     ring_buffer_wait_read(&r, 0, 1);
// }
// 
} // namespace android
} // namespace base

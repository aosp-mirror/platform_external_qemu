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
            ring_buffer_write(&r, elements.data() + written, toWrite);
        written += writtenThisTime;

        if (writtenThisTime < toWrite) {
            EXPECT_EQ(-EAGAIN, errno);
        }

        uint32_t toRead = kNumElts - read;
        long readThisTime =
            ring_buffer_read(&r, result.data() + read, toRead);
        read += readThisTime;

        if (readThisTime < toRead) {
            EXPECT_EQ(-EAGAIN, errno);
        }
    }

    EXPECT_EQ(elements, result);
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

    std::vector<uint8_t> result(kNumElts);
    bool done = false;
    size_t consumed = 0;

    ring_buffer r;
    ring_buffer_init(&r);

    FunctorThread producer([&r, &elements]() {
        size_t written = 0;
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
                ring_buffer_write(&r, elements.data() + written, toWrite);
            written += writtenThisTime;

            if (writtenThisTime < toWrite) {
                EXPECT_EQ(-EAGAIN, errno);
            }
        }
    });

    FunctorThread consumer([&r, &result]() {
        size_t read = 0;
        int i = 0;
        while (read < kNumElts) {
            ++i;
    
            // Safety factor; we do not expect the ring buffer
            // implementation to be this hangy if used this way.
            if (i > kNumElts * 10) {
                FAIL() << "Error: too many iterations. Hanging?";
                return;
            }
    
            uint32_t toRead = kNumElts - read;
            long readThisTime =
                ring_buffer_read(&r, result.data() + read, toRead);
            read += readThisTime;
    
            if (readThisTime < toRead) {
                EXPECT_EQ(-EAGAIN, errno);
            }
        }
    });

    producer.start();
    consumer.start();

    consumer.wait();

    EXPECT_EQ(elements, result);
}
} // namespace android
} // namespace base

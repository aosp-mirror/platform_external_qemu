// Copyright (C) 2019 The Android Open Source Project
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

#include "android/emulation/control/logcat/RingStreambuf.h"

#include <gtest/gtest.h>

#include "android/base/threads/FunctorThread.h"

namespace android {
namespace emulation {
namespace control {

TEST(RingStreambuf, basic_stream_avail) {
    RingStreambuf buf(5);
    std::ostream stream(&buf);
    stream << "hi";
    EXPECT_EQ(2, buf.in_avail());
}

TEST(RingStreambuf, basic_stream) {
    RingStreambuf buf(6);
    std::ostream stream(&buf);
    stream << "hello";
    auto res = buf.bufferAtOffset(0);
    EXPECT_EQ(res.first, 0);
    EXPECT_STREQ("hello", res.second.c_str());
}

TEST(RingStreambuf, basic_stream_offset) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "AABB";
    auto res = buf.bufferAtOffset(2);
    EXPECT_EQ(res.first, 2);
    EXPECT_STREQ("BB", res.second.c_str());
}

TEST(RingStreambuf, stream_offset_takes_earliest_available) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaa";
    stream << "bbbb";
    auto res = buf.bufferAtOffset(0);
    EXPECT_EQ(res.first, 4);
    EXPECT_STREQ("bbbb", res.second.c_str());

    res = buf.bufferAtOffset(4);
    EXPECT_EQ(res.first, 4);
    EXPECT_STREQ("bbbb", res.second.c_str());
}

TEST(RingStreambuf, no_loss_when_iterating) {
    // This simulates calling getLogcat over and over.
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    int offset = 0;
    for (int i = 0; i < 26; i++) {
        std::string write;
        write += ('a' + i);
        write += ('a' + i);
        write += ('a' + i);
        write += ('a' + i);
        stream << write;
        auto res = buf.bufferAtOffset(offset);

        EXPECT_EQ(offset, i * 4);
        EXPECT_EQ(res.first, i * 4);
        EXPECT_STREQ(write.c_str(), res.second.c_str());

        // Move to the next offset where we find data.
        offset = res.first + res.second.size();
    }
}

TEST(RingStreambuf, can_also_read) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    std::istream in(&buf);
    int offset = 0;
    for (int i = 0; i < 26; i++) {
        std::string write;
        std::string read;
        write += ('a' + i);
        write += ('a' + i);
        write += ('a' + i);
        stream << write << "\n";
        in >> read;
        EXPECT_STREQ(read.c_str(), write.c_str());
    }
}

TEST(RingStreambuf, stream_overwrites_ring) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaa";
    stream << "bbbb";

    auto res = buf.bufferAtOffset(0, 1000);
    EXPECT_EQ(res.first, 4);
    EXPECT_STREQ("bbbb", res.second.c_str());

    res = buf.bufferAtOffset(4);
    EXPECT_EQ(res.first, 4);
    EXPECT_STREQ("bbbb", res.second.c_str());
}

TEST(RingStreambuf, stream_not_yet_available_no_block) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaa";

    auto res = buf.bufferAtOffset(8);
    EXPECT_EQ(res.first, 4);
    EXPECT_STREQ("", res.second.c_str());
}

TEST(RingStreambuf, stream_offset_blocks_until_available) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaa";
    FunctorThread writer([&stream] {
        Thread::sleepMs(100);
        stream << "bbbb";
        Thread::sleepMs(100);
        stream << "cccc";
    });
    FunctorThread reader([&buf] {
        auto res = buf.bufferAtOffset(8, 1000);
        EXPECT_EQ(res.first, 8);
        EXPECT_STREQ("cccc", res.second.c_str());
    });

    writer.start();
    reader.start();
    writer.wait(nullptr);
    reader.wait(nullptr);
}

}  // namespace control
}  // namespace emulation
}  // namespace android

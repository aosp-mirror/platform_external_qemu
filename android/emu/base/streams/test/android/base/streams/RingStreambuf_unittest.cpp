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
#include "aemu/base/streams/RingStreambuf.h"

#include <chrono>
#include <istream>
#include <ratio>
#include <string>
#include <thread>

#include <gtest/gtest.h>

namespace android {
namespace base {
namespace streams {

using namespace std::chrono_literals;
TEST(RingStreambuf, basic_stream_avail) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "hi";
    EXPECT_EQ(2, buf.in_avail());
}

TEST(RingStreambuf, no_write_after_close) {
    RingStreambuf buf(8);
    std::ostream stream(&buf);
    stream << "hi";
    EXPECT_EQ(2, buf.in_avail());
    buf.close();
    stream << "there";
    EXPECT_EQ(2, buf.in_avail());
}


TEST(RingStreambuf, stream_can_read_after_close) {
    RingStreambuf buf(8);
    std::ostream stream(&buf);
    std::istream in(&buf);
    std::string read;
    stream << "hi\n";
    buf.close();
    stream << "there\n";
    in >> read;
    EXPECT_EQ(read, "hi");
}

TEST(RingStreambuf, underflow_immediately_return_when_closed) {
    using namespace std::chrono_literals;

    // Timeout after 10ms..
    RingStreambuf buf(4, 5ms);
    auto start = std::chrono::steady_clock::now();

    // This results in an underflow (i.e. buffer is empty)
    // so after timeout we decleare eof.
    EXPECT_EQ(buf.sbumpc(), RingStreambuf::traits_type::eof());
    auto end = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // We must have waited at least 4ms.
    EXPECT_GT(diff, 4ms) <<  "Elapsed time in milliseconds: " << diff.count() << " ms.";

    // A closed buffer times out immediately
    start = std::chrono::steady_clock::now();
    buf.close();
    EXPECT_EQ(buf.sbumpc(), RingStreambuf::traits_type::eof());
    end = std::chrono::steady_clock::now();
    diff = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(diff, 1ms) <<  "Elapsed time in milliseconds: " << diff.count() << " ms.";
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
    stream << "aaaaaaa";
    stream << "bbbbbbb";
    auto res = buf.bufferAtOffset(0);
    EXPECT_EQ(res.first, 7);
    EXPECT_STREQ("bbbbbbb", res.second.c_str());

    res = buf.bufferAtOffset(7);
    EXPECT_EQ(res.first, 7);
    EXPECT_STREQ("bbbbbbb", res.second.c_str());
}

TEST(RingStreambuf, no_loss_when_iterating) {
    // This simulates calling getLogcat over and over.
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    int offset = 0;
    for (int i = 0; i < 26; i++) {
        std::string write(7, 'a' + i);
        stream << write;
        auto res = buf.bufferAtOffset(offset);

        EXPECT_EQ(offset, i * 7);
        EXPECT_EQ(res.first, i * 7);
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
        std::string write(6, 'a' + i);
        std::string read;
        stream << write << "\n";
        in >> read;
        EXPECT_STREQ(read.c_str(), write.c_str());
    }
}

TEST(RingStreambuf, can_also_read_from_buf) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    std::istream in(&buf);
    int offset = 0;
    for (int i = 0; i < 26; i++) {
        std::string write(6, 'a' + i);
        write += "\n";  // EOL parsing..
        buf.sputn(write.c_str(), write.size());

        in.clear();
        EXPECT_GT(buf.in_avail(), 0);
        EXPECT_TRUE(in.good());
        EXPECT_FALSE(in.bad());
        EXPECT_FALSE(in.fail());

        std::string read;
        in >> read;
        read += "\n";
        ASSERT_STREQ(read.c_str(), write.c_str());
    }
}

TEST(RingStreambuf, stream_overwrites_ring) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaaaaa";
    stream << "bbbbbbb";

    auto res = buf.bufferAtOffset(0, 1s);
    EXPECT_EQ(res.first, 7);
    EXPECT_STREQ("bbbbbbb", res.second.c_str());

    res = buf.bufferAtOffset(7);
    EXPECT_EQ(res.first, 7);
    EXPECT_STREQ("bbbbbbb", res.second.c_str());
}

TEST(RingStreambuf, stream_not_yet_available_no_block) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaaaaa";

    auto res = buf.bufferAtOffset(7);
    EXPECT_EQ(res.first, 7);
    EXPECT_STREQ("", res.second.c_str());
}
TEST(RingStreambuf, istream_times_out_when_empty) {
    // String buffer that will block at most 10ms.
    RingStreambuf buf(4, 5ms);
    std::istream in(&buf);
    std::string read;
    auto start = std::chrono::high_resolution_clock::now();

    in >> read;

    auto finish = std::chrono::high_resolution_clock::now();
    auto waited = std::chrono::duration_cast<milliseconds>(finish - start);
    // We should have waited at least 5ms.
    EXPECT_GE(waited, 5ms);
    SUCCEED();
}

TEST(RingStreambuf, stream_not_yet_available_no_block_gives_proper_distance) {
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaaaaa";

    auto res = buf.bufferAtOffset(200);
    EXPECT_EQ(res.first, 7);
    EXPECT_STREQ("", res.second.c_str());
}

TEST(RingStreambuf, stream_offset_blocks_until_available) {
    using namespace std::chrono_literals;
    RingStreambuf buf(4);
    std::ostream stream(&buf);
    stream << "aaaaaaa";
    std::thread writer([&stream] {
        std::this_thread::sleep_for(100ms);
        stream << "bbbbbbb";
        std::this_thread::sleep_for(100ms);
        stream << "ccccccc";
    });
    std::thread reader([&buf] {
        auto res = buf.bufferAtOffset(14, 1s);
        EXPECT_EQ(res.first, 14);
        EXPECT_STREQ("ccccccc", res.second.c_str());
    });

    writer.join();
    reader.join();
}

}  // namespace streams
}  // namespace base
}  // namespace android

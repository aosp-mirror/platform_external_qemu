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

#include "android/emulation/control/waterfall/SocketStreambuf.h"
#include <gtest/gtest.h>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>
#include <thread>
#include "android/base/sockets/SocketUtils.h"

#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"

namespace android {
namespace emulation {
namespace control {

using namespace base;

TEST(SocketStreambuf, identity) {
    int in, out;
    std::string read;
    EXPECT_FALSE(base::socketCreatePair(&in, &out));
    SocketStreambuf write_buf(in);
    SocketStreambuf read_buf(out);

    std::ostream ostream(&write_buf);
    std::istream istream(&read_buf);
    ostream << "hello" << std::endl;
    ostream << "world" << std::endl;
    istream >> read;
    EXPECT_STREQ("hello", read.c_str());

    istream >> read;
    EXPECT_STREQ("world", read.c_str());
    socketClose(in);
    socketClose(out);
}

TEST(SocketStreambuf, closed_reader) {
    int in, out;
    std::string read;
    EXPECT_FALSE(base::socketCreatePair(&in, &out));
    SocketStreambuf write_buf(in);
    SocketStreambuf read_buf(out);

    std::ostream ostream(&write_buf);
    std::istream istream(&read_buf);
    ostream << "hello" << std::endl;
    socketClose(out);
    istream >> read;
    EXPECT_STREQ("", read.c_str());
    EXPECT_TRUE(istream.eof());
}

TEST(SocketStreambuf, closed_writer) {
    int in, out;
    std::string read;
    EXPECT_FALSE(base::socketCreatePair(&in, &out));
    SocketStreambuf write_buf(in);
    SocketStreambuf read_buf(out);

    std::ostream ostream(&write_buf);
    std::istream istream(&read_buf);
    EXPECT_TRUE(ostream << "hello" << std::endl);
    socketClose(out);
    EXPECT_FALSE(ostream << "world" << std::endl);
}

TEST(SocketStreambuf, read_some) {
    int in, out;
    std::string read;
    char buf[6] = {0};

    EXPECT_FALSE(base::socketCreatePair(&in, &out));
    SocketStreambuf write_buf(in);
    SocketStreambuf read_buf(out);

    std::ostream ostream(&write_buf);
    std::istream istream(&read_buf);
    ostream << "hello";
    ostream << "world";
    EXPECT_EQ(5, istream.readsome(buf, 5));
    EXPECT_STREQ("hello", buf);
    EXPECT_EQ(5, istream.readsome(buf, 5));
    EXPECT_STREQ("world", buf);
    socketClose(in);
    socketClose(out);
}

static void forwardStream(std::istream& in,
                          std::ostream& out,
                          int chunksize = 256) {
    char chunk[chunksize];
    int n = 0;
    while (!in.eof() && !out.eof()) {
        int fst = in.get();  // Blocks until at least on byte available.
        if (fst != SocketStreambuf::traits_type::eof()) {
            chunk[0] = (char)fst;
            n = in.readsome(chunk + 1, sizeof(chunk) - 1) + 1;
            out.write(chunk, n);
        }
    }
    LOG(INFO) << "in: " << in.eof() << ", out: " << out.eof() << " n:" << n;
}

TEST(SocketStreambuf, read_chunks_as_you_go) {
    int in, out;
    std::string read;
    char buf[6] = {0};

    EXPECT_FALSE(base::socketCreatePair(&in, &out));
    SocketStreambuf write_buf(in);
    SocketStreambuf read_buf(out);
    std::stringbuf buffer;

    std::ostream sockostream(&write_buf);
    std::istream sockistream(&read_buf);
    std::ostream strostream(&buffer);
    std::stringstream sstream;

    std::thread forwarder([&sockistream, &strostream]() {
        forwardStream(sockistream, strostream);
    });
    for (int i = 0; i < 8; i++) {
        std::string msg;
        for (int j = 0; j < 1024; j++) {
            char rndChar = 'a' + rand() % 26;
            msg.append(&rndChar, 1);
        }
        sstream << msg << " - |";
        sockostream << msg << " - |";
    }

    System::get()->sleepMs(20);
    socketClose(in);
    socketClose(out);
    forwarder.join();
    EXPECT_GT(sstream.str().size(), 0);
    EXPECT_EQ(sstream.str().size(), buffer.str().size());

    std::string sstr = sstream.str();
    std::string bstr = buffer.str();
    EXPECT_STREQ(sstr.c_str(), bstr.c_str());
}

}  // namespace control
}  // namespace emulation
}  // namespace android

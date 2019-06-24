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

#include "android/emulation/control/waterfall/BstrStreambuf.h"
#include <gtest/gtest.h>
#include <iostream>
#include <istream>
#include <sstream>
#include <string>

#include "android/base/threads/FunctorThread.h"

namespace android {
namespace emulation {
namespace control {

TEST(BstrBuf, write_header) {
    std::stringbuf buffer;
    BstrStreambuf bstr_buf(&buffer);

    std::ostream stream(&bstr_buf);
    stream << "hi";
    EXPECT_EQ(6, buffer.str().size());
}

TEST(BstrBuf, write_header_with_eof) {
    std::stringbuf buffer;
    BstrStreambuf bstr_buf(&buffer);
    bstr_buf.close();
    EXPECT_EQ(4, buffer.str().size());
}

TEST(BstrBuf, identity) {
    std::string read;
    std::stringbuf buffer;
    BstrStreambuf bstr_buf(&buffer);

    std::ostream ostream(&bstr_buf);
    std::istream istream(&bstr_buf);
    ostream << "hello" << std::endl;
    ostream << "world" << std::endl;
    istream >> read;
    EXPECT_STREQ("hello", read.c_str());
    istream >> read;
    EXPECT_STREQ("world", read.c_str());
}

TEST(BstrBuf, detects_eof) {
    std::string read;
    std::string eof("\00\00\00\00");
    std::stringbuf buffer(eof);
    BstrStreambuf bstr_buf(&buffer);

    std::istream istream(&bstr_buf);
    istream >> read;
    EXPECT_STREQ("", read.c_str());
    EXPECT_TRUE(istream.eof());
}

TEST(BstrBuf, read_decodes_header) {
    uint32_t cnt = 3;
    std::string hi = std::string((char*)&cnt, 4) + std::string("hi\n");
    EXPECT_EQ(7, hi.size());
    std::stringbuf buffer(hi);
    BstrStreambuf bstr_buf(&buffer);
    std::istream stream(&bstr_buf);

    std::string read;
    stream >> read;
    EXPECT_EQ(2, read.size());
    EXPECT_STREQ("hi", read.c_str());
}

}  // namespace control
}  // namespace emulation
}  // namespace android

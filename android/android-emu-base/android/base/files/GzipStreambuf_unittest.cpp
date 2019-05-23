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

#include "android/base/files/GzipStreambuf.h"

#include <gtest/gtest.h>  // for Test, SuiteApiResolver, TestInfo (ptr only)
#include <cstdlib>        // for srand, rand
#include <ctime>          // for time
#include <istream>        // for stringstream, operator>>, basic_istream
#include <ostream>        // for operator<<, char_traits, basic_ostream, endl
#include <string>         // for basic_string, string
#include <vector>         // for vector

namespace android {
namespace base {

TEST(Gzip, output_writes_to_stream) {
    std::stringstream ss;
    GzipOutputStream gos(ss);
    gos << "Hello World";
    gos.flush();
    EXPECT_TRUE(ss.str().size() > 0);
}

TEST(Gzip, hello) {
    std::stringstream ss;
    std::string msg;
    GzipOutputStream gos(ss);
    gos << "Hello" << std::endl;
    gos.flush();

    GzipInputStream gis(ss);
    gis >> msg;

    EXPECT_EQ(msg, "Hello");
}

TEST(Gzip, identity_random) {
    std::srand(std::time(nullptr));

    // Let's write about 128kb..
    int write_bytes = 1024 * 128;
    std::vector<char> data(write_bytes);
    std::vector<char> decoded(write_bytes);

    // Create a checker board of 16 blocks of 16 chars..
    // This should result in some compression..
    const int BLOCK_SIZE = 256;
    for (int i = 0; i < write_bytes; i += BLOCK_SIZE) {
        char c = (char)rand() % 256;
        for (int j = 0; j < BLOCK_SIZE; j++)
            data[i + j] = c;
    }
    std::stringstream ss;
    GzipOutputStream gos(ss);
    gos.write(data.data(), data.size());
    gos.flush();

    GzipInputStream gis(ss);
    gis.read(decoded.data(), decoded.size());
    auto rd = gis.gcount();

    EXPECT_EQ(rd, write_bytes);

    // We expect some deflation from our gzip stream..
    EXPECT_LE(ss.str().size(), write_bytes);
    EXPECT_EQ(data, decoded);
}

TEST(Gzip, handles_errors) {
    std::stringstream ss;
    std::string msg;
    ss << "Hello" << std::endl;
    ss.flush();

    GzipInputStream gis(ss);
    gis >> msg;
    EXPECT_EQ(gis.gcount(), 0);
    EXPECT_NE(msg, "Hello");
}

}  // namespace base
}  // namespace android
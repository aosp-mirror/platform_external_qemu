// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/files/InplaceStream.h"

#include <gtest/gtest.h>

#include <stdint.h>

namespace android {
namespace base {

TEST(InplaceStream, write) {
    {
        std::vector<char> testbuf(sizeof(int32_t) * 2);
        InplaceStream stream(&testbuf[0], testbuf.size());
        int32_t val = 1;
        EXPECT_EQ(sizeof(val), stream.write(&val, sizeof(val)));
        EXPECT_EQ(sizeof(val), stream.writtenSize());
    }
    {
        std::vector<char> testbuf(sizeof(int32_t));
        InplaceStream stream(&testbuf[0], testbuf.size());
        int32_t val = 1;
        EXPECT_EQ(sizeof(val), stream.write(&val, sizeof(val)));
        EXPECT_EQ(sizeof(val), stream.writtenSize());
    }
    {
        std::vector<char> testbuf(sizeof(int16_t));
        InplaceStream stream(&testbuf[0], testbuf.size());
        int32_t val = 1;
        EXPECT_EQ(sizeof(int16_t), stream.write(&val, sizeof(val)));
        EXPECT_EQ(sizeof(int16_t), stream.writtenSize());
    }
    {
        std::vector<char> testbuf(sizeof(int32_t) * 2);
        InplaceStream stream(&testbuf[0], testbuf.size());
        int32_t val = 1;
        EXPECT_EQ(sizeof(val), stream.write(&val, sizeof(val)));
        EXPECT_EQ(sizeof(val), stream.writtenSize());
        val = 2;
        EXPECT_EQ(sizeof(val), stream.write(&val, sizeof(val)));
        EXPECT_EQ(2 * sizeof(val), stream.writtenSize());
    }
}

TEST(InplaceStream, read) {
    {
        int32_t val = 1;
        std::vector<int32_t> testbuf = { val };
        InplaceStream stream((char*)&testbuf[0], testbuf.size() * sizeof(int32_t));
        val = 0;
        EXPECT_EQ(sizeof(val), stream.read(&val, sizeof(val)));
        EXPECT_EQ(1, val);
        EXPECT_EQ(0, stream.readSize());
    }
    {
        std::vector<int32_t> testbuf = { 1, 2, 3, 4, 5 };
        InplaceStream stream((char*)&testbuf[0], testbuf.size() * sizeof(int32_t));
        for (int i = 0; i < 10; i++) {
            int32_t val = 0;
            if (i < 5) {
                EXPECT_EQ(sizeof(val), stream.read(&val, sizeof(val)));
                EXPECT_EQ(i + 1, val);
                EXPECT_EQ((5 - i - 1) * sizeof(int32_t), stream.readSize());
            } else {
                EXPECT_EQ(0, stream.read(&val, sizeof(val)));
                EXPECT_EQ(0, val);
                EXPECT_EQ(0, stream.readSize());
            }
        }
    }
}

TEST(InplaceStream, sizes) {
    int bufsize = sizeof(int64_t) + sizeof(int16_t);
    std::vector<char> testbuf(bufsize);
    InplaceStream stream(&testbuf[0], testbuf.size());

    EXPECT_EQ(0, stream.writtenSize());
    EXPECT_EQ(bufsize, stream.readSize());

    stream.putBe64(1);
    EXPECT_EQ(sizeof(int64_t), stream.writtenSize());
    EXPECT_EQ(bufsize, stream.readSize());

    stream.getBe16();
    EXPECT_EQ(sizeof(int64_t), stream.writtenSize());
    EXPECT_EQ(bufsize - sizeof(int16_t), stream.readSize());
    EXPECT_EQ(sizeof(int16_t), stream.readPos());

    stream.putBe16(2);
    EXPECT_EQ(sizeof(int64_t) + sizeof(int16_t), stream.writtenSize());
    EXPECT_EQ(bufsize - sizeof(int16_t), stream.readSize());
    EXPECT_EQ(sizeof(int16_t), stream.readPos());

    stream.getBe64();
    EXPECT_EQ(sizeof(int64_t) + sizeof(int16_t), stream.writtenSize());
    EXPECT_EQ(0, stream.readSize());
    EXPECT_EQ(bufsize, stream.readPos());
}

TEST(InplaceStream, saveLoad) {
    int bufsize = sizeof(int) + sizeof(float) + strlen("333 as string") + 4;
    std::vector<char> testbuf(bufsize);

    InplaceStream stream(&testbuf[0], bufsize);
    const int val = 1;
    const float fval = 2.;
    const StringView sval = "333 as string";
    stream.putBe32(val);
    stream.putFloat(fval);
    stream.putString(sval);

    std::vector<char> savebuf(bufsize + 4);
    InplaceStream saveStream(&savebuf[0], bufsize + 4);
    stream.save(&saveStream);

    std::vector<char> stream2buf(bufsize + 4);
    InplaceStream stream2(&stream2buf[0], bufsize + 4);
    stream2.load(&saveStream);
    EXPECT_EQ(val, stream2.getBe32());
    EXPECT_EQ(fval, stream2.getFloat());
    EXPECT_STREQ(c_str(sval), stream2.getString().c_str());
}

TEST(InplaceStream, getPosAndAdvance) {
    std::vector<int32_t> testbuf = { 1, 2, 3, 4, 5 };
    InplaceStream stream((char*)&testbuf[0], testbuf.size() * sizeof(int32_t));
    EXPECT_EQ((char*)&testbuf[0], stream.currentRead());
    stream.advanceRead(1);
    EXPECT_EQ(((char*)&testbuf[0]) + 1, stream.currentRead());
    stream.advanceRead(1);
    EXPECT_EQ(((char*)&testbuf[0]) + 2, stream.currentRead());
    stream.advanceRead(1);
    EXPECT_EQ(((char*)&testbuf[0]) + 3, stream.currentRead());
    stream.advanceRead(1);
    EXPECT_EQ(((char*)&testbuf[0]) + 4, stream.currentRead());

    int32_t readint = 0;
    stream.read(&readint, sizeof(int32_t));
    EXPECT_EQ(readint, 2);
    EXPECT_EQ(readint, *(((char*)&testbuf[0]) + 4));
}

TEST(InplaceStream, getNullTerminatedString) {
    std::string test("1234");

    std::vector<char> buf(10);
    InplaceStream stream(&buf[0], 10);

    stream.putStringNullTerminated(&test[0]);
    const char* fromStream =
        stream.getStringNullTerminated();

    EXPECT_EQ(0, strncmp(fromStream, test.c_str(), 5));
}

TEST(InplaceStream, getNullTerminatedStringEmpty) {
    std::string test("");

    std::vector<char> buf(10);
    InplaceStream stream(&buf[0], 10);

    stream.putStringNullTerminated(&test[0]);
    const char* fromStream =
        stream.getStringNullTerminated();

    EXPECT_EQ(0, strncmp(fromStream, test.c_str(), 5));
}

TEST(InplaceStream, getNullTerminatedStringError) {
    std::vector<char> buf(10, 0);
    InplaceStream stream(&buf[0], 10);
    // oops; we didn't actually put a string there,
    // but we did write uint32_t zero. in this case,
    // we can get a nullptr from getStringNullTerminated
    // Otherwise, all bets are off
    const char* fromStream =
        stream.getStringNullTerminated();
    EXPECT_EQ(nullptr, fromStream);
}

}  // namespace base
}  // namespace android

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

#include "android/emulation/serial_line.h"
#include "android/emulation/SerialLine.h"

#include <gtest/gtest.h>

using android::SerialLine;

namespace ut {
class TestSerialLine : public SerialLine {
public:
    virtual void addHandlers(void* opaque, CanReadFunc canReadFunc, ReadFunc readFunc) {
        ++AddHandlersCallCount;

        EXPECT_EQ(ExpectedOpaque, opaque);
        EXPECT_EQ(ExpectedCanReadFunc, canReadFunc);
        EXPECT_EQ(ExpectedReadFunc, readFunc);
    }

    virtual int write(const uint8_t* data, int len) {
        ++WriteCallCount;

        EXPECT_EQ(ExpectedData, data);
        EXPECT_EQ(ExpectedLen, len);

        return WriteReturn;
    }

    int AddHandlersCallCount = 0;
    int WriteCallCount = 0;

    void* ExpectedOpaque = nullptr;
    CanReadFunc ExpectedCanReadFunc = nullptr;
    ReadFunc ExpectedReadFunc = nullptr;

    const uint8_t* ExpectedData = nullptr;
    int ExpectedLen = 0;
    int WriteReturn = 0;
};

}  // namespace ut

TEST(SerialLine, write) {
    ut::TestSerialLine sl;
    sl.ExpectedLen = 1;
    uint8_t data;
    sl.ExpectedData = &data;
    sl.WriteReturn = 2;

    const int res = android_serialline_write(&sl, &data, 1);

    EXPECT_EQ(2, res);

    EXPECT_EQ(1, sl.WriteCallCount);
    EXPECT_EQ(0, sl.AddHandlersCallCount);
}

// some test callbacks
static int testCanRead(void*) {
    return 0;
}
static void testRead(void*, const uint8_t*, int) {
}

TEST(SerialLine, addHandlers) {
    ut::TestSerialLine sl;
    int dummy;
    sl.ExpectedOpaque = &dummy;
    sl.ExpectedCanReadFunc = &testCanRead;
    sl.ExpectedReadFunc = &testRead;

    android_serialline_addhandlers(&sl, &dummy, testCanRead, testRead);

    EXPECT_EQ(1, sl.AddHandlersCallCount);
    EXPECT_EQ(0, sl.WriteCallCount);
}

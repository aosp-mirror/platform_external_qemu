// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AdbMessageSniffer.h"

#include <gtest/gtest.h>
#include <memory>
#include <sstream>

namespace android {
namespace emulation {
using namespace base;

enum class AdbWireMessage {
    A_SYNC = 0x434e5953,
    A_CNXN = 0x4e584e43,
    A_AUTH = 0x48545541,
    A_OPEN = 0x4e45504f,
    A_OKAY = 0x59414b4f,
    A_CLSE = 0x45534c43,
    A_WRTE = 0x45545257,
};

#define MAX_ADB_MESSAGE_PAYLOAD 1048576



struct amessage {
    uint32_t command;     /* command identifier constant      */
    uint32_t arg0;        /* first argument                   */
    uint32_t arg1;        /* second argument                  */
    uint32_t data_length; /* length of payload (0 is allowed) */
    uint32_t data_check;  /* checksum of data payload         */
    uint32_t magic;       /* command ^ 0xffffffff             */
} __attribute__((packed));

struct apacket {
    amessage msg;
    std::vector<char> payload;
};

using Message = std::string;

Message addMessage(AdbWireMessage cmd,
                   uint32_t arg0,
                   uint32_t arg1,
                   std::string msg = "") {
    amessage message = {(uint32_t)cmd,        arg0, arg1,
                        (uint32_t)msg.size(), 0,    (uint32_t)cmd ^ 0xffffffff};
    auto wire = std::string((char*)&message, sizeof(message));
    if (!msg.empty())
        wire += msg;
    return wire;
}

Message connectMsg() {
    return addMessage(AdbWireMessage::A_CNXN, 0x01000001, 1024 * 255,
                      "features=shell_v2,foo,bar");
}

Message smallMsg() {
    return addMessage(AdbWireMessage::A_WRTE, 0, 0, "x");
}



Message emptyMsg() {
    return addMessage(AdbWireMessage::A_WRTE, 0, 0, "");
}

using namespace ::android::base::testing;


class TestAdbMessageSniffer : public AdbMessageSniffer {
    public:
        TestAdbMessageSniffer(std::ostream* stream) : AdbMessageSniffer("Test", 2, stream) {};
        void setLogLevel(int level) { setLevel(level); }
};

class AdbMessageSnifferTest : public ::testing::Test {
public:
    AdbMessageSnifferTest() : mSniffer(&mOutput) {}

    void sendMessage(Message msg) {
        AndroidPipeBuffer buffer;
        buffer.size = msg.size();
        buffer.data = (uint8_t*)msg.data();

        mSniffer.read(&buffer, 1, msg.size());
    }

    std::string messageStringOfAtLeast(int bytes) {
        std::string msg = connectMsg();
        msg.reserve(bytes + msg.size());
        while(msg.size() < bytes) {
            msg.append(connectMsg());
            msg.append(smallMsg());
            msg.append(emptyMsg());
        }

        return msg;
    }

protected:
    std::stringstream mOutput;
    TestAdbMessageSniffer mSniffer;
};

TEST_F(AdbMessageSnifferTest, nolog_on_level_0) {
    // Nothing should happen.
    mSniffer.setLogLevel(0);

    sendMessage(connectMsg());
    sendMessage(smallMsg());

    EXPECT_EQ(mOutput.str().size(), 0);
}

TEST_F(AdbMessageSnifferTest, log_on_level_2) {
    // Logs a bunch of stuff.
    sendMessage(connectMsg());
    sendMessage(smallMsg());

    // We should have logged things!
    EXPECT_NE(mOutput.str().size(), 0);
}

TEST_F(AdbMessageSnifferTest, single_byte_no_crash) {
    // Should not crash!
    std::string msg = messageStringOfAtLeast(MAX_ADB_MESSAGE_PAYLOAD * 2);
    AndroidPipeBuffer buffer;
    buffer.size = 1;
    buffer.data = (uint8_t*)msg.data();
    for (int i = 0; i < msg.size(); i++) {
        mSniffer.read(&buffer, 1, 1);
        buffer.data++;
    }

    EXPECT_NE(mOutput.str().size(), 0);
}

TEST_F(AdbMessageSnifferTest, many_bytes_no_crash) {
    std::string msg = messageStringOfAtLeast(MAX_ADB_MESSAGE_PAYLOAD * 2);
    AndroidPipeBuffer buffer;


    // Deliver in random sized chunks..
    while (msg.size() > 0) {
        int div = (msg.size() / 2) + 1;
        int toSend = std::min<int>(rand() % div + 1, msg.size());
        buffer.size = toSend;
        buffer.data = (uint8_t*)msg.data();
        mSniffer.read(&buffer, 1, buffer.size);
        msg.erase(0, toSend);
    }

    EXPECT_NE(mOutput.str().size(), 0);
}


TEST_F(AdbMessageSnifferTest, many_bytes_partial_receives) {
    std::string msg = messageStringOfAtLeast(MAX_ADB_MESSAGE_PAYLOAD * 2);
    AndroidPipeBuffer buffer;


    // Deliver in random sized chunks..
    while (msg.size() > 0) {
        int div = (msg.size() / 2) + 1;
        int toSend = std::min<int>(rand() % div + 1, msg.size());
        int toRec = std::min<int>(rand() % toSend + 1, toSend);
        buffer.size = toSend;
        buffer.data = (uint8_t*)msg.data();
        mSniffer.read(&buffer, 1, toRec);
        msg.erase(0, toSend);
    }

    EXPECT_NE(mOutput.str().size(), 0);
}

TEST_F(AdbMessageSnifferTest, single_byte_partial_recieves) {
    // Should not crash!
    std::string msg = messageStringOfAtLeast(MAX_ADB_MESSAGE_PAYLOAD * 2);
    AndroidPipeBuffer buffer;
    buffer.size = 1;
    buffer.data = (uint8_t*)msg.data();
    for (int i = 0; i < msg.size(); i++) {
        mSniffer.read(&buffer, 1, i % 2);
        buffer.data++;
    }

    EXPECT_NE(mOutput.str().size(), 0);
}

}  // namespace emulation
}  // namespace android
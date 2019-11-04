// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AdbHub.h"

#include "android/emulation/apacket_utils.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"


#include <gtest/gtest.h>

namespace android {
namespace emulation {

namespace {
    const int kGuestId = 1;
    const int kHostId = 2;
    const int kHostIdAfterLoad = 3;
    const int kHeaderSize = sizeof(amessage);
    apacket buildPacket(int command) {
        apacket ret;
        ret.mesg.command = command;
        ret.mesg.magic = ret.mesg.command ^ 0xffffffff;
        ret.mesg.data_check = 0;
        ret.mesg.data_length = 0;
        return ret;
    }
    apacket buildPacketHostToGuest(int command) {
        apacket ret = buildPacket(command);
        ret.mesg.arg0 = kHostId;
        ret.mesg.arg1 = kGuestId;
        return ret;
    }
    apacket buildPacketGuestToHost(int command) {
        apacket ret = buildPacket(command);
        ret.mesg.arg0 = kGuestId;
        ret.mesg.arg1 = kHostId;
        return ret;
    }
    bool comparePacket(const apacket& a, const apacket& b) {
        return memcmp(&a.mesg, &b.mesg, kHeaderSize) == 0
            && memcmp(a.data.data(), b.data.data(), a.mesg.data_length) == 0;
    }
    AndroidPipeBuffer packetToBuffer(const apacket& packet) {
        AndroidPipeBuffer buffer;
        buffer.size = packetSize(packet);
        buffer.data = new uint8_t[buffer.size];
        memcpy(buffer.data, &packet.mesg, kHeaderSize);
        memcpy(buffer.data + kHeaderSize, packet.data.data(), packet.data.size());
        return buffer;
    }
    void releaseBuffer(AndroidPipeBuffer& buffer) {
        delete [] buffer.data;
    }
    class AdbHubTest : public ::testing::Test {
        protected:
            void SetUp() override {
                int inSocket;
                int outSocket;
                android::base::socketCreatePair(&inSocket, &outSocket);
                android::base::socketSetBlocking(inSocket);
                android::base::socketSetBlocking(outSocket);
                mInSocket.reset(inSocket);
                mOutSocket.reset(outSocket);
            }
            base::ScopedSocket mInSocket;
            base::ScopedSocket mOutSocket;
    };
}

TEST_F(AdbHubTest, SendPacket) {
    AdbHub hub;
    apacket packet = buildPacketHostToGuest(ADB_OKAY);
    AndroidPipeBuffer buffer = packetToBuffer(packet);
    EXPECT_FALSE(hub.socketWantWrite());
    hub.onGuestSendData(&buffer, 1);
    EXPECT_TRUE(hub.socketWantWrite());
    hub.onHostSocketEvent(mOutSocket.get(), base::Looper::FdWatch::kEventWrite,
        [] { FAIL(); });
    apacket receivedPacket;
    EXPECT_TRUE(recvPacket(mInSocket.get(), &receivedPacket));
    EXPECT_TRUE(comparePacket(packet, receivedPacket));
}

}
}

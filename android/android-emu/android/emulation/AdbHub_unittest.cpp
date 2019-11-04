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
    apacket bufferToPacket(const AndroidPipeBuffer buffer) {
        apacket packet;
        memcpy(&packet.mesg, buffer.data, kHeaderSize);
        packet.data.resize(packet.mesg.data_length);
        memcpy(packet.data.data(), buffer.data + kHeaderSize, packet.mesg.data_length);
        return packet;
    }
    void releaseBuffer(AndroidPipeBuffer& buffer) {
        delete [] buffer.data;
    }
    class AdbHubTest : public ::testing::Test {
        protected:
            void SetUp() override {
                int hubSocket;
                int testerSocket;
                android::base::socketCreatePair(&hubSocket, &testerSocket);
                android::base::socketSetNonBlocking(hubSocket);
                android::base::socketSetBlocking(testerSocket);
                mHubSocket.reset(hubSocket);
                mTesterSocket.reset(testerSocket);
                mHub.setWakePipeFunc([this](int wakeFlag) {
                    mPipeWakeFlag |= wakeFlag;
                });
            }
            base::ScopedSocket mHubSocket;
            base::ScopedSocket mTesterSocket;
            AdbHub mHub;
            int mPipeWakeFlag = 0;
    };
}

TEST_F(AdbHubTest, SendPacket) {
    apacket packet = buildPacketHostToGuest(ADB_OKAY);
    AndroidPipeBuffer buffer = packetToBuffer(packet);
    EXPECT_FALSE(mHub.socketWantWrite());
    mHub.onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventWrite,
        [] { FAIL(); });
    mHub.onGuestSendData(&buffer, 1);
    EXPECT_TRUE(mHub.socketWantWrite());
    mHub.onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventWrite,
        [] { FAIL(); });
    apacket receivedPacket;
    EXPECT_TRUE(recvPacket(mTesterSocket.get(), &receivedPacket));
    EXPECT_TRUE(comparePacket(packet, receivedPacket));
    releaseBuffer(buffer);
}

TEST_F(AdbHubTest, SendPacketSeparateHeader) {
    EXPECT_FALSE(mHub.socketWantWrite());

    apacket packet = buildPacketHostToGuest(ADB_WRTE);
    packet.data.resize(1);
    packet.data[0] = 'x';
    packet.mesg.data_length = packet.data.size();

    AndroidPipeBuffer buffer;
    buffer.size = kHeaderSize;
    buffer.data = (uint8_t*)&packet.mesg;
    mHub.onGuestSendData(&buffer, 1);
    EXPECT_FALSE(mHub.socketWantWrite());

    buffer.size = packet.mesg.data_length;
    buffer.data = packet.data.data();
    mHub.onGuestSendData(&buffer, 1);
    EXPECT_TRUE(mHub.socketWantWrite());

    mHub.onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventWrite,
        [] { FAIL(); });
    apacket receivedPacket;
    EXPECT_TRUE(recvPacket(mTesterSocket.get(), &receivedPacket));
    EXPECT_TRUE(comparePacket(packet, receivedPacket));
}

TEST_F(AdbHubTest, SendMultipleBuffers) {
    apacket packet0 = buildPacketHostToGuest(ADB_OKAY);

    apacket packet1 = buildPacketHostToGuest(ADB_WRTE);
    packet1.data.resize(1);
    packet1.data[0] = 'x';
    packet1.mesg.data_length = packet1.data.size();

    AndroidPipeBuffer buffers[3];
    buffers[0] = packetToBuffer(packet0);
    buffers[1].size = kHeaderSize;
    buffers[1].data = (uint8_t*)&packet1.mesg;

    buffers[2].size = packet1.mesg.data_length;
    buffers[2].data = packet1.data.data();

    EXPECT_FALSE(mHub.socketWantWrite());
    mHub.onGuestSendData(buffers, 3);
    EXPECT_TRUE(mHub.socketWantWrite());
    mHub.onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventWrite,
        [] { FAIL(); });

    apacket receivedPacket;
    EXPECT_TRUE(recvPacket(mTesterSocket.get(), &receivedPacket));
    EXPECT_TRUE(comparePacket(packet0, receivedPacket));

    EXPECT_TRUE(recvPacket(mTesterSocket.get(), &receivedPacket));
    EXPECT_TRUE(comparePacket(packet1, receivedPacket));
    releaseBuffer(buffers[0]);
}

TEST_F(AdbHubTest, RecvPacket) {
    apacket packet = buildPacketHostToGuest(ADB_WRTE);
    packet.data.resize(1);
    packet.data[0] = 'x';
    packet.mesg.data_length = packet.data.size();

    AndroidPipeBuffer buffer;
    buffer.size = packetSize(packet);
    buffer.data = new uint8_t[buffer.size];

    EXPECT_EQ(mHub.onGuestRecvData(&buffer, 1), PIPE_ERROR_AGAIN);
    EXPECT_EQ(mPipeWakeFlag, 0);

    EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet));
    mHub.onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventRead,
        [] { FAIL(); });
    EXPECT_EQ(mPipeWakeFlag, PIPE_WAKE_READ);
    EXPECT_EQ(mHub.onGuestRecvData(&buffer, 1), packetSize(packet));

    apacket receivedPacket = bufferToPacket(buffer);
    EXPECT_TRUE(comparePacket(packet, receivedPacket));

    EXPECT_EQ(mHub.onGuestRecvData(&buffer, 1), PIPE_ERROR_AGAIN);

    releaseBuffer(buffer);
}

}
}

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

#include "android/base/files/MemStream.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/base/files/StdioStream.h"
#include "android/base/sockets/ScopedSocket.h"
#include "android/base/sockets/SocketUtils.h"
#include "android/base/testing/TestTempDir.h"
#include "android/emulation/apacket_utils.h"

#include <gtest/gtest.h>
#include <memory>

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
apacket buildPacketHostToGuest(int command, bool afterLoad = false) {
    apacket ret = buildPacket(command);
    ret.mesg.arg0 = afterLoad ? kHostIdAfterLoad : kHostId;
    ret.mesg.arg1 = kGuestId;
    return ret;
}
apacket buildPacketGuestToHost(int command, bool afterLoad = false) {
    apacket ret = buildPacket(command);
    ret.mesg.arg0 = kGuestId;
    ret.mesg.arg1 = afterLoad ? kHostIdAfterLoad : kHostId;
    return ret;
}
bool comparePacket(const apacket& a, const apacket& b) {
    return memcmp(&a.mesg, &b.mesg, kHeaderSize) == 0 &&
           memcmp(a.data.data(), b.data.data(), a.mesg.data_length) == 0;
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
    memcpy(packet.data.data(), buffer.data + kHeaderSize,
           packet.mesg.data_length);
    return packet;
}
void releaseBuffer(AndroidPipeBuffer& buffer) {
    delete[] buffer.data;
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
        resetHub();
        mTempDir.reset(new base::TestTempDir("adbhubtest"));
    }
    void TearDown() override {
        mHubSocket.reset(-1);
        mTesterSocket.reset(-1);
        mTempDir.reset();
    }
    void resetHub() {
        mHub.reset(new AdbHub());
    }
    void expectGuestRecvPacket(const apacket& expectedPacket) {
        AndroidPipeBuffer buffer;
        buffer.size = packetSize(expectedPacket);
        buffer.data = new uint8_t[buffer.size];
        EXPECT_EQ(buffer.size, mHub->onGuestRecvData(&buffer, 1));
        apacket recvedPacket = bufferToPacket(buffer);
        EXPECT_TRUE(comparePacket(expectedPacket, recvedPacket));
        releaseBuffer(buffer);
    }
    void expectGuestSendPacket(const apacket& expectedPacket) {
        apacket receivedPacket;
        EXPECT_TRUE(recvPacket(mTesterSocket.get(), &receivedPacket));
        EXPECT_TRUE(comparePacket(expectedPacket, receivedPacket));
    }
    void guestSendAndCheck(const apacket& packet) {
        AndroidPipeBuffer buffer = packetToBuffer(packet);
        mHub->onGuestSendData(&buffer, 1);
        EXPECT_TRUE(mHub->socketWantWrite());
        mHub->onHostSocketEvent(mHubSocket.get(),
                                base::Looper::FdWatch::kEventWrite,
                                [] { FAIL(); });
        expectGuestSendPacket(packet);
        releaseBuffer(buffer);
    }
    void hostSendAndCheck(const apacket& packet) {
        using FdWatch = base::Looper::FdWatch;
        EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet));
        mHub->onHostSocketEvent(mHubSocket.get(), FdWatch::kEventRead,
                                [] { FAIL(); });
        expectGuestRecvPacket(packet);
    }

    void adbConnectionAndHandshake(bool reconnect) {
        using FdWatch = base::Looper::FdWatch;
        const char* kOpenData = "jdwp:777";
        const char* kHandshake = "JDWP-Handshake";
        std::function<void(const apacket& packet)> guestSend;
        std::function<void(const apacket& packet)> hostSend;
        if (reconnect) {
            guestSend = [this](const apacket& packet) {
                mHub->onHostSocketEvent(mHubSocket.get(),
                                        base::Looper::FdWatch::kEventWrite,
                                        [] { FAIL(); });
                expectGuestSendPacket(packet);
            };
            hostSend = [this](const apacket& packet) {
                EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet));
                mHub->onHostSocketEvent(mHubSocket.get(), FdWatch::kEventRead,
                                        [] { FAIL(); });
                AndroidPipeBuffer buffer;
                buffer.size = 24;
                buffer.data = new uint8_t[buffer.size];
                EXPECT_EQ(PIPE_ERROR_AGAIN, mHub->onGuestRecvData(&buffer, 1));
                releaseBuffer(buffer);
            };
        } else {
            guestSend = [this](const apacket& packet) {
                guestSendAndCheck(packet);
            };
            hostSend = [this](const apacket& packet) {
                hostSendAndCheck(packet);
            };
        }

        apacket packet = buildPacketHostToGuest(ADB_OPEN, reconnect);
        packet.mesg.arg1 = 0;
        packet.mesg.data_length = strlen(kOpenData) + 1;
        packet.data.assign((const uint8_t*)kOpenData,
                           (const uint8_t*)kOpenData + packet.mesg.data_length);
        hostSend(packet);

        packet = buildPacketGuestToHost(ADB_OKAY, reconnect);
        guestSend(packet);
        ASSERT_EQ(mHub->getProxyCount(), 1);

        packet = buildPacketHostToGuest(ADB_WRTE, reconnect);
        packet.mesg.data_length = strlen(kHandshake);
        packet.data.assign(
                (const uint8_t*)kHandshake,
                (const uint8_t*)kHandshake + packet.mesg.data_length);
        hostSend(packet);
        ASSERT_EQ(mHub->getProxyCount(), 1);

        packet = buildPacketGuestToHost(ADB_OKAY, reconnect);
        guestSend(packet);
        ASSERT_EQ(mHub->getProxyCount(), 1);

        packet = buildPacketGuestToHost(ADB_WRTE, reconnect);
        packet.mesg.data_length = strlen(kHandshake);
        packet.data.assign(
                (const uint8_t*)kHandshake,
                (const uint8_t*)kHandshake + packet.mesg.data_length);
        guestSend(packet);
        ASSERT_EQ(mHub->getProxyCount(), 1);

        packet = buildPacketHostToGuest(ADB_OKAY, reconnect);
        if (reconnect) {
            // The last ADB_OKAY will be delivered to the guest
            using FdWatch = base::Looper::FdWatch;
            EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet));
            mHub->onHostSocketEvent(mHubSocket.get(), FdWatch::kEventRead,
                                    [] { FAIL(); });
            packet = buildPacketHostToGuest(ADB_OKAY, false);
            expectGuestRecvPacket(packet);
        } else {
            hostSendAndCheck(packet);
        }
        ASSERT_EQ(mHub->getProxyCount(), 1);
    }

    base::ScopedSocket mHubSocket;
    base::ScopedSocket mTesterSocket;
    std::unique_ptr<AdbHub> mHub;
    std::unique_ptr<base::TestTempDir> mTempDir;
};
}  // namespace

TEST_F(AdbHubTest, SendPacket) {
    apacket packet = buildPacketGuestToHost(ADB_OKAY);
    guestSendAndCheck(packet);
}

TEST_F(AdbHubTest, SendPacketSeparateHeader) {
    EXPECT_FALSE(mHub->socketWantWrite());

    apacket packet = buildPacketGuestToHost(ADB_WRTE);
    packet.data.resize(1);
    packet.data[0] = 'x';
    packet.mesg.data_length = packet.data.size();

    AndroidPipeBuffer buffer;
    buffer.size = kHeaderSize;
    buffer.data = (uint8_t*)&packet.mesg;
    mHub->onGuestSendData(&buffer, 1);
    EXPECT_FALSE(mHub->socketWantWrite());

    buffer.size = packet.mesg.data_length;
    buffer.data = packet.data.data();
    mHub->onGuestSendData(&buffer, 1);
    EXPECT_TRUE(mHub->socketWantWrite());

    mHub->onHostSocketEvent(mHubSocket.get(),
                            base::Looper::FdWatch::kEventWrite, [] { FAIL(); });
    expectGuestSendPacket(packet);
}

TEST_F(AdbHubTest, SendMultipleBuffers) {
    apacket packet0 = buildPacketGuestToHost(ADB_OKAY);

    apacket packet1 = buildPacketGuestToHost(ADB_WRTE);
    packet1.data.resize(1);
    packet1.data[0] = 'x';
    packet1.mesg.data_length = packet1.data.size();

    AndroidPipeBuffer buffers[3];
    buffers[0] = packetToBuffer(packet0);
    buffers[1].size = kHeaderSize;
    buffers[1].data = (uint8_t*)&packet1.mesg;

    buffers[2].size = packet1.mesg.data_length;
    buffers[2].data = packet1.data.data();

    EXPECT_FALSE(mHub->socketWantWrite());
    mHub->onGuestSendData(buffers, 3);
    EXPECT_TRUE(mHub->socketWantWrite());
    mHub->onHostSocketEvent(mHubSocket.get(),
                            base::Looper::FdWatch::kEventWrite, [] { FAIL(); });

    expectGuestSendPacket(packet0);
    expectGuestSendPacket(packet1);

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

    EXPECT_EQ(mHub->onGuestRecvData(&buffer, 1), PIPE_ERROR_AGAIN);
    EXPECT_FALSE(mHub->pipeWakeFlags() & PIPE_WAKE_READ);

    EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet));
    mHub->onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventRead,
                            [] { FAIL(); });
    EXPECT_TRUE(mHub->pipeWakeFlags() & PIPE_WAKE_READ);
    expectGuestRecvPacket(packet);

    EXPECT_EQ(mHub->onGuestRecvData(&buffer, 1), PIPE_ERROR_AGAIN);

    releaseBuffer(buffer);
}

TEST_F(AdbHubTest, RecvPacketSeparateHeader) {
    apacket packet = buildPacketHostToGuest(ADB_WRTE);
    packet.data.resize(1);
    packet.data[0] = 'x';
    packet.mesg.data_length = packet.data.size();

    apacket receivedPacket;
    AndroidPipeBuffer buffer;
    buffer.size = kHeaderSize;
    buffer.data = (uint8_t*)&receivedPacket.mesg;

    EXPECT_TRUE(base::socketSendAll(mTesterSocket.get(), &packet.mesg,
                                    kHeaderSize));
    mHub->onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventRead,
                            [] { FAIL(); });
    EXPECT_FALSE(mHub->pipeWakeFlags() & PIPE_WAKE_READ);
    EXPECT_EQ(mHub->onGuestRecvData(&buffer, 1), PIPE_ERROR_AGAIN);

    EXPECT_TRUE(base::socketSendAll(mTesterSocket.get(), packet.data.data(),
                                    packet.mesg.data_length));
    mHub->onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventRead,
                            [] { FAIL(); });
    EXPECT_TRUE(mHub->pipeWakeFlags() & PIPE_WAKE_READ);

    buffer.size = kHeaderSize;
    buffer.data = (uint8_t*)&receivedPacket.mesg;
    EXPECT_EQ(mHub->onGuestRecvData(&buffer, 1), kHeaderSize);

    receivedPacket.data.resize(receivedPacket.mesg.data_length);
    buffer.size = receivedPacket.mesg.data_length;
    buffer.data = receivedPacket.data.data();
    EXPECT_EQ(mHub->onGuestRecvData(&buffer, 1), buffer.size);

    EXPECT_TRUE(comparePacket(packet, receivedPacket));

    EXPECT_EQ(mHub->onGuestRecvData(&buffer, 1), PIPE_ERROR_AGAIN);
}

TEST_F(AdbHubTest, RecvPacketMultiplePacketSingleBuffer) {
    apacket packet0 = buildPacketHostToGuest(ADB_WRTE);
    packet0.data.resize(1);
    packet0.data[0] = 'x';
    packet0.mesg.data_length = packet0.data.size();

    apacket packet1 = buildPacketHostToGuest(ADB_WRTE);
    packet1.data.resize(2);
    packet1.data[0] = 'y';
    packet1.data[1] = 'z';
    packet1.mesg.data_length = packet1.data.size();

    EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet0));
    EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet1));

    AndroidPipeBuffer buffer;
    buffer.size = packetSize(packet0) + packetSize(packet1);
    buffer.data = new uint8_t[buffer.size];

    mHub->onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventRead,
                            [] { FAIL(); });
    EXPECT_TRUE(mHub->pipeWakeFlags() & PIPE_WAKE_READ);
    EXPECT_EQ(mHub->onGuestRecvData(&buffer, 1), buffer.size);

    uint8_t* data = buffer.data;
    EXPECT_EQ(0, memcmp(data, &packet0.mesg, kHeaderSize));
    data += kHeaderSize;
    EXPECT_EQ(0, memcmp(data, packet0.data.data(), packet0.mesg.data_length));
    data += packet0.mesg.data_length;
    EXPECT_EQ(0, memcmp(data, &packet1.mesg, kHeaderSize));
    data += kHeaderSize;
    EXPECT_EQ(0, memcmp(data, packet1.data.data(), packet1.mesg.data_length));
    data += packet1.mesg.data_length;

    releaseBuffer(buffer);
}

TEST_F(AdbHubTest, RecvPacketMultiplePacketMultipleBuffers) {
    apacket packet0 = buildPacketHostToGuest(ADB_WRTE);
    packet0.data.resize(1);
    packet0.data[0] = 'x';
    packet0.mesg.data_length = packet0.data.size();

    apacket packet1 = buildPacketHostToGuest(ADB_WRTE);
    packet1.data.resize(2);
    packet1.data[0] = 'y';
    packet1.data[1] = 'z';
    packet1.mesg.data_length = packet1.data.size();

    EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet0));
    EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet1));

    AndroidPipeBuffer buffers[4];
    buffers[0].size = kHeaderSize;
    buffers[0].data = new uint8_t[kHeaderSize];
    buffers[1].size = packet0.mesg.data_length;
    buffers[1].data = new uint8_t[buffers[1].size];
    buffers[2].size = kHeaderSize;
    buffers[2].data = new uint8_t[kHeaderSize];
    buffers[3].size = packet1.mesg.data_length;
    buffers[3].data = new uint8_t[buffers[3].size];

    mHub->onHostSocketEvent(mHubSocket.get(), base::Looper::FdWatch::kEventRead,
                            [] { FAIL(); });
    EXPECT_TRUE(mHub->pipeWakeFlags() & PIPE_WAKE_READ);
    EXPECT_EQ(mHub->onGuestRecvData(buffers, 4),
              packetSize(packet0) + packetSize(packet1));

    EXPECT_EQ(0, memcmp(buffers[0].data, &packet0.mesg, kHeaderSize));
    EXPECT_EQ(0, memcmp(buffers[1].data, packet0.data.data(),
                        packet0.mesg.data_length));
    EXPECT_EQ(0, memcmp(buffers[2].data, &packet1.mesg, kHeaderSize));
    EXPECT_EQ(0, memcmp(buffers[3].data, packet1.data.data(),
                        packet1.mesg.data_length));

    releaseBuffer(buffers[0]);
    releaseBuffer(buffers[1]);
    releaseBuffer(buffers[2]);
    releaseBuffer(buffers[3]);
}

TEST_F(AdbHubTest, AdbReconnect) {
    using FdWatch = base::Looper::FdWatch;
    const char* kConnectMesg = "connect";
    const char* kConnectReply = "reply";

    apacket packet = buildPacketHostToGuest(ADB_CNXN);
    packet.mesg.data_length = strlen(kConnectMesg) + 1;
    packet.data.assign((const uint8_t*)kConnectMesg,
                       (const uint8_t*)kConnectMesg + packet.mesg.data_length);
    hostSendAndCheck(packet);

    packet = buildPacketGuestToHost(ADB_CNXN);
    packet.mesg.data_length = strlen(kConnectReply) + 1;
    packet.data.assign((const uint8_t*)kConnectReply,
                       (const uint8_t*)kConnectReply + packet.mesg.data_length);
    guestSendAndCheck(packet);

    android::base::MemStream snapshot;
    mHub->onSave(&snapshot);
    resetHub();
    mHub->onLoad(&snapshot);

    packet = buildPacketHostToGuest(ADB_CNXN);
    packet.mesg.data_length = strlen(kConnectMesg) + 1;
    packet.data.assign((const uint8_t*)kConnectMesg,
                       (const uint8_t*)kConnectMesg + packet.mesg.data_length);
    EXPECT_TRUE(sendPacket(mTesterSocket.get(), &packet));
    mHub->onHostSocketEvent(mHubSocket.get(), FdWatch::kEventRead,
                            [] { FAIL(); });

    packet = buildPacketGuestToHost(ADB_CNXN);
    packet.mesg.data_length = strlen(kConnectReply) + 1;
    packet.data.assign((const uint8_t*)kConnectReply,
                       (const uint8_t*)kConnectReply + packet.mesg.data_length);
    EXPECT_TRUE(mHub->socketWantWrite());
    mHub->onHostSocketEvent(mHubSocket.get(),
                            base::Looper::FdWatch::kEventWrite, [] { FAIL(); });
    expectGuestSendPacket(packet);
}

TEST_F(AdbHubTest, JdwpBadHandshake) {
    std::string snapshotPath = mTempDir->makeSubPath("snapshot.bin");
    const char* kOpenData = "jdwp:777";
    const char* kHandshake = "JDWP-Handshake";
    const char* kBad = "jdwp-Handshake";

    apacket packet = buildPacketHostToGuest(ADB_OPEN);
    packet.mesg.arg1 = 0;
    packet.mesg.data_length = strlen(kOpenData) + 1;
    packet.data.assign((const uint8_t*)kOpenData,
                       (const uint8_t*)kOpenData + packet.mesg.data_length);
    hostSendAndCheck(packet);

    packet = buildPacketGuestToHost(ADB_OKAY);
    guestSendAndCheck(packet);
    ASSERT_EQ(mHub->getProxyCount(), 1);

    packet = buildPacketHostToGuest(ADB_WRTE);
    packet.mesg.data_length = strlen(kHandshake);
    packet.data.assign((const uint8_t*)kHandshake,
                       (const uint8_t*)kHandshake + packet.mesg.data_length);
    hostSendAndCheck(packet);
    ASSERT_EQ(mHub->getProxyCount(), 1);

    packet = buildPacketGuestToHost(ADB_OKAY);
    guestSendAndCheck(packet);
    ASSERT_EQ(mHub->getProxyCount(), 1);

    packet = buildPacketGuestToHost(ADB_WRTE);
    packet.mesg.data_length = strlen(kBad);
    packet.data.assign((const uint8_t*)kBad,
                       (const uint8_t*)kBad + packet.mesg.data_length);
    guestSendAndCheck(packet);
    ASSERT_EQ(mHub->getProxyCount(), 0);
}

TEST_F(AdbHubTest, JdwpReconnect) {
    adbConnectionAndHandshake(false);

    // Snapshot save and load
    android::base::MemStream snapshot;
    mHub->onSave(&snapshot);
    resetHub();
    mHub->onLoad(&snapshot);

    adbConnectionAndHandshake(true);
}

}  // namespace emulation
}  // namespace android

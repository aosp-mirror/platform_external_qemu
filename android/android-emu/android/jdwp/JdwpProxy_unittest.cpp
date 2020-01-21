// Copyright 2020 The Android Open Source Project
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

#include "android/jdwp/JdwpProxy.h"

#include <gtest/gtest.h>
#include <cstring>
#include <memory>

#include "android/base/files/MemStream.h"
#include "android/emulation/apacket_utils.h"
#include "android/jdwp/Jdwp.h"

using namespace android::emulation;

namespace android {
namespace jdwp {

namespace {

const int kGuestId = 1;
const int kHostId = 2;
const int kHostIdAfterLoad = 3;
const int kGuestPid = 4;
const char* kJdwpOpenStr = "jdwp:4";

class JdwpProxyTest : public ::testing::Test {
protected:
    void SetUp() override {
        mProxy.reset(new JdwpProxy(kHostId, kGuestId, kGuestPid));
    }
    void TearDown() override {}
    void saveAndLoadSnapshot() {
        android::base::MemStream snapshot;
        mProxy->onSave(&snapshot);
        mProxy.reset(new JdwpProxy(&snapshot));
        mProxy->setCurrentHostId(kHostIdAfterLoad);
    }
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
    void validateGuestToHost(const amessage& message, bool reconnect) {
        ASSERT_EQ(message.arg0, kGuestId);
        if (reconnect) {
            ASSERT_EQ(message.arg1, kHostIdAfterLoad);
        } else {
            ASSERT_EQ(message.arg1, kHostId);
        }
    }

    void jdwpConnectionAndHandshake(bool reconnect) {
        auto guestSend = [this, reconnect](const apacket& packet) {
            if (reconnect) {
                return;
            }
            bool shouldForward;
            std::queue<emulation::apacket> extraSends;
            mProxy->onGuestSendData(&packet.mesg, packet.data.data(),
                                    &shouldForward, &extraSends);
            ASSERT_NE(shouldForward, reconnect);
            ASSERT_EQ(extraSends.size(), 0);
        };

        auto hostSend = [this, reconnect](
                                const apacket& packet,
                                std::queue<emulation::apacket>* replies) {
            ASSERT_EQ(replies->size(), 0);
            bool shouldForward;
            mProxy->onGuestRecvData(&packet.mesg, packet.data.data(),
                                    &shouldForward, replies);
            if (reconnect) {
                ASSERT_FALSE(shouldForward);
                ASSERT_GT(replies->size(), 0);
            } else {
                ASSERT_TRUE(shouldForward);
                ASSERT_EQ(replies->size(), 0);
            }
        };

        EXPECT_FALSE(mProxy->shouldClose());
        // For new proxy (without snapshot), ADB_OPEN is already sent before
        // proxy setup. For snapshot load, it needs to forward ADB_OPEN
        std::queue<emulation::apacket> replies;
        apacket packet;
        if (reconnect) {
            apacket packet = buildPacketHostToGuest(ADB_OPEN, reconnect);
            packet.mesg.arg1 = 0;
            packet.mesg.data_length = strlen(kJdwpOpenStr) + 1;
            packet.data.assign(
                    (const uint8_t*)kJdwpOpenStr,
                    (const uint8_t*)kJdwpOpenStr + packet.mesg.data_length);
            hostSend(packet, &replies);
            EXPECT_EQ(1, replies.size());
            validateGuestToHost(replies.front().mesg, reconnect);
            replies.pop();
            EXPECT_FALSE(mProxy->shouldClose());
        }

        packet = buildPacketHostToGuest(ADB_WRTE, reconnect);
        packet.mesg.data_length = strlen(kJdwpHandshake);
        packet.data.assign(
                (const uint8_t*)kJdwpHandshake,
                (const uint8_t*)kJdwpHandshake + packet.mesg.data_length);
        hostSend(packet, &replies);
        EXPECT_FALSE(mProxy->shouldClose());
        if (reconnect) {
            validateGuestToHost(replies.front().mesg, reconnect);
            EXPECT_EQ(replies.front().mesg.command, ADB_OKAY);
            replies.pop();
            validateGuestToHost(replies.front().mesg, reconnect);
            EXPECT_EQ(replies.front().mesg.command, ADB_WRTE);
            EXPECT_EQ(replies.front().mesg.data_length, kJdwpHandshakeSize);
            EXPECT_EQ(0, memcmp(replies.front().data.data(), kJdwpHandshake,
                                kJdwpHandshakeSize));
            replies.pop();
        }

        packet = buildPacketGuestToHost(ADB_OKAY, reconnect);
        guestSend(packet);
        EXPECT_FALSE(mProxy->shouldClose());

        packet = buildPacketGuestToHost(ADB_WRTE, reconnect);
        packet.mesg.data_length = strlen(kJdwpHandshake);
        packet.data.assign(
                (const uint8_t*)kJdwpHandshake,
                (const uint8_t*)kJdwpHandshake + packet.mesg.data_length);
        guestSend(packet);
        EXPECT_FALSE(mProxy->shouldClose());

        packet = buildPacketHostToGuest(ADB_OKAY, reconnect);
        if (reconnect) {
            // The last ADB_OKAY will be delivered to the guest
            bool shouldForward = false;
            mProxy->onGuestRecvData(&packet.mesg, packet.data.data(),
                                    &shouldForward, &replies);
            EXPECT_TRUE(shouldForward);
            EXPECT_EQ(replies.size(), 0);
        } else {
            hostSend(packet, &replies);
        }
        EXPECT_FALSE(mProxy->shouldClose());
    }

    std::unique_ptr<JdwpProxy> mProxy;
};

TEST_F(JdwpProxyTest, JdwpReconnect) {
    jdwpConnectionAndHandshake(false);
    saveAndLoadSnapshot();
    jdwpConnectionAndHandshake(true);
}

TEST_F(JdwpProxyTest, JdwpRepeatBreakpoint) {
    jdwpConnectionAndHandshake(false);
    apacket breakpointEventPacket;
    JdwpCommandHeader breakpointEventHeader;
    {
        // Send a breakpoint message from guest
        // Message: 1 byte suspend policy, 4 bytes number of events,
        // 1 byte event kind, 4 bytes event id
        breakpointEventPacket = buildPacketGuestToHost(ADB_WRTE, false);
        const int kBreakpointMesgSize = kJdwpHeaderSize + 1 + 4 + 1 + 4;
        breakpointEventHeader = {
                kBreakpointMesgSize /*length*/,
                1 /*id*/,
                0 /*flags*/,
                CommandSet::Event /*command_set*/,
                EventCommand::Composite /*command*/,
        };
        breakpointEventPacket.mesg.data_length = kBreakpointMesgSize;
        breakpointEventPacket.data.resize(kBreakpointMesgSize);
        breakpointEventHeader.writeToBuffer(breakpointEventPacket.data.data());
        uint8_t* data = breakpointEventPacket.data.data() + kJdwpHeaderSize;
        *data = SuspendPolicy::All;
        data++;
        int numEvents = 1;
        memcpy(data, &numEvents, 4);
        data += 4;
        *data = EventKind::Exception;
        data++;
        int eventId = 1;
        memcpy(data, &eventId, 4);
        bool shouldForward = false;
        std::queue<emulation::apacket> packetQueue;
        mProxy->onGuestSendData(&breakpointEventPacket.mesg,
                                breakpointEventPacket.data.data(),
                                &shouldForward, &packetQueue);
        ASSERT_TRUE(shouldForward);
        ASSERT_EQ(packetQueue.size(), 0);
    }

    saveAndLoadSnapshot();
    jdwpConnectionAndHandshake(true);
    const int kRequestCmdId = 101;
    const int kRequestId = 102;
    {
        // Send a breakpoint request from host
        apacket packet = buildPacketHostToGuest(ADB_WRTE, true);
        const int kRequestMesgSize = kJdwpHeaderSize + 1 + 1;
        JdwpCommandHeader jdwpHeader = {
                kRequestMesgSize /*length*/,
                kRequestCmdId /*id*/,
                0 /*flags*/,
                CommandSet::EventRequest /*command_set*/,
                EventRequestCommand::Set /*command*/,
        };
        packet.mesg.data_length = kRequestMesgSize;
        packet.data.resize(kRequestMesgSize);
        jdwpHeader.writeToBuffer(packet.data.data());
        uint8_t* data = packet.data.data() + kJdwpHeaderSize;
        *data = EventKind::Exception;
        data++;
        *data = SuspendPolicy::All;
        data++;
        bool shouldForward = false;
        std::queue<emulation::apacket> packetQueue;
        mProxy->onGuestRecvData(&packet.mesg, packet.data.data(),
                                &shouldForward, &packetQueue);
        ASSERT_TRUE(shouldForward);
        ASSERT_EQ(packetQueue.size(), 0);
    }

    {
        // Guest should reply the request id
        // JdwpProxy should inject the breakpoint event
        apacket packet = buildPacketGuestToHost(ADB_WRTE, true);
        const int kReplyMesgSize = kJdwpHeaderSize + 4;
        JdwpCommandHeader jdwpHeader = {
                kReplyMesgSize /*length*/,
                kRequestCmdId /*id*/,
                kJdwpReplyFlag /*flags*/,
                CommandSet::EventRequest /*command_set*/,
                EventRequestCommand::Set /*command*/,
        };
        packet.mesg.data_length = kReplyMesgSize;
        packet.data.resize(kReplyMesgSize);
        jdwpHeader.writeToBuffer(packet.data.data());
        uint8_t* data = packet.data.data() + kJdwpHeaderSize;
        memcpy(data, &kRequestId, 4);
        std::queue<emulation::apacket> packetQueue;
        bool shouldForward = true;
        mProxy->onGuestSendData(&packet.mesg, packet.data.data(),
                                &shouldForward, &packetQueue);
        ASSERT_FALSE(shouldForward);
        ASSERT_EQ(packetQueue.size(), 2);
        validateGuestToHost(packetQueue.front().mesg, true);
        EXPECT_EQ(0, memcmp(packetQueue.front().data.data() + kJdwpHeaderSize,
                            &kRequestId, 4));
        packetQueue.pop();
        validateGuestToHost(packetQueue.front().mesg, true);
        JdwpCommandHeader repeatedHeader;
        repeatedHeader.parseFrom(packetQueue.front().data.data());
        EXPECT_EQ(repeatedHeader.length, breakpointEventHeader.length);
        EXPECT_EQ(repeatedHeader.flags, breakpointEventHeader.flags);
        EXPECT_EQ(repeatedHeader.command_set,
                  breakpointEventHeader.command_set);
        EXPECT_EQ(repeatedHeader.command, breakpointEventHeader.command);
        // The data packet should be the same except for the ID field
        const int kIdOffset = 6;
        EXPECT_EQ(0, memcmp(packetQueue.front().data.data() + kJdwpHeaderSize,
                            breakpointEventPacket.data.data() + kJdwpHeaderSize,
                            kIdOffset));
        EXPECT_EQ(0, memcmp(packetQueue.front().data.data() + kJdwpHeaderSize +
                                    kIdOffset,
                            &kRequestId, 4));
    }
}

}  // namespace

}  // namespace jdwp
}  // namespace android
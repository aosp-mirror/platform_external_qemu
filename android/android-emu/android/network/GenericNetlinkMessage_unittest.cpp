// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/network/GenericNetlinkMessage.h"

#include <gtest/gtest.h>

using android::network::GenericNetlinkMessage;
using android::network::genlmsghdr;
using android::network::nlmsghdr;

#define HWSIM_CMD_FRAME 2
#define HWSIM_ATTR_FRAME 3

static constexpr int family = 0;
static constexpr int hdrlen = 6;
static constexpr int flags = 0;
static constexpr uint8_t cmd = HWSIM_CMD_FRAME;
static const uint8_t version = 0;

TEST(GenericNetlinkMessageTest, BasicOperationsTest) {
    // Build basic generic netlink msg
    GenericNetlinkMessage msg(GenericNetlinkMessage::NL_AUTO_PORT,
                              GenericNetlinkMessage::NL_AUTO_SEQ, family,
                              hdrlen, flags, cmd, version);
    EXPECT_TRUE(msg.isValid());

    // verify netlink header
    const auto* hdr = msg.netlinkHeader();
    EXPECT_EQ(hdr->nlmsg_flags, flags);
    EXPECT_EQ(hdr->nlmsg_type, family);
    EXPECT_EQ(hdr->nlmsg_seq,
              GenericNetlinkMessage::NL_AUTO_SEQ);
    EXPECT_EQ(hdr->nlmsg_pid,
              GenericNetlinkMessage::NL_AUTO_PORT);
    EXPECT_EQ(msg.dataLen(), hdr->nlmsg_len);

    // verify generic netlink header
    const auto* gehdr = msg.genericNetlinkHeader();
    EXPECT_EQ(gehdr->cmd, cmd);
    EXPECT_EQ(gehdr->version, version);
    constexpr size_t kSize = 10;
    uint8_t frame[kSize];
    for (size_t i = 0; i < kSize; i++) {
        frame[i] = i + 1;
    }

    // Verify if attribute HWSIM_ATTR_FRAME is succesfully added
    EXPECT_TRUE(msg.putAttribute(HWSIM_ATTR_FRAME, frame, kSize));
    EXPECT_EQ(msg.dataLen(), msg.netlinkHeader()->nlmsg_len);
    uint8_t copy[kSize];
    EXPECT_TRUE(msg.getAttribute(HWSIM_ATTR_FRAME, copy, kSize));
    EXPECT_EQ(std::memcmp(frame, copy, kSize), 0);
    // Override previous value for HWSIM_ATTR_FRAME
    for (size_t i = 0; i < kSize; i++) {
        frame[i] = i + 2;
    }
    EXPECT_TRUE(msg.putAttribute(HWSIM_ATTR_FRAME, frame, kSize));
    struct iovec iov = msg.getAttribute(HWSIM_ATTR_FRAME);
    EXPECT_EQ(std::memcmp(frame, iov.iov_base, kSize), 0);
}

TEST(GenericNetlinkMessageTest, ConversionTest) {
    GenericNetlinkMessage msg(GenericNetlinkMessage::NL_AUTO_PORT,
                              GenericNetlinkMessage::NL_AUTO_SEQ, family,
                              hdrlen, flags, cmd, version);
    GenericNetlinkMessage msg2(msg.data(), msg.dataLen(), hdrlen);
    EXPECT_TRUE(msg2.isValid());
    const auto* hdr = msg2.netlinkHeader();
    EXPECT_EQ(hdr->nlmsg_flags, flags);
}

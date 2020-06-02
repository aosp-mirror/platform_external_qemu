// Copyright 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/network/Ieee80211Frame.h"

#include "android/network/Endian.h"

#include <gtest/gtest.h>

using android::base::IOVector;
using android::network::Ieee80211Frame;
using android::network::MacAddress;
// Copied over from common/ieee802_11_defs.h to avoid hostapd includes directory

struct ieee80211_hdr {
    uint16_t frame_control;
    uint16_t duration_id;
    uint8_t addr1[6];
    uint8_t addr2[6];
    uint8_t addr3[6];
    uint16_t seq_ctrl;
    /* followed by 'u8 addr4[6];' if ToDS and FromDS is set in data frame
     */
} __attribute__((packed));

static uint16_t toIEEE80211FrameControl(uint16_t type, uint16_t stype) {
    return (type << 2) | (stype << 4);
}

TEST(Ieee80211FrameTest, BasicCreation) {
    constexpr size_t IEEE80211_HDRLEN = sizeof(struct ieee80211_hdr);
    constexpr uint16_t WLAN_FC_TYPE_DATA = 2;
    constexpr uint16_t WLAN_FC_STYPE_DATA = 0;
    constexpr uint16_t WLAN_FC_FROMDS = 0x0200;
    constexpr uint8_t kMac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    struct ieee80211_hdr hdr;
    memcpy(hdr.addr1, kMac, ETH_ALEN);
    memcpy(hdr.addr2, kMac, ETH_ALEN);
    memcpy(hdr.addr3, kMac, ETH_ALEN);
    hdr.frame_control =
            toIEEE80211FrameControl(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
    hdr.frame_control |= WLAN_FC_FROMDS;
    const size_t kSize = 100;
    std::vector<uint8_t> packet(kSize);
    memcpy(packet.data(), &hdr, IEEE80211_HDRLEN);
    Ieee80211Frame frame(packet.data(), packet.size());
    EXPECT_EQ(frame.size(), packet.size());
    EXPECT_TRUE(frame.frameBody());
    EXPECT_TRUE(frame.isData());
    EXPECT_FALSE(frame.isMgmt());
    EXPECT_TRUE(frame.isFromDS());
    EXPECT_FALSE(frame.uses4Addresses());
    EXPECT_EQ(frame.hdrLength(), sizeof(ieee80211_hdr));
    EXPECT_TRUE(frame.addr1() == MacAddress(MACARG(hdr.addr1)));
    EXPECT_TRUE(frame.addr2() == MacAddress(MACARG(hdr.addr2)));
    EXPECT_TRUE(frame.addr3() == MacAddress(MACARG(hdr.addr3)));
}

TEST(Ieee80211FrameTest, conversionTest) {
    const uint8_t kSrc[] = {0x55, 0x54, 0x03, 0x04, 0x00, 0x00};

    const uint8_t kDst[] = {0x20, 0x30, 0x03, 0x04, 0x05, 0x01};

    const uint8_t kBssID[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

    // The first two bytes are the ether type.
    const uint8_t kPayload[] = {0x08,  // (uint8_t*)ETH_IP >> 8
                                0x00,  // (uint8_t*)ETH_IP
                                0x1,  0x2, 0x3, 0x4, 0x5, 0x1, 0x2,
                                0x3,  0x4, 0x5, 0x1, 0x2, 0x3, 0x4,
                                0x5,  0x1, 0x2, 0x3, 0x4, 0x5};

    // build up the ethernet packet.
    std::vector<uint8_t> ethernet;
    ethernet.insert(ethernet.end(), kDst, kDst + sizeof(kDst));
    ethernet.insert(ethernet.end(), kSrc, kSrc + sizeof(kSrc));
    ethernet.insert(ethernet.end(), kPayload, kPayload + sizeof(kPayload));

    // init Ieee80211 frame with ethernet
    Ieee80211Frame frame(ethernet, MacAddress(MACARG(kBssID)));
    EXPECT_TRUE(frame.isData());
    EXPECT_TRUE(frame.isFromDS());
    EXPECT_TRUE((frame.hdrLength() + ETH_ALEN + sizeof(kPayload)) ==
                frame.size());
    EXPECT_TRUE(MacAddress(MACARG(kDst)) == frame.addr1());
    EXPECT_TRUE(MacAddress(MACARG(kBssID)) == frame.addr2());
    EXPECT_TRUE(MacAddress(MACARG(kSrc)) == frame.addr3());
    auto outSg = frame.toEthernet();
    std::vector<uint8_t> packet(outSg.summedLength());
    EXPECT_EQ(outSg.size(), 2);
    outSg.copyTo(packet.data(), 0, outSg.summedLength());

    EXPECT_EQ(memcmp(packet.data(), kSrc, ETH_ALEN), 0);
    EXPECT_EQ(memcmp(packet.data() + ETH_ALEN, kBssID, ETH_ALEN), 0);

    // Test if payload(including ethertype) is the same.
    EXPECT_EQ(
            memcmp(ethernet.data() + ETH_ALEN * 2, packet.data() + ETH_ALEN * 2,
                   ethernet.size() - ETH_ALEN * 2),
            0);

    // Test with invalid ethertype
    ethernet[12] = 0x00;
    Ieee80211Frame frame2(ethernet, MacAddress(MACARG(kBssID)));
    // Ethertype is no longer skipped when copying.
    EXPECT_TRUE((frame2.hdrLength() + sizeof(kPayload) - 2) == frame2.size());
    outSg = frame2.toEthernet();
    EXPECT_EQ(outSg.size(), 0);
}
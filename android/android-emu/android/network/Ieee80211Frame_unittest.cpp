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
using android::network::CipherScheme;
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

constexpr uint16_t toIEEE80211FrameControl(uint16_t type, uint16_t stype) {
    return (type << 2) | (stype << 4);
}

static const uint8_t kMac[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static const uint8_t kSrc[] = {0x55, 0x54, 0x03, 0x04, 0x00, 0x00};
static const uint8_t kDst[] = {0x20, 0x30, 0x03, 0x04, 0x05, 0x01};
static const uint8_t kBssID[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};

// The first two bytes are the ether type.
static const uint8_t kPayload[] = {0x08,  // (uint8_t*)ETH_IP >> 8
                                   0x00,  // (uint8_t*)ETH_IP
                                   0x1,  0x2, 0x3, 0x4, 0x5, 0x1, 0x2,
                                   0x3,  0x4, 0x5, 0x1, 0x2, 0x3, 0x4,
                                   0x5,  0x1, 0x2, 0x3, 0x4, 0x5};

static const uint8_t kGTK[] = {0x68, 0xa3, 0xef, 0x80, 0x3d, 0x47, 0xc4, 0x15,
                               0x5c, 0x93, 0xc3, 0xa2, 0xfe, 0x8e, 0xd8, 0xdd};

static const uint8_t kPTK[] = {0xf2, 0x7f, 0xed, 0x9c, 0xab, 0x6d, 0x3b, 0x05,
                               0x93, 0x7a, 0xe8, 0xf6, 0xcf, 0x75, 0x03, 0xb7};

class Ieee80211FrameTest : public testing::Test {
public:
    static constexpr size_t IEEE80211_HDRLEN = sizeof(struct ieee80211_hdr);
    static constexpr uint16_t WLAN_FC_TYPE_DATA = 2;
    static constexpr uint16_t WLAN_FC_STYPE_DATA = 0;
    static constexpr uint16_t WLAN_FC_FROMDS = 0x0200;
    static constexpr size_t kSize = 100;

protected:
    Ieee80211Frame buildDataFrame() {
        struct ieee80211_hdr hdr;
        memcpy(hdr.addr1, kMac, ETH_ALEN);
        memcpy(hdr.addr2, kMac, ETH_ALEN);
        memcpy(hdr.addr3, kMac, ETH_ALEN);
        hdr.frame_control =
                toIEEE80211FrameControl(WLAN_FC_TYPE_DATA, WLAN_FC_STYPE_DATA);
        hdr.frame_control |= WLAN_FC_FROMDS;
        std::vector<uint8_t> packet(kSize, 0);
        memcpy(packet.data(), &hdr, IEEE80211_HDRLEN);
        Ieee80211Frame frame(packet.data(), packet.size());
        EXPECT_EQ(frame.size(), packet.size());
        return frame;
    }
};

TEST_F(Ieee80211FrameTest, BasicCreation) {
    auto frame = buildDataFrame();
    EXPECT_EQ(frame.size(), kSize);
    EXPECT_TRUE(frame.frameBody());
    EXPECT_TRUE(frame.isData());
    EXPECT_FALSE(frame.isDataQoS());
    EXPECT_FALSE(frame.isMgmt());
    EXPECT_TRUE(frame.isFromDS());
    EXPECT_FALSE(frame.uses4Addresses());
    EXPECT_EQ(frame.hdrLength(), sizeof(ieee80211_hdr));
    EXPECT_TRUE(frame.addr1() == MacAddress(MACARG(kMac)));
    EXPECT_TRUE(frame.addr2() == MacAddress(MACARG(kMac)));
    EXPECT_TRUE(frame.addr3() == MacAddress(MACARG(kMac)));
}

TEST_F(Ieee80211FrameTest, conversionTest) {
    // build up the ethernet packet.
    std::vector<uint8_t> ethernet;
    ethernet.insert(ethernet.end(), kDst, kDst + sizeof(kDst));
    ethernet.insert(ethernet.end(), kSrc, kSrc + sizeof(kSrc));
    ethernet.insert(ethernet.end(), kPayload, kPayload + sizeof(kPayload));

    // init Ieee80211 frame with ethernet
    auto frame = Ieee80211Frame::buildFromEthernet(
            ethernet.data(), ethernet.size(), MacAddress(MACARG(kBssID)));
    EXPECT_TRUE(frame->isData());
    EXPECT_TRUE(frame->isFromDS());
    EXPECT_TRUE((frame->hdrLength() + ETH_ALEN + sizeof(kPayload)) ==
                frame->size());
    EXPECT_TRUE(MacAddress(MACARG(kDst)) == frame->addr1());
    EXPECT_TRUE(MacAddress(MACARG(kBssID)) == frame->addr2());
    EXPECT_TRUE(MacAddress(MACARG(kSrc)) == frame->addr3());
    auto outSg = frame->toEthernet();
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

    // Test with invalid ethertype and expect nullptr returned.
    ethernet[12] = 0x00;
    auto frame2 = Ieee80211Frame::buildFromEthernet(
            ethernet.data(), ethernet.size(), MacAddress(MACARG(kBssID)));
    EXPECT_TRUE(frame2.get() == nullptr);
}

//TODO(wdu@) b/216352528 Re-enable this test after a test has been fixed. 
/*TEST_F(Ieee80211FrameTest, EncryptionDecryptionTest) {
    auto dataFrame = buildDataFrame();
    // Set up GTK and PTK for testing.
    std::vector<uint8_t> gtk(kGTK, kGTK + sizeof(kGTK));
    std::vector<uint8_t> ptk(kPTK, kPTK + sizeof(kPTK));
    dataFrame.setPTKForTesting(std::move(gtk), 0);
    dataFrame.setGTKForTesting(std::move(ptk), 1);

    // Test if size remains same when Cipher is NONE.
    size_t sizeBefore = dataFrame.size();
    EXPECT_TRUE(dataFrame.encrypt(CipherScheme::NONE));
    EXPECT_FALSE(dataFrame.isProtected());
    EXPECT_EQ(sizeBefore, dataFrame.size());

    // Test when Cipher is not supported.
    EXPECT_FALSE(dataFrame.encrypt(CipherScheme::TKIP));

    // Test when Cipher is CCMP.
    EXPECT_TRUE(dataFrame.encrypt(CipherScheme::CCMP));
    EXPECT_TRUE(dataFrame.isProtected());
    EXPECT_EQ(dataFrame.size(), sizeBefore +
                                        Ieee80211Frame::IEEE80211_CCMP_MIC_LEN +
                                        Ieee80211Frame::IEEE80211_CCMP_HDR_LEN);

    // Test if protected frame return false.
    EXPECT_FALSE(dataFrame.encrypt(CipherScheme::CCMP));

    // Test when Cipher is NONE.
    EXPECT_TRUE(dataFrame.decrypt(CipherScheme::NONE));

    // Test when Cipher is not supported.
    EXPECT_FALSE(dataFrame.decrypt(CipherScheme::TKIP));

    // Test decryption and verify size and contents.
    EXPECT_TRUE(dataFrame.decrypt(CipherScheme::CCMP));
    EXPECT_EQ(dataFrame.size(), sizeBefore);
    const auto* frameBody = dataFrame.frameBody();
    for (size_t i = 0; i < dataFrame.size() - dataFrame.hdrLength(); i++) {
        EXPECT_EQ(frameBody[i], 0);
    }
    EXPECT_FALSE(dataFrame.isProtected());
    // Test if unprotected frame return false.
    EXPECT_FALSE(dataFrame.decrypt(CipherScheme::CCMP));
}*/
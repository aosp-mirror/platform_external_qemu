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
#include "emulator/webrtc/RtcConfig.h"

#include <gtest/gtest.h>  // for SuiteApiResolver, Message, TestFa...
#include <string>         // for string

#include "gtest/gtest_pred_impl.h"  // for Test, EXPECT_EQ, TEST

using emulator::webrtc::RtcConfig;
using nlohmann::json;
using ::webrtc::PeerConnectionInterface;

static std::string VALID_TURN_CFG = R"#(
{
   "iceServers":[
      {
         "urls":"stun:stun.services.mozilla.com",
         "username":"louis@mozilla.com",
         "credential":"webrtcdemo"
      },
      {
         "urls":[
            "stun:stun.example.com",
            "stun:stun-1.example.com"
         ]
      }
   ]
}
)#";

TEST(RtcConfig, sets_servers) {
    auto cfg = RtcConfig::parse(VALID_TURN_CFG);
    EXPECT_EQ(cfg.servers.size(), 2);
    PeerConnectionInterface::IceServer ice = cfg.servers[0];
    EXPECT_EQ(ice.password, "webrtcdemo");
    EXPECT_EQ(ice.username, "louis@mozilla.com");
    EXPECT_EQ(ice.urls.size(), 1);
    EXPECT_EQ(ice.urls[0], "stun:stun.services.mozilla.com");

    // Parses the second ince set
    ice = cfg.servers[1];
    EXPECT_EQ(ice.urls.size(), 2);
    EXPECT_EQ(ice.urls[0], "stun:stun.example.com");
    EXPECT_EQ(ice.urls[1], "stun:stun-1.example.com");
}

TEST(RtcConfig, set_poolsize) {
    auto cfg = RtcConfig::parse(
            (std::string) R"#({ "iceCandidatePoolSize" : 5 })#");
    EXPECT_EQ(cfg.ice_candidate_pool_size, 5);
}

TEST(RtcConfig, set_bundlePolicy) {
    auto cfg = RtcConfig::parse(
            (std::string) R"#({ "bundlePolicy" : "balanced" })#");
    EXPECT_EQ(cfg.bundle_policy,
              PeerConnectionInterface::BundlePolicy::kBundlePolicyBalanced);

    cfg = RtcConfig::parse(
            (std::string) R"#({ "bundlePolicy" : "max-bundle" })#");
    EXPECT_EQ(cfg.bundle_policy,
              PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxBundle);

    cfg = RtcConfig::parse(
            (std::string) R"#({ "bundlePolicy" : "max-compat" })#");
    EXPECT_EQ(cfg.bundle_policy,
              PeerConnectionInterface::BundlePolicy::kBundlePolicyMaxCompat);
}

TEST(RtcConfig, set_type) {
    auto cfg = RtcConfig::parse(
            (std::string) R"#({ "iceTransportPolicy" : "relay" })#");
    EXPECT_EQ(cfg.type, PeerConnectionInterface::IceTransportsType::kRelay);

    cfg = RtcConfig::parse(
            (std::string) R"#({ "iceTransportPolicy" : "all" })#");
    EXPECT_EQ(cfg.type, PeerConnectionInterface::IceTransportsType::kAll);
}

TEST(RtcConfig, set_rtcpMuxPolicy) {
    auto cfg = RtcConfig::parse(
            (std::string) R"#({ "rtcpMuxPolicy" : "require" })#");

    EXPECT_EQ(cfg.rtcp_mux_policy,
              PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyRequire);

    cfg = RtcConfig::parse(
            (std::string) R"#({ "rtcpMuxPolicy" : "negotiate" })#");
    EXPECT_EQ(cfg.rtcp_mux_policy,
              PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyNegotiate);
}

TEST(RtcConfig, defaults) {
    auto cfg = RtcConfig::parse(json{});

    EXPECT_EQ(cfg.rtcp_mux_policy,
              PeerConnectionInterface::RtcpMuxPolicy::kRtcpMuxPolicyRequire);
    EXPECT_EQ(cfg.servers.size(), 1);
    PeerConnectionInterface::IceServer ice = cfg.servers[0];
    EXPECT_EQ(ice.uri, "stun:stun.l.google.com:19302");
}

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
#include "android/emulation/control/TurnConfig.h"
#include "android/base/testing/TestSystem.h"

#include <gtest/gtest.h>
#include <string>

using android::base::TestSystem;
using android::emulation::control::TurnConfig;
using nlohmann::json;
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

static std::string VALID_TURN_TWILIO_CFG = R"#(
{
   "username":"dc2d2894d5a9023620c467b0e71cfa6a35457e6679785ed6ae9856fe5bdfa269",
   "ice_servers":[
      {
         "urls":"stun:global.stun.twilio.com:3478"
      },
      {
         "username":"dc2d2894d5a9023620c467b0e71cfa6a35457e6679785ed6ae9856fe5bdfa269",
         "credential":"tE2DajzSJwnsSbc123",
         "urls":"turn:global.turn.twilio.com:3478?transport=udp"
      },
      {
         "username":"dc2d2894d5a9023620c467b0e71cfa6a35457e6679785ed6ae9856fe5bdfa269",
         "credential":"tE2DajzSJwnsSbc123",
         "urls":"turn:global.turn.twilio.com:3478?transport=tcp"
      },
      {
         "username":"dc2d2894d5a9023620c467b0e71cfa6a35457e6679785ed6ae9856fe5bdfa269",
         "credential":"tE2DajzSJwnsSbc123",
         "urls":"turn:global.turn.twilio.com:443?transport=tcp"
      }
   ],
   "date_updated":"Fri, 01 May 2020 01:42:57 +0000",
   "account_sid":"ACXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX",
   "ttl":"86400",
   "date_created":"Fri, 01 May 2020 01:42:57 +0000",
   "password":"tE2DajzSJwnsSbc123"
})#";

std::string NO_ICE = R"#(
{
}
)#";

#ifndef _WIN32
TEST(TurnConfig, Timeout) {
    TurnConfig cfg("sleep 2000");
    EXPECT_FALSE(cfg.validCommand());
}

TEST(TurnConfig, MaxTurnCfgTime) {
    TestSystem testSystem("/bin", 32);
    testSystem.envSet("ANDROID_EMU_MAX_TURNCFG_TIME", "100");
    TurnConfig cfg("sleep 800");
    EXPECT_FALSE(cfg.validCommand());
}

TEST(TurnConfig, Valid) {
    TurnConfig cfg("printf '" + VALID_TURN_CFG + "'");
    EXPECT_TRUE(cfg.validCommand());
}


TEST(TurnConfig, ValidTwilio) {
    TurnConfig cfg("printf '" + VALID_TURN_TWILIO_CFG + "'");
    EXPECT_TRUE(cfg.validCommand());
}


TEST(TurnConfig, EmptyIsValid) {
    TurnConfig cfg("");
    EXPECT_TRUE(cfg.validCommand());
    json expect = R"#({})#"_json;
    auto snippet = cfg.getConfig();
    EXPECT_EQ(expect, snippet);
}

TEST(TurnConfig, BadExit) {
    EXPECT_FALSE(TurnConfig::producesValidTurn("false"));
}

TEST(TurnConfig, BadJson) {
    EXPECT_FALSE(TurnConfig::producesValidTurn("printf '{{}'"));
}

TEST(TurnConfig, MissingIce) {
    TurnConfig cfg("printf '" + NO_ICE + "'");
    EXPECT_FALSE(cfg.validCommand());
}

TEST(TurnConfig, ProducesTurn) {
    TurnConfig cfg("printf '" + VALID_TURN_CFG + "'");
    json expect = json::parse(VALID_TURN_CFG);
    auto snippet = cfg.getConfig();
    EXPECT_EQ(expect, snippet);
}


TEST(TurnConfig, ProducesTwilioTurn) {
    TurnConfig cfg("printf '" + VALID_TURN_TWILIO_CFG + "'");
    json expect = json::parse(VALID_TURN_TWILIO_CFG);
    auto snippet = cfg.getConfig();
    EXPECT_EQ(expect, snippet);
}
#endif

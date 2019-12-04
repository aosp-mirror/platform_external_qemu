// Copyright 2019 The Android Open Source Project
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

#include "android/emulation/control/AdbConnection.h"
#include <gtest/gtest.h>
#include "android/base/async/TestAsyncSocket.h"
#include "android/base/testing/TestSystem.h"

#include <string>
#include <thread>
#include <vector>

using android::emulation::AdbConnection;
using emulator::net::TestAsyncSocketAdapter;

enum class AdbWireMessage {
    A_SYNC = 0x434e5953,
    A_CNXN = 0x4e584e43,
    A_AUTH = 0x48545541,
    A_OPEN = 0x4e45504f,
    A_OKAY = 0x59414b4f,
    A_CLSE = 0x45534c43,
    A_WRTE = 0x45545257,
};

// ADB protocol version.
// Version revision:
// 0x01000000: original
// 0x01000001: skip checksum (Dec 2017)
#define A_VERSION_MIN 0x01000000
#define A_VERSION_SKIP_CHECKSUM 0x01000001
#define A_VERSION 0x01000001

#define ADB_AUTH_TOKEN 1
#define ADB_AUTH_SIGNATURE 2
#define ADB_AUTH_RSAPUBLICKEY 3

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

using android::base::System;

// Secret token that gets verified.
uint8_t challenge_token[20] = {0xE8, 0x99, 0xE1, 0xFF, 0x95, 0x9B, 0x8F,
                               0x6B, 0x54, 0xBE, 0xCE, 0xC5, 0x42, 0xF1,
                               0x93, 0x7D, 0x3,  0x99, 0xA2, 0x32};

// Private key that can be used to answer challenge token above.
std::string privkey = R"##(-----BEGIN PRIVATE KEY-----
MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCxc0jUMYZ5fJSo
7t0MEfGDV9GPhdvOfgKv3GRaqnPuTD04SCXPwu7iJ9y6rS/Rk1CsW9HI+LcQvu5h
AsbpzDW88Sg5kSWfbEWP7EgPfdcfWdOfaRFZ/0J/sRejvCnekmnPF75irEYPI/sx
jL2xIJ1kTOywOCBB3l/xkPPSLPt2159Lm3v+pMdinq6rp8RQ7XK9pj4dv4tb778+
mLtp4p6FhOH6rU/q4848v4CM1/YVXZZaMKmos4PfvFLknzCmd3xN2o1MusP2OSxb
NkXXpCTsJLufcb3ItBgLyEri/eOQcYzpV84vTHwqr121stDpjVAR3H4AaelHZqOF
JKY5NVJTAgMBAAECggEASD7BdfK75xY7iBPH1zQu+eR1I1PCS+2ttl+qU+d1z50m
h5WIH3Ajxduo2C/OeirZ+3JelM396klxz/lLdsB3WHdugxF/GcsA/zmZlQUM4my1
5f7m25c7QbWeBEGFYmKFxZTLJG0zENL7YA8G4+h9a+qNqqkPKQIaWcVEH1vE/Xra
hNwD6wYyj01aVAdSDKgu6EtIpmVDlgDFE33JQnHM9r3c6o6QdNQTlhGyQ+SX2rCx
y3a0pW4MlGas1RtWDwOHt6XunSn2O6YUGuEXLhixFxv/rhr3+x5TSFcHB9Lg8/Rm
OEvW9614/C0ePkDuoxcdGbauMz1JU4O0xkCQameJIQKBgQDYCjmQJ3s55G6nIrOe
uE5R8m1QpbZhg2w/8J5dUirsidw7WG5Wo3bnd5RTciti59r7TUUD/XS7BkIUiRZk
GKHvC+W2QJT3u7oCY2KGXhCAniwbckHLsVnfPMqKyN+LKU8SbXqos9pZUz7nAmvG
G7FVrrd2lYnjDSgMMPr+8cO/CQKBgQDSRcp0jUifpoYOnyO3Lt50s94Y4J3SEDJW
N/DXtck9WOjDuGA379KZV2BTkBVKoZQ3a3qamHjGBkClSg01/no0jhAuieMt8UTG
NvKGx4Ec8pBOkJleeDwNkcaC62AFjlz2+aWe4FFpnSRp7S20R+dMOGTLc+YaD6xo
JLvi2f2BewKBgBNSDsXSkhWiVTcDRncKWo6/lIEi4MWlwDeTqEYGRCp1RcnU5cE/
yzF2I0C3NCQbQh05UtPBhf/31k8J14PKJClBsiBzdB8XndH622PS47zs6FroA/RY
fwYU5LQ2tK84WYb3XYHa28sjQ7vbHpJQBbL49hVX2EYC9jLo6nmEW5IpAoGABAoZ
MJICQiblzmQaQIui9GT8MEgoX/+1p9hdRReV7RrHJfNlzc1Ko219ST2sWwmtmj7z
VQL21v8JwOMiS9Y+rMHJ58r4VUqcQp6NnC86+L5kLU4z1A/FP5F8WcmBx7mLaac0
GlA+4COHro1C4oK7G8i9jvcEBZ4ldr616U68wv8CgYEAgH2P+jwmAOiEDGJIvTOP
IPkwJ9Kz2z6z4JxfQxfo2sdM8aCrgsXvfFIFZpAWZ13HXzKFNjfdZ3tNlJOuiiEP
Nw3rvNsjZXrpaIYpIKHL/QJmRM7MO1El4ebV3/qBZl0NU33fguD4P3FmqZmh2BvJ
Uauxw86jrPSgjbiRB8FyvTo=
-----END PRIVATE KEY-----)##";

const int FAST_TIMEOUT = 20;

std::vector<std::string> addMessage(AdbWireMessage cmd,
                                    uint32_t arg0,
                                    uint32_t arg1,
                                    std::string msg = "") {
    std::vector<std::string> wire;
    amessage message = {(uint32_t)cmd,        arg0, arg1,
                        (uint32_t)msg.size(), 0,    (uint32_t)cmd ^ 0xffffffff};
    wire.emplace_back(std::string((char*)&message, sizeof(message)));
    wire.emplace_back(std::move(msg));
    return wire;
}

std::vector<std::string> connectMsg() {
    return addMessage(AdbWireMessage::A_CNXN, 0x01000001, 1024 * 255,
                      "features=shell_v2,foo,bar");
}

std::vector<std::string> authMsg() {
    return addMessage(AdbWireMessage::A_AUTH, 0, 0, "#{kYbV");
}

std::vector<std::string> okayMsg(int idx, int to) {
    return addMessage(AdbWireMessage::A_OKAY, idx, to, "some_token");
}

std::vector<std::string> flatten(std::vector<std::vector<std::string>> flat) {
    std::vector<std::string> wire;
    for (const auto& vec : flat) {
        for (const auto& str : vec) {
            wire.push_back(str);
        }
    }
    return wire;
}

TEST(AdbConnectionTest, noResponseShouldTimeout) {
    auto taas = new TestAsyncSocketAdapter({});
    AdbConnection::setAdbSocket(taas);
    auto stream = AdbConnection::connection(FAST_TIMEOUT)
                          ->open("shell:ls", FAST_TIMEOUT);

    EXPECT_TRUE(stream->bad());
    // Connect message has been send.
    EXPECT_NE(taas->mSend.size(), 0);
}

TEST(AdbConnectionTest, secondResponseShouldSucceedAfterTimeout) {
    // This validates that we send another connect message.
    auto taas = new TestAsyncSocketAdapter({});
    AdbConnection::setAdbSocket(taas);
    auto adb = AdbConnection::connection(FAST_TIMEOUT);
    auto stream = adb->open("shell:ls", FAST_TIMEOUT);

    EXPECT_TRUE(stream->bad());
    // Connect message has been send.
    auto sendSize = taas->mSend.size();
    EXPECT_NE(sendSize, 0);

    // We expect another connect message to have been send.
    auto con = connectMsg();
    taas->mRecv.push_back(con[0]);
    taas->mRecv.push_back(con[1]);
    stream = adb->open("shell:ls", FAST_TIMEOUT);

    // We send a new connect sequence
    EXPECT_NE(sendSize, taas->mSend.size());

    // And got connected so feature is available.
    EXPECT_TRUE(adb->hasFeature("foo"));
}

TEST(AdbConnectionTest, canParseFeatures) {
    auto taas = new TestAsyncSocketAdapter(connectMsg());
    AdbConnection::setAdbSocket(taas);
    auto adb = AdbConnection::connection(FAST_TIMEOUT);

    // Setup connection by requesting a service..
    auto stream = AdbConnection::connection()->open("shell:ls", FAST_TIMEOUT);

    EXPECT_TRUE(adb->hasFeature("foo"));
    EXPECT_FALSE(adb->hasFeature("fos"));
}

// Disabled until we support loading rsa keys from memory..
TEST(AdbConnectionTest, DISABLED_handleAuth) {
    auto taas = new TestAsyncSocketAdapter({});
    AdbConnection::setAdbSocket(taas);
    auto adb = AdbConnection::connection(FAST_TIMEOUT);

    // Connect to adb..
    taas->waitForConnect();
    auto t = std::thread([&taas]() {
        taas->signalRecv(authMsg());
        taas->signalRecv(connectMsg());
        taas->signalRecv(okayMsg(1, 1));
    });

    // Setup connection by requesting a service..
    auto stream = AdbConnection::connection(FAST_TIMEOUT)
                          ->open("shell:ls", FAST_TIMEOUT);
    t.join();
    // We should have send an A_OPEN and A_AUTH answer.
    EXPECT_TRUE(stream->good());
}

TEST(AdbConnectionTest, writeWaitsForOkay) {
    std::string hello = "Hello";
    std::string world = "World";

    auto taas = new TestAsyncSocketAdapter(connectMsg());
    AdbConnection::setAdbSocket(taas);
    auto adb = AdbConnection::connection(FAST_TIMEOUT);

    // Setup connection by requesting a service..
    taas->asyncReader();
    taas->addRecv(okayMsg(1, 1));
    taas->signalRecv();
    auto stream = AdbConnection::connection()->open("shell:ls", 200000000);
    stream->setWriteTimeout(100);
    stream->write(hello.data(), hello.size());
    // This will never get written, as we never received an OKAY message
    // indicating we can write the enxt one.
    stream->write(world.data(), world.size());
    taas->close();
    // A message with the string "Hello should have been written."
    EXPECT_TRUE(std::find(taas->mSend.begin(), taas->mSend.end(), hello) !=
                taas->mSend.end());
    // But no World.
    EXPECT_FALSE(std::find(taas->mSend.begin(), taas->mSend.end(), world) !=
                 taas->mSend.end());
}

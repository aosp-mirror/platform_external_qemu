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

#include "android/emulation/control/adb/AdbConnection.h"
#include <gtest/gtest.h>
#include "android/base/async/TestAsyncSocket.h"
#include "android/base/testing/TestLooper.h"
#include "android/base/testing/TestSystem.h"

#include <atomic>
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
using android::emulation::AdbState;

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

// Make this too aggressive and you can see timeouts! (tsan for example)
const int FAST_TIMEOUT = 200;

using emulator::net::Message;

const Message DO_NO_REPLY{};

Message addMessage(AdbWireMessage cmd,
                   uint32_t arg0,
                   uint32_t arg1,
                   std::string msg = "") {
    Message wire;
    amessage message = {(uint32_t)cmd,        arg0, arg1,
                        (uint32_t)msg.size(), 0,    (uint32_t)cmd ^ 0xffffffff};
    wire.emplace_back(std::string((char*)&message, sizeof(message)));
    if (!msg.empty())
        wire.emplace_back(std::move(msg));
    return wire;
}

Message connectMsg() {
    return addMessage(AdbWireMessage::A_CNXN, 0x01000001, 1024 * 255,
                      "features=shell_v2,foo,bar");
}

Message authMsg() {
    return addMessage(AdbWireMessage::A_AUTH, 0, 0, "#{kYbV");
}

Message okayMsg(int idx, int to) {
    return addMessage(AdbWireMessage::A_OKAY, idx, to, "");
}

Message closeMsg(int idx, int to) {
    return addMessage(AdbWireMessage::A_CLSE, idx, to, "");
}

using namespace ::android::base::testing;

class AdbConnectionTest : public ::testing::Test {
public:
    AdbConnectionTest() : mSocket(new TestAsyncSocketAdapter()) {
        AdbConnection::setAdbSocket(mSocket);
        mSocket->connectSync();
    }

    ~AdbConnectionTest() {
        AdbConnection::setAdbSocket(nullptr);
    }

protected:
    android::base::TestLooper mLooper;
    TestAsyncSocketAdapter* mSocket;
};

TEST(TestSocket, basic) {
    TestAsyncSocketAdapter ts;
    ts.connectSync();
    EXPECT_TRUE(ts.connected());
    ts.close();
    EXPECT_FALSE(ts.connected());
}

TEST_F(AdbConnectionTest, socketReflectsState) {
    // The state of the socket should be properly reflected by the state() call
    mSocket->addRecvs({".*", "host::features=.*"}, connectMsg());
    auto adb = AdbConnection::connection(FAST_TIMEOUT);
    EXPECT_EQ(AdbState::connected, adb->state());
    mSocket->close();
    EXPECT_EQ(AdbState::disconnected, adb->state());
    mSocket->connectSync(FAST_TIMEOUT);
    EXPECT_EQ(AdbState::socket, adb->state());
}

TEST_F(AdbConnectionTest, noResponseShouldTimeout) {
    // If ABDB never answers we return a bad stream.
    mSocket->connectSync(FAST_TIMEOUT);
    EXPECT_TRUE(mSocket->connected());
    mSocket->addRecvs({".*", "host::features=.*"}, {DO_NO_REPLY});
    mSocket->addRecvs({".*", "host::features=.*"}, {DO_NO_REPLY});
    mSocket->addRecvs({".*", "host::features=.*"}, {DO_NO_REPLY});

    auto adb = AdbConnection::connection(FAST_TIMEOUT);
    auto stream = adb->open("shell:ls", FAST_TIMEOUT);

    EXPECT_TRUE(stream->bad());
}

TEST_F(AdbConnectionTest, eventuallyConnectAfterTimeout) {
    // This simulates the scenario where ADBD goes down and we recover.
    mSocket->addRecvs({".*", "host::features=.*"}, {DO_NO_REPLY});
    auto adb = AdbConnection::connection(FAST_TIMEOUT);
    // Note we might be in some in between state due to concurrency,
    // but we are def. not connected.
    EXPECT_NE(AdbState::connected, adb->state());

    // Eventually we should connect..
    int max_attempts = 10;
    while (max_attempts > 0 && adb->state() != AdbState::connected) {
        mSocket->addRecvs({".*", "host::features=.*"}, connectMsg());
        adb = AdbConnection::connection(FAST_TIMEOUT);
        max_attempts--;
    }

    EXPECT_GT(max_attempts, 0);
}

TEST_F(AdbConnectionTest, canParseFeatures) {
    // We properly extract features from ADBD
    mSocket->addRecvs({".*", "host::features=.*"}, connectMsg());
    auto adb = AdbConnection::connection(FAST_TIMEOUT);
    EXPECT_EQ(AdbState::connected, adb->state());
    EXPECT_TRUE(adb->hasFeature("foo"));
    EXPECT_FALSE(adb->hasFeature("fos"));
}

// Disabled until we support loading rsa keys from memory..
TEST_F(AdbConnectionTest, DISABLED_handleAuth) {
    auto adb = AdbConnection::connection(FAST_TIMEOUT);

    // Setup connection by requesting a service..
    auto stream = adb->open("shell:ls", FAST_TIMEOUT);
    // We should have send an A_OPEN and A_AUTH answer.
    EXPECT_TRUE(stream->good());
}

TEST_F(AdbConnectionTest, writeWaitsForOkay) {
    // Validates that we observe the WRITE->okay protocol.
    std::string hello = "Hello";
    std::string world = "World";
    mSocket->addRecvs({".*", "host::features=.*"}, connectMsg());
    mSocket->addRecvs({"OPEN.*", "shell:ls"}, okayMsg(1, 1));
    mSocket->addRecvs({"WRTE.*", "Hello"}, okayMsg(1, 1));
    mSocket->addRecvs({"WRTE.*", "World"}, okayMsg(1, 1));
    mSocket->addRecvs({"CLSE.*"}, closeMsg(1, 1));
    mSocket->addRecvs({"CLSE.*"}, DO_NO_REPLY);

    auto adb = AdbConnection::connection(FAST_TIMEOUT);
    EXPECT_EQ(AdbState::connected, adb->state());
    auto stream = adb->open("shell:ls", FAST_TIMEOUT);
    EXPECT_EQ(AdbState::connected, adb->state());

    stream->setWriteTimeout(FAST_TIMEOUT);
    stream->write(hello.data(), hello.size());
    stream->write(world.data(), world.size());
    stream->close();
}

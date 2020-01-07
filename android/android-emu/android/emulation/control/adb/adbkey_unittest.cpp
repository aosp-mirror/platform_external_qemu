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

#include "android/emulation/control/adb/adbkey.h"
#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include <vector>
#include "android/base/files/PathUtils.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

namespace android {
namespace base {

// Secret token that gets verified.
const uint8_t challenge_token[20] = {0xE8, 0x99, 0xE1, 0xFF, 0x95, 0x9B, 0x8F,
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

class AdbKeyTest : public ::testing::Test {
public:
    AdbKeyTest() {
        mTestDir = mTestSystem.getTempRoot();
        mTestDir->makeSubDir(".android");
        mTestSystem.setHomeDirectory(mTestDir->pathString());
    }

    ~AdbKeyTest() {}

protected:
    TestSystem mTestSystem{"/", System::kProgramBitness};
    TestTempDir* mTestDir;
    const char* tstKey = "adbsamplekey";
};

TEST_F(AdbKeyTest, key_does_not_exist) {
    EXPECT_EQ("", getAdbKeyPath("thisshouldnotexist.boo"));
}

TEST_F(AdbKeyTest, find_keys_in_default_path) {
    mTestDir->makeSubFile(".android/adbkey");
    mTestDir->makeSubFile(".android/adbkey.pub");
    EXPECT_NE("", getPrivateAdbKeyPath());
    EXPECT_NE("", getPublicAdbKeyPath());
}

TEST_F(AdbKeyTest, generate_writes_a_key) {
    std::string keyFile = pj(mTestDir->pathString(), ".android", tstKey);
    EXPECT_EQ("", getAdbKeyPath(tstKey));
    EXPECT_TRUE(adb_auth_keygen(keyFile.c_str()));
    EXPECT_NE("", getAdbKeyPath(tstKey));
}

TEST_F(AdbKeyTest, can_create_pub_from_generated_priv) {
    std::string pubkey;
    std::string keyFile = pj(mTestDir->pathString(), ".android", tstKey);
    EXPECT_TRUE(adb_auth_keygen(keyFile.c_str()));
    EXPECT_TRUE(pubkey_from_privkey(keyFile, &pubkey));
    EXPECT_NE("", pubkey);
}

TEST_F(AdbKeyTest, can_sign_token) {
    std::string pubkey;
    std::string keyFile = pj(mTestDir->pathString(), ".android", kPrivateKeyFileName);
    std::ofstream out(keyFile);
    out << privkey << std::endl;
    out.close();


    EXPECT_TRUE(pubkey_from_privkey(keyFile, &pubkey));
    EXPECT_NE("", pubkey);

    int siglen = 256;
    std::vector<uint8_t> signed_token{};
    signed_token.resize(siglen);
    EXPECT_TRUE(sign_auth_token(challenge_token, sizeof(challenge_token),
                                signed_token.data(), siglen));

    EXPECT_NE("", std::string((char*)signed_token.data(), siglen));
}

}  // namespace base
}  // namespace android

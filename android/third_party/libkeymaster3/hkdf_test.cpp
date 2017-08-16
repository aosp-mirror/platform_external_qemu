/*
 * Copyright 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hkdf.h"

#include <gtest/gtest.h>
#include <string.h>

#include "android_keymaster_test_utils.h"

using std::string;

namespace keymaster {
namespace test {

struct HkdfTest {
    const char* key_hex;
    const char* salt_hex;
    const char* info_hex;
    const char* output_hex;
};

// These test cases are taken from
// https://tools.ietf.org/html/rfc5869#appendix-A.
static const HkdfTest kHkdfTests[] = {
    {
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "000102030405060708090a0b0c",
        "f0f1f2f3f4f5f6f7f8f9",
        "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf34007208d5b887185865",
    },
    {
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c"
        "2d2e2f303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f",
        "606162636465666768696a6b6c6d6e6f707172737475767778797a7b7c7d7e7f808182838485868788898a8b8c"
        "8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabacadaeaf",
        "b0b1b2b3b4b5b6b7b8b9babbbcbdbebfc0c1c2c3c4c5c6c7c8c9cacbcccdcecfd0d1d2d3d4d5d6d7d8d9dadbdc"
        "dddedfe0e1e2e3e4e5e6e7e8e9eaebecedeeeff0f1f2f3f4f5f6f7f8f9fafbfcfdfeff",
        "b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c59045a99cac7827271cb41c65e"
        "590e09da3275600c2f09b8367793a9aca3db71cc30c58179ec3e87c14c01d5c1f3434f1d87",
    },
    {
        "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b", "", "",
        "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d9d201395faa4b61a96c8",
    },
};

TEST(HkdfTest, Hkdf) {
    for (auto& test : kHkdfTests) {
        const string key = hex2str(test.key_hex);
        const string salt = hex2str(test.salt_hex);
        const string info = hex2str(test.info_hex);
        const string expected = hex2str(test.output_hex);
        size_t output_len = expected.size();
        uint8_t output[output_len];
        Rfc5869Sha256Kdf hkdf;
        ASSERT_TRUE(hkdf.Init(reinterpret_cast<const uint8_t*>(key.data()), key.size(),
                              reinterpret_cast<const uint8_t*>(salt.data()), salt.size()));
        ASSERT_TRUE(hkdf.GenerateKey(reinterpret_cast<const uint8_t*>(info.data()), info.size(),
                                     output, output_len));
        EXPECT_EQ(0, memcmp(output, expected.data(), output_len));
    }
}

}  // namespace test
}  // namespace keymaster

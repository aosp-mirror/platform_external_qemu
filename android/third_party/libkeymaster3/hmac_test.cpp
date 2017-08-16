/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "hmac.h"

#include <gtest/gtest.h>
#include <string.h>

#include "android_keymaster_test_utils.h"

using std::string;

namespace keymaster {

namespace test {

struct HmacTest {
    const char* data;
    const char* key;
    uint8_t digest[32];
};

static const HmacTest kHmacTests[] = {
    {
        "Test Using Larger Than Block-Size Key - Hash Key First",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        {
            0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f, 0x0d, 0x8a, 0x26, 0xaa, 0xcb, 0xf5,
            0xb7, 0x7f, 0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28, 0xc5, 0x14, 0x05, 0x46, 0x04, 0x0f,
            0x0e, 0xe3, 0x7f, 0x54,
        },
    },
    {
        "The test message for the MD2, MD5, and SHA-1 hashing algorithms.",
        "46697265666f7820616e64205468756e64657242697264206172652061776573"
        "6f6d652100",
        {
            0x05, 0x75, 0x9a, 0x9e, 0x70, 0x5e, 0xe7, 0x44, 0xe2, 0x46, 0x4b, 0x92, 0x22, 0x14,
            0x22, 0xe0, 0x1b, 0x92, 0x8a, 0x0c, 0xfe, 0xf5, 0x49, 0xe9, 0xa7, 0x1b, 0x56, 0x7d,
            0x1d, 0x29, 0x40, 0x48,
        },
    },
};

TEST(HmacTest, SHA256) {
    for (size_t i = 0; i < 2; i++) {
        const HmacTest& test(kHmacTests[i]);

        HmacSha256 hmac;
        const string key = hex2str(test.key);
        Buffer key_buffer(key.data(), key.size());
        ASSERT_TRUE(hmac.Init(key_buffer));

        uint8_t digest_copy[sizeof(test.digest)];
        memcpy(digest_copy, test.digest, sizeof(test.digest));
        Buffer digest_buffer(reinterpret_cast<uint8_t*>(digest_copy), sizeof(digest_copy));

        Buffer data_buffer(test.data, strlen(test.data));
        EXPECT_TRUE(hmac.Verify(data_buffer, digest_buffer));

        digest_copy[16] ^= 0x80;
        digest_buffer.Reinitialize(reinterpret_cast<uint8_t*>(digest_copy), sizeof(digest_copy));
        EXPECT_FALSE(hmac.Verify(data_buffer, digest_buffer));
    }
}

}  // namespace test
}  // namespace keymaster

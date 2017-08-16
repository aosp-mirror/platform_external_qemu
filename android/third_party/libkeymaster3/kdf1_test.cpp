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

#include "kdf1.h"
#include <gtest/gtest.h>
#include <string.h>

#include "android_keymaster_test_utils.h"

using std::string;

namespace keymaster {

namespace test {

struct Kdf1Test {
    const char* key_hex;
    const char* expected_output_hex;
    keymaster_digest_t digest_type;
};

static const Kdf1Test kKdf1Tests[] = {
    {"032e45326fa859a72ec235acff929b15d1372e30b207255f0611b8f785d764374152e0ac009e509e7ba30cd2f1778"
     "e113b64e135cf4e2292c75efe5288edfda4",
     "5f8de105b5e96b2e490ddecbd147dd1def7e3b8e0e6a26eb7b956ccb8b3bdc1ca975bc57c3989e8fbad31a224655d"
     "800c46954840ff32052cdf0d640562bdfadfa263cfccf3c52b29f2af4a1869959bc77f854cf15bd7a25192985a842"
     "dbff8e13efee5b7e7e55bbe4d389647c686a9a9ab3fb889b2d7767d3837eea4e0a2f04",
     KM_DIGEST_SHA1}};

TEST(Kdf1Test, Kdf1) {
    for (auto& test : kKdf1Tests) {
        const string key = hex2str(test.key_hex);
        const string expected_output = hex2str(test.expected_output_hex);
        size_t output_len = expected_output.size();
        uint8_t output[output_len];

        Kdf1 kdf1;
        ASSERT_TRUE(
            kdf1.Init(test.digest_type, reinterpret_cast<const uint8_t*>(key.data()), key.size()));
        ASSERT_TRUE(kdf1.GenerateKey(nullptr, 0, output, output_len));
        EXPECT_EQ(0, memcmp(output, expected_output.data(), output_len));
    }
}

}  // namespace test

}  // namespace keymaster

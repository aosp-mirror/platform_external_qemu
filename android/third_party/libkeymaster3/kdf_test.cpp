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

#include "kdf.h"
#include <gtest/gtest.h>

namespace keymaster {

namespace test {

class ForTestAbstractKdf : public Kdf {
    bool GenerateKey(const uint8_t* /* info */, size_t /* info_len */, uint8_t* /* output */,
                     size_t /* output_len */) {
        return true;
    }
};

TEST(KdfTest, Kdf) {
    ForTestAbstractKdf kdf;
    uint8_t key[128];
    uint8_t salt[128];
    ASSERT_TRUE(kdf.Init(KM_DIGEST_SHA1, key, 128, salt, 128));
    ASSERT_TRUE(kdf.Init(KM_DIGEST_SHA_2_256, key, 128, salt, 128));
    ASSERT_TRUE(kdf.Init(KM_DIGEST_SHA1, key, 128, nullptr, 0));
    ASSERT_FALSE(kdf.Init(KM_DIGEST_MD5, key, 128, salt, 128));
    ASSERT_FALSE(kdf.Init(KM_DIGEST_SHA1, nullptr, 0, salt, 128));
    ASSERT_FALSE(kdf.Init(KM_DIGEST_SHA1, nullptr, 128, salt, 128));
    ASSERT_FALSE(kdf.Init(KM_DIGEST_SHA1, key, 0, salt, 128));
}

}  // namespace test

}  // namespace keymaster

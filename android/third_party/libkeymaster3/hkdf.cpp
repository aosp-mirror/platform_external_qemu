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

#include <new>

#include <keymaster/android_keymaster_utils.h>

#include "hmac.h"

namespace keymaster {

bool Rfc5869Sha256Kdf::GenerateKey(const uint8_t* info, size_t info_len, uint8_t* output,
                                   size_t output_len) {
    if (!is_initialized_ || output == nullptr)
        return false;
    /**
     * Step 1. Extract: PRK = HMAC-SHA256(actual_salt, secret)
     * https://tools.ietf.org/html/rfc5869#section-2.2
     */
    HmacSha256 prk_hmac;
    bool result;
    if (salt_.get() != nullptr && salt_len_ > 0) {
        result = prk_hmac.Init(salt_.get(), salt_len_);
    } else {
        UniquePtr<uint8_t[]> zeros(new uint8_t[digest_size_]);
        if (zeros.get() == nullptr)
            return false;
        /* If salt is not given, digest size of zeros are used. */
        memset(zeros.get(), 0, digest_size_);
        result = prk_hmac.Init(zeros.get(), digest_size_);
    }
    if (!result)
        return false;

    UniquePtr<uint8_t[]> pseudo_random_key(new uint8_t[digest_size_]);
    if (pseudo_random_key.get() == nullptr || digest_size_ != prk_hmac.DigestLength())
        return false;
    result =
        prk_hmac.Sign(secret_key_.get(), secret_key_len_, pseudo_random_key.get(), digest_size_);
    if (!result)
        return false;

    /**
     * Step 2. Expand: OUTPUT = HKDF-Expand(PRK, info)
     * https://tools.ietf.org/html/rfc5869#section-2.3
     */
    const size_t num_blocks = (output_len + digest_size_ - 1) / digest_size_;
    if (num_blocks >= 256u)
        return false;

    UniquePtr<uint8_t[]> buf(new uint8_t[digest_size_ + info_len + 1]);
    UniquePtr<uint8_t[]> digest(new uint8_t[digest_size_]);
    if (buf.get() == nullptr || digest.get() == nullptr)
        return false;
    HmacSha256 hmac;
    result = hmac.Init(pseudo_random_key.get(), digest_size_);
    if (!result)
        return false;

    for (size_t i = 0; i < num_blocks; i++) {
        size_t block_input_len = 0;
        if (i != 0) {
            memcpy(buf.get(), digest.get(), digest_size_);
            block_input_len = digest_size_;
        }
        if (info != nullptr && info_len > 0)
            memcpy(buf.get() + block_input_len, info, info_len);
        block_input_len += info_len;
        *(buf.get() + block_input_len++) = static_cast<uint8_t>(i + 1);
        result = hmac.Sign(buf.get(), block_input_len, digest.get(), digest_size_);
        if (!result)
            return false;
        size_t block_output_len = digest_size_ < output_len - i * digest_size_
                                      ? digest_size_
                                      : output_len - i * digest_size_;
        memcpy(output + i * digest_size_, digest.get(), block_output_len);
    }
    return true;
}

}  // namespace keymaster

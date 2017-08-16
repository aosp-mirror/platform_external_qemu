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

#include "iso18033kdf.h"
#include "openssl_utils.h"

#include <algorithm>

#include <openssl/evp.h>

namespace keymaster {

inline size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

bool Iso18033Kdf::GenerateKey(const uint8_t* info, size_t info_len, uint8_t* output,
                              size_t output_len) {
    if (!is_initialized_ || output == nullptr)
        return false;

    /* Check whether output length is too long as specified in ISO/IEC 18033-2. */
    if ((0xFFFFFFFFULL + start_counter_) * digest_size_ < (uint64_t)output_len)
        return false;

    EVP_MD_CTX ctx;
    EvpMdCtxCleaner ctxCleaner(&ctx);
    EVP_MD_CTX_init(&ctx);

    size_t num_blocks = (output_len + digest_size_ - 1) / digest_size_;
    UniquePtr<uint8_t[]> counter(new uint8_t[4]);
    UniquePtr<uint8_t[]> digest_result(new uint8_t[digest_size_]);
    if (counter.get() == nullptr || digest_result.get() == nullptr)
        return false;
    for (size_t block = 0; block < num_blocks; block++) {
        switch (digest_type_) {
        case KM_DIGEST_SHA1:
            if (!EVP_DigestInit_ex(&ctx, EVP_sha1(), nullptr /* default digest */))
                return false;
            break;
        case KM_DIGEST_SHA_2_256:
            if (!EVP_DigestInit_ex(&ctx, EVP_sha256(), nullptr /* default digest */))
                return false;
            break;
        default:
            return false;
        }

        if (!EVP_DigestUpdate(&ctx, secret_key_.get(), secret_key_len_) ||
            !Uint32ToBigEndianByteArray(block + start_counter_, counter.get()) ||
            !EVP_DigestUpdate(&ctx, counter.get(), 4))
            return false;

        if (info != nullptr && info_len > 0) {
            if (!EVP_DigestUpdate(&ctx, info, info_len))
                return false;
        }

        /* OpenSSL does not accept size_t parameter. */
        uint32_t uint32_digest_size_ = digest_size_;
        if (!EVP_DigestFinal_ex(&ctx, digest_result.get(), &uint32_digest_size_) ||
            uint32_digest_size_ != digest_size_)
            return false;

        size_t block_start = digest_size_ * block;
        size_t block_length = min(digest_size_, output_len - block_start);
        memcpy(output + block_start, digest_result.get(), block_length);
    }
    return true;
}

}  // namespace keymaster

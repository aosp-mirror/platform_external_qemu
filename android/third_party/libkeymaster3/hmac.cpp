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

#include "hmac.h"

#include <assert.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/mem.h>
#include <openssl/sha.h>

#include <keymaster/android_keymaster_utils.h>

namespace keymaster {

size_t HmacSha256::DigestLength() const {
    return SHA256_DIGEST_LENGTH;
}

bool HmacSha256::Init(const Buffer& key) {
    return Init(key.peek_read(), key.available_read());
}

bool HmacSha256::Init(const uint8_t* key, size_t key_len) {
    if (!key)
        return false;

    key_len_ = key_len;
    key_.reset(dup_buffer(key, key_len));
    if (!key_.get()) {
        return false;
    }
    return true;
}

bool HmacSha256::Sign(const Buffer& data, uint8_t* out_digest, size_t digest_len) const {
    return Sign(data.peek_read(), data.available_read(), out_digest, digest_len);
}

bool HmacSha256::Sign(const uint8_t* data, size_t data_len, uint8_t* out_digest,
                      size_t digest_len) const {
    assert(digest_len);

    uint8_t tmp[SHA256_DIGEST_LENGTH];
    uint8_t* digest = tmp;
    if (digest_len >= SHA256_DIGEST_LENGTH)
        digest = out_digest;

    if (nullptr == ::HMAC(EVP_sha256(), key_.get(), key_len_, data, data_len, digest, nullptr)) {
        return false;
    }
    if (digest_len < SHA256_DIGEST_LENGTH)
        memcpy(out_digest, tmp, digest_len);

    return true;
}

bool HmacSha256::Verify(const Buffer& data, const Buffer& digest) const {
    return Verify(data.peek_read(), data.available_read(), digest.peek_read(),
                  digest.available_read());
}

bool HmacSha256::Verify(const uint8_t* data, size_t data_len, const uint8_t* digest,
                        size_t digest_len) const {
    if (digest_len != SHA256_DIGEST_LENGTH)
        return false;

    uint8_t computed_digest[SHA256_DIGEST_LENGTH];
    if (!Sign(data, data_len, computed_digest, sizeof(computed_digest)))
        return false;

    return 0 == CRYPTO_memcmp(digest, computed_digest, SHA256_DIGEST_LENGTH);
}

}  // namespace keymaster

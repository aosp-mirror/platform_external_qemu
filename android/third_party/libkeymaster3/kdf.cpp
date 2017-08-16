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

namespace keymaster {

Kdf::Kdf() : is_initialized_(false) {}

bool Kdf::Init(keymaster_digest_t digest_type, const uint8_t* secret, size_t secret_len,
               const uint8_t* salt, size_t salt_len) {
    is_initialized_ = false;

    switch (digest_type) {
    case KM_DIGEST_SHA1:
        digest_size_ = 20;
        digest_type_ = digest_type;
        break;
    case KM_DIGEST_SHA_2_256:
        digest_size_ = 32;
        digest_type_ = digest_type;
        break;
    default:
        return false;
    }

    if (!secret || secret_len == 0)
        return false;

    secret_key_len_ = secret_len;
    secret_key_.reset(dup_buffer(secret, secret_len));
    if (!secret_key_.get())
        return false;

    salt_len_ = salt_len;
    if (salt && salt_len > 0) {
        salt_.reset(dup_buffer(salt, salt_len));
        if (!salt_.get())
            return false;
    } else {
        salt_.reset();
    }

    is_initialized_ = true;
    return true;
}

bool Kdf::Uint32ToBigEndianByteArray(uint32_t number, uint8_t* output) {
    if (!output)
        return false;

    output[0] = (number >> 24) & 0xff;
    output[1] = (number >> 16) & 0xff;
    output[2] = (number >> 8) & 0xff;
    output[3] = (number)&0xff;
    return true;
}

}  // namespace keymaster

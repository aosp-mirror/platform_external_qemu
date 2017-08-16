/*
 * Copyright 2014 The Android Open Source Project
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

#include "hmac_key.h"

#include <new>

#include <openssl/err.h>
#include <openssl/rand.h>

#include "hmac_operation.h"

namespace keymaster {

static HmacSignOperationFactory sign_factory;
static HmacVerifyOperationFactory verify_factory;

OperationFactory* HmacKeyFactory::GetOperationFactory(keymaster_purpose_t purpose) const {
    switch (purpose) {
    case KM_PURPOSE_SIGN:
        return &sign_factory;
    case KM_PURPOSE_VERIFY:
        return &verify_factory;
    default:
        return nullptr;
    }
}

keymaster_error_t HmacKeyFactory::LoadKey(const KeymasterKeyBlob& key_material,
                                          const AuthorizationSet& /* additional_params */,
                                          const AuthorizationSet& hw_enforced,
                                          const AuthorizationSet& sw_enforced,
                                          UniquePtr<Key>* key) const {
    if (!key)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    uint32_t min_mac_length;
    if (!hw_enforced.GetTagValue(TAG_MIN_MAC_LENGTH, &min_mac_length) &&
        !sw_enforced.GetTagValue(TAG_MIN_MAC_LENGTH, &min_mac_length)) {
        LOG_E("HMAC key must have KM_TAG_MIN_MAC_LENGTH", 0);
        return KM_ERROR_INVALID_KEY_BLOB;
    }

    keymaster_error_t error;
    key->reset(new (std::nothrow) HmacKey(key_material, hw_enforced, sw_enforced, &error));
    if (!key->get())
        error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return error;
}

keymaster_error_t HmacKeyFactory::validate_algorithm_specific_new_key_params(
    const AuthorizationSet& key_description) const {
    uint32_t min_mac_length_bits;
    if (!key_description.GetTagValue(TAG_MIN_MAC_LENGTH, &min_mac_length_bits))
        return KM_ERROR_MISSING_MIN_MAC_LENGTH;

    keymaster_digest_t digest;
    if (!key_description.GetTagValue(TAG_DIGEST, &digest)) {
        LOG_E("%d digests specified for HMAC key", key_description.GetTagCount(TAG_DIGEST));
        return KM_ERROR_UNSUPPORTED_DIGEST;
    }

    size_t hash_size_bits = 0;
    switch (digest) {
    case KM_DIGEST_NONE:
        return KM_ERROR_UNSUPPORTED_DIGEST;
    case KM_DIGEST_MD5:
        hash_size_bits = 128;
        break;
    case KM_DIGEST_SHA1:
        hash_size_bits = 160;
        break;
    case KM_DIGEST_SHA_2_224:
        hash_size_bits = 224;
        break;
    case KM_DIGEST_SHA_2_256:
        hash_size_bits = 256;
        break;
    case KM_DIGEST_SHA_2_384:
        hash_size_bits = 384;
        break;
    case KM_DIGEST_SHA_2_512:
        hash_size_bits = 512;
        break;
    };

    if (hash_size_bits == 0) {
        // digest was not matched
        return KM_ERROR_UNSUPPORTED_DIGEST;
    }

    if (min_mac_length_bits % 8 != 0 || min_mac_length_bits > hash_size_bits)
        return KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH;

    if (min_mac_length_bits < kMinHmacLengthBits)
        return KM_ERROR_UNSUPPORTED_MIN_MAC_LENGTH;

    return KM_ERROR_OK;
}

}  // namespace keymaster

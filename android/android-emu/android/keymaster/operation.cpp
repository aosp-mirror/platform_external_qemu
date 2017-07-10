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

#include "operation.h"

#include <keymaster/authorization_set.h>

#include "key.h"

namespace keymaster {

bool OperationFactory::supported(keymaster_padding_t padding) const {
    size_t padding_count;
    const keymaster_padding_t* supported_paddings = SupportedPaddingModes(&padding_count);
    for (size_t i = 0; i < padding_count; ++i)
        if (padding == supported_paddings[i])
            return true;
    return false;
}

bool OperationFactory::supported(keymaster_block_mode_t block_mode) const {
    size_t block_mode_count;
    const keymaster_block_mode_t* supported_block_modes = SupportedBlockModes(&block_mode_count);
    for (size_t i = 0; i < block_mode_count; ++i)
        if (block_mode == supported_block_modes[i])
            return true;
    return false;
}

bool OperationFactory::supported(keymaster_digest_t digest) const {
    size_t digest_count;
    const keymaster_digest_t* supported_digests = SupportedDigests(&digest_count);
    for (size_t i = 0; i < digest_count; ++i)
        if (digest == supported_digests[i])
            return true;
    return false;
}

inline bool is_public_key_algorithm(keymaster_algorithm_t algorithm) {
    switch (algorithm) {
    case KM_ALGORITHM_HMAC:
    case KM_ALGORITHM_AES:
        return false;
    case KM_ALGORITHM_RSA:
    case KM_ALGORITHM_EC:
        return true;
    }

    // Unreachable.
    assert(false);
    return false;
}

bool OperationFactory::is_public_key_operation() const {
    KeyType key_type = registry_key();

    if (!is_public_key_algorithm(key_type.algorithm))
        return false;

    switch (key_type.purpose) {
    case KM_PURPOSE_VERIFY:
    case KM_PURPOSE_ENCRYPT:
        return true;
    case KM_PURPOSE_SIGN:
    case KM_PURPOSE_DECRYPT:
    case KM_PURPOSE_DERIVE_KEY:
        return false;
    };

    // Unreachable.
    assert(false);
    return false;
}

bool OperationFactory::GetAndValidatePadding(const AuthorizationSet& begin_params, const Key& key,
                                             keymaster_padding_t* padding,
                                             keymaster_error_t* error) const {
    *error = KM_ERROR_UNSUPPORTED_PADDING_MODE;
    if (!begin_params.GetTagValue(TAG_PADDING, padding)) {
        LOG_E("%d padding modes specified in begin params", begin_params.GetTagCount(TAG_PADDING));
        return false;
    } else if (!supported(*padding)) {
        LOG_E("Padding mode %d not supported", *padding);
        return false;
    } else if (
        // If it's a public key operation, all padding modes are authorized.
        !is_public_key_operation() &&
        // Otherwise the key needs to authorize the specific mode.
        !key.authorizations().Contains(TAG_PADDING, *padding) &&
        !key.authorizations().Contains(TAG_PADDING_OLD, *padding)) {
        LOG_E("Padding mode %d was specified, but not authorized by key", *padding);
        *error = KM_ERROR_INCOMPATIBLE_PADDING_MODE;
        return false;
    }

    *error = KM_ERROR_OK;
    return true;
}

bool OperationFactory::GetAndValidateDigest(const AuthorizationSet& begin_params, const Key& key,
                                            keymaster_digest_t* digest,
                                            keymaster_error_t* error) const {
    *error = KM_ERROR_UNSUPPORTED_DIGEST;
    if (!begin_params.GetTagValue(TAG_DIGEST, digest)) {
        LOG_E("%d digests specified in begin params", begin_params.GetTagCount(TAG_DIGEST));
        return false;
    } else if (!supported(*digest)) {
        LOG_E("Digest %d not supported", *digest);
        return false;
    } else if (
        // If it's a public key operation, all digests are authorized.
        !is_public_key_operation() &&
        // Otherwise the key needs to authorize the specific digest.
        !key.authorizations().Contains(TAG_DIGEST, *digest) &&
        !key.authorizations().Contains(TAG_DIGEST_OLD, *digest)) {
        LOG_E("Digest %d was specified, but not authorized by key", *digest);
        *error = KM_ERROR_INCOMPATIBLE_DIGEST;
        return false;
    }
    *error = KM_ERROR_OK;
    return true;
}

keymaster_error_t Operation::UpdateForFinish(const AuthorizationSet& input_params,
                                             const Buffer& input) {
    if (!input_params.empty() || input.available_read()) {
        size_t input_consumed;
        Buffer output;
        AuthorizationSet output_params;
        keymaster_error_t error =
            Update(input_params, input, &output_params, &output, &input_consumed);
        if (error != KM_ERROR_OK)
            return error;
        assert(input_consumed == input.available_read());
        assert(output_params.empty());
        assert(output.available_read() == 0);
    }

    return KM_ERROR_OK;
}

}  // namespace keymaster

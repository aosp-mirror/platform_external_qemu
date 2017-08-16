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

#include "trusty_keymaster_enforcement.h"

#include <openssl/hmac.h>


#include <hardware/hw_auth_token.h>
#include <keymaster/android_keymaster_utils.h>

#include "openssl_err.h"
#include "trusty_keymaster_context.h"

namespace keymaster {

bool TrustyKeymasterEnforcement::auth_token_timed_out(const hw_auth_token_t& token,
                                                      uint32_t timeout_seconds) const {
    uint64_t token_timestamp_millis = ntoh(token.timestamp);
    uint64_t timeout_millis = static_cast<uint64_t>(timeout_seconds) * 1000;
    uint64_t millis_since_boot = milliseconds_since_boot();
    return (millis_since_boot >= token_timestamp_millis &&
            (millis_since_boot - token_timestamp_millis) > timeout_millis);
}

uint32_t TrustyKeymasterEnforcement::get_current_time() const {
    return milliseconds_since_boot() / 1000;
}

inline size_t min(size_t a, size_t b) {
    return a < b ? a : b;
}

bool TrustyKeymasterEnforcement::ValidateTokenSignature(const hw_auth_token_t& token) const {
    keymaster_key_blob_t auth_token_key;
    keymaster_error_t error = context_->GetAuthTokenKey(&auth_token_key);
    if (error != KM_ERROR_OK)
        return false;

    // Signature covers entire token except HMAC field.
    const uint8_t* hash_data = reinterpret_cast<const uint8_t*>(&token);
    size_t hash_data_length = reinterpret_cast<const uint8_t*>(&token.hmac) - hash_data;

    uint8_t computed_hash[EVP_MAX_MD_SIZE];
    unsigned int computed_hash_length;
    if (!HMAC(EVP_sha256(), auth_token_key.key_material, auth_token_key.key_material_size,
              hash_data, hash_data_length, computed_hash, &computed_hash_length)) {
        LOG_S("Error %d computing token signature", TranslateLastOpenSslError());
        return false;
    }

    return 0 == memcmp_s(computed_hash, token.hmac, min(sizeof(token.hmac), computed_hash_length));
}

uint64_t TrustyKeymasterEnforcement::milliseconds_since_boot() const {
    LOG_D("Error getting time is not implemtned", 0);
    return static_cast<uint64_t>(0) / 1000 / 1000;
}

}  // namespace keymaster

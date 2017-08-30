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

#include "rsa_key.h"

#include <keymaster/keymaster_context.h>

#include "openssl_err.h"
#include "openssl_utils.h"
#include "rsa_operation.h"

namespace keymaster {

bool RsaKey::EvpToInternal(const EVP_PKEY* pkey) {
    rsa_key_.reset(EVP_PKEY_get1_RSA(const_cast<EVP_PKEY*>(pkey)));
    return rsa_key_.get() != NULL;
}

bool RsaKey::InternalToEvp(EVP_PKEY* pkey) const {
    return EVP_PKEY_set1_RSA(pkey, rsa_key_.get()) == 1;
}

bool RsaKey::SupportedMode(keymaster_purpose_t purpose, keymaster_padding_t padding) {
    switch (purpose) {
    case KM_PURPOSE_SIGN:
    case KM_PURPOSE_VERIFY:
        return padding == KM_PAD_NONE || padding == KM_PAD_RSA_PSS ||
               padding == KM_PAD_RSA_PKCS1_1_5_SIGN;

    case KM_PURPOSE_ENCRYPT:
    case KM_PURPOSE_DECRYPT:
        return padding == KM_PAD_RSA_OAEP || padding == KM_PAD_RSA_PKCS1_1_5_ENCRYPT;

    case KM_PURPOSE_DERIVE_KEY:
        return false;
    };
    return false;
}

bool RsaKey::SupportedMode(keymaster_purpose_t purpose, keymaster_digest_t digest) {
    switch (purpose) {
    case KM_PURPOSE_SIGN:
    case KM_PURPOSE_VERIFY:
        return digest == KM_DIGEST_NONE || digest == KM_DIGEST_SHA_2_256;

    case KM_PURPOSE_ENCRYPT:
    case KM_PURPOSE_DECRYPT:
        /* Don't care */
        break;

    case KM_PURPOSE_DERIVE_KEY:
        return false;
    };
    return true;
}

}  // namespace keymaster

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

#include "trusty_keymaster_context.h"

extern "C" {
#include <lib/rng/trusty_rng.h>
}

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/ec_key_factory.h>
#include <keymaster/logger.h>
#include <keymaster/rsa_key_factory.h>

#include "aes_key.h"
#include "auth_encrypted_key_blob.h"
#include "hmac_key.h"
#include "ocb_utils.h"

namespace keymaster {

namespace {
static const int kCallsBetweenRngReseeds = 32;
static const int kRngReseedSize = 64;
}  // anonymous namespace

TrustyKeymasterContext::TrustyKeymasterContext()
    : enforcement_policy_(this), rng_initialized_(false), calls_since_reseed_(0) {
    LOG_D("Creating TrustyKeymaster", 0);
    rsa_factory_.reset(new RsaKeyFactory(this));
    ec_factory_.reset(new EcKeyFactory(this));
    aes_factory_.reset(new AesKeyFactory(this));
    hmac_factory_.reset(new HmacKeyFactory(this));
}

KeyFactory* TrustyKeymasterContext::GetKeyFactory(keymaster_algorithm_t algorithm) const {
    switch (algorithm) {
    case KM_ALGORITHM_RSA:
        return rsa_factory_.get();
    case KM_ALGORITHM_EC:
        return ec_factory_.get();
    case KM_ALGORITHM_AES:
        return aes_factory_.get();
    case KM_ALGORITHM_HMAC:
        return hmac_factory_.get();
    default:
        return nullptr;
    }
}

static keymaster_algorithm_t supported_algorithms[] = {KM_ALGORITHM_RSA, KM_ALGORITHM_EC,
                                                       KM_ALGORITHM_AES, KM_ALGORITHM_HMAC};

keymaster_algorithm_t*
TrustyKeymasterContext::GetSupportedAlgorithms(size_t* algorithms_count) const {
    *algorithms_count = array_length(supported_algorithms);
    return supported_algorithms;
}

OperationFactory* TrustyKeymasterContext::GetOperationFactory(keymaster_algorithm_t algorithm,
                                                              keymaster_purpose_t purpose) const {
    KeyFactory* key_factory = GetKeyFactory(algorithm);
    if (!key_factory)
        return nullptr;
    return key_factory->GetOperationFactory(purpose);
}

static keymaster_error_t TranslateAuthorizationSetError(AuthorizationSet::Error err) {
    switch (err) {
    case AuthorizationSet::OK:
        return KM_ERROR_OK;
    case AuthorizationSet::ALLOCATION_FAILURE:
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    case AuthorizationSet::MALFORMED_DATA:
        return KM_ERROR_UNKNOWN_ERROR;
    }
    return KM_ERROR_OK;
}

static keymaster_error_t SetAuthorizations(const AuthorizationSet& key_description,
                                           keymaster_key_origin_t origin, uint32_t os_version,
                                           uint32_t os_patchlevel,
                                           AuthorizationSet* hw_enforced,
                                           AuthorizationSet* sw_enforced) {
    sw_enforced->Clear();
    hw_enforced->Clear();

    LOG_D("SetAuthorizations", 0);
    for (auto& entry : key_description) {

        switch (entry.tag) {
        case KM_TAG_INVALID:
        case KM_TAG_BOOTLOADER_ONLY:
        case KM_TAG_NONCE:
        case KM_TAG_AUTH_TOKEN:
        case KM_TAG_MAC_LENGTH:
        case KM_TAG_ASSOCIATED_DATA:
        case KM_TAG_UNIQUE_ID:
            return KM_ERROR_INVALID_KEY_BLOB;

        case KM_TAG_ROLLBACK_RESISTANT:
        case KM_TAG_APPLICATION_ID:
        case KM_TAG_APPLICATION_DATA:
        case KM_TAG_ALL_APPLICATIONS:
        case KM_TAG_ROOT_OF_TRUST:
        case KM_TAG_ORIGIN:
        case KM_TAG_RESET_SINCE_ID_ROTATION:
        case KM_TAG_ALLOW_WHILE_ON_BODY:
        case KM_TAG_ATTESTATION_CHALLENGE:
            // Ignore these.
            break;

        case KM_TAG_PURPOSE:
        case KM_TAG_ALGORITHM:
        case KM_TAG_KEY_SIZE:
        case KM_TAG_RSA_PUBLIC_EXPONENT:
        case KM_TAG_BLOB_USAGE_REQUIREMENTS:
        case KM_TAG_DIGEST:
        case KM_TAG_PADDING:
        case KM_TAG_BLOCK_MODE:
        case KM_TAG_MIN_SECONDS_BETWEEN_OPS:
        case KM_TAG_MAX_USES_PER_BOOT:
        case KM_TAG_USER_SECURE_ID:
        case KM_TAG_NO_AUTH_REQUIRED:
        case KM_TAG_AUTH_TIMEOUT:
        case KM_TAG_CALLER_NONCE:
        case KM_TAG_MIN_MAC_LENGTH:
        case KM_TAG_KDF:
        case KM_TAG_EC_CURVE:
        case KM_TAG_ECIES_SINGLE_HASH_MODE:
            hw_enforced->push_back(entry);
            break;

        case KM_TAG_USER_AUTH_TYPE:
            if (entry.enumerated == HW_AUTH_PASSWORD)
                hw_enforced->push_back(entry);
            else
                sw_enforced->push_back(entry);
            break;

        case KM_TAG_ACTIVE_DATETIME:
        case KM_TAG_ORIGINATION_EXPIRE_DATETIME:
        case KM_TAG_USAGE_EXPIRE_DATETIME:
        case KM_TAG_USER_ID:
        case KM_TAG_ALL_USERS:
        case KM_TAG_CREATION_DATETIME:
        case KM_TAG_INCLUDE_UNIQUE_ID:
        case KM_TAG_EXPORTABLE:
        case KM_TAG_OS_VERSION:
        case KM_TAG_OS_PATCHLEVEL:

            sw_enforced->push_back(entry);
            break;
        default:
            LOG_I("not handled case here !!! %s at %d", __func__, __LINE__);
            break;
        }
    }

    hw_enforced->push_back(TAG_ORIGIN, origin);


    sw_enforced->push_back(TAG_OS_VERSION, os_version);
    sw_enforced->push_back(TAG_OS_PATCHLEVEL, os_patchlevel);

    if (sw_enforced->is_valid() != AuthorizationSet::OK)
        return TranslateAuthorizationSetError(sw_enforced->is_valid());
    if (hw_enforced->is_valid() != AuthorizationSet::OK)
        return TranslateAuthorizationSetError(hw_enforced->is_valid());
    return KM_ERROR_OK;
}

static keymaster_error_t BuildHiddenAuthorizations(const AuthorizationSet& input_set,
                                                   AuthorizationSet* hidden) {
    keymaster_blob_t entry;
    if (input_set.GetTagValue(TAG_APPLICATION_ID, &entry))
        hidden->push_back(TAG_APPLICATION_ID, entry.data, entry.data_length);
    if (input_set.GetTagValue(TAG_APPLICATION_DATA, &entry))
        hidden->push_back(TAG_APPLICATION_DATA, entry.data, entry.data_length);

    keymaster_key_param_t root_of_trust;
    root_of_trust.tag = KM_TAG_ROOT_OF_TRUST;
    root_of_trust.blob.data = reinterpret_cast<const uint8_t*>("Unbound");
    root_of_trust.blob.data_length = 7;
    hidden->push_back(root_of_trust);

    return TranslateAuthorizationSetError(hidden->is_valid());
}

keymaster_error_t TrustyKeymasterContext::CreateKeyBlob(const AuthorizationSet& key_description,
                                                        keymaster_key_origin_t origin,
                                                        const KeymasterKeyBlob& key_material,
                                                        KeymasterKeyBlob* blob,
                                                        AuthorizationSet* hw_enforced,
                                                        AuthorizationSet* sw_enforced) const {
    uint32_t os_version=80000;
    uint32_t os_patchlevel=201709;
    keymaster_error_t error = SetAuthorizations(key_description, origin, os_version, os_patchlevel, hw_enforced, sw_enforced);
    if (error != KM_ERROR_OK)
        return error;

    AuthorizationSet hidden;
    LOG_I("calling %s at %d", __func__, __LINE__);
    error = BuildHiddenAuthorizations(key_description, &hidden);
    LOG_I("calling %s at %d", __func__, __LINE__);
    if (error != KM_ERROR_OK)
        return error;

    KeymasterKeyBlob master_key;
    LOG_I("calling %s at %d", __func__, __LINE__);
    error = DeriveMasterKey(&master_key);
    LOG_I("calling %s at %d", __func__, __LINE__);
    if (error != KM_ERROR_OK)
        return error;

    Buffer nonce(OCB_NONCE_LENGTH);
    Buffer tag(OCB_TAG_LENGTH);
    if (!nonce.peek_write() || !tag.peek_write())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    LOG_I("calling %s at %d", __func__, __LINE__);
    error = GenerateRandom(nonce.peek_write(), OCB_NONCE_LENGTH);
    LOG_I("calling %s at %d", __func__, __LINE__);
    if (error != KM_ERROR_OK)
        return error;
    nonce.advance_write(OCB_NONCE_LENGTH);

    KeymasterKeyBlob encrypted_key;
    LOG_I("calling %s at %d", __func__, __LINE__);
    error = OcbEncryptKey(*hw_enforced, *sw_enforced, hidden, master_key, key_material, nonce,
                          &encrypted_key, &tag);
    LOG_I("calling %s at %d", __func__, __LINE__);

    return SerializeAuthEncryptedBlob(encrypted_key, *hw_enforced, *sw_enforced, nonce, tag, blob);
}

keymaster_error_t TrustyKeymasterContext::ParseKeyBlob(const KeymasterKeyBlob& blob,
                                                       const AuthorizationSet& additional_params,
                                                       KeymasterKeyBlob* key_material,
                                                       AuthorizationSet* hw_enforced,
                                                       AuthorizationSet* sw_enforced) const {
    Buffer nonce, tag;
    KeymasterKeyBlob encrypted_key_material;
    keymaster_error_t error = DeserializeAuthEncryptedBlob(blob, &encrypted_key_material,
                                                           hw_enforced, sw_enforced, &nonce, &tag);
    if (error != KM_ERROR_OK)
        return error;

    if (nonce.available_read() != OCB_NONCE_LENGTH || tag.available_read() != OCB_TAG_LENGTH)
        return KM_ERROR_INVALID_KEY_BLOB;

    KeymasterKeyBlob master_key;
    error = DeriveMasterKey(&master_key);
    if (error != KM_ERROR_OK)
        return error;

    AuthorizationSet hidden;
    error = BuildHiddenAuthorizations(additional_params, &hidden);
    if (error != KM_ERROR_OK)
        return error;

    return OcbDecryptKey(*hw_enforced, *sw_enforced, hidden, master_key, encrypted_key_material,
                         nonce, tag, key_material);
}

keymaster_error_t TrustyKeymasterContext::AddRngEntropy(const uint8_t* buf, size_t length) const {
    if (trusty_rng_add_entropy(buf, length) != 0)
        return KM_ERROR_UNKNOWN_ERROR;
    return KM_ERROR_OK;
}

bool TrustyKeymasterContext::SeedRngIfNeeded() const {
    if (ShouldReseedRng())
        const_cast<TrustyKeymasterContext*>(this)->ReseedRng();
    return rng_initialized_;
}

bool TrustyKeymasterContext::ShouldReseedRng() const {
    if (!rng_initialized_) {
        LOG_I("RNG not initalized, reseed", 0);
        return true;
    }

    if (++calls_since_reseed_ % kCallsBetweenRngReseeds == 0) {
        LOG_I("Periodic reseed", 0);
        return true;
    }
    return false;
}

bool TrustyKeymasterContext::ReseedRng() {
    UniquePtr<uint8_t[]> rand_seed(new uint8_t[kRngReseedSize]);
    memset(rand_seed.get(), 0, kRngReseedSize);
    if (trusty_rng_hw_rand(rand_seed.get(), kRngReseedSize) != 0) {
        LOG_E("Failed to get bytes from HW RNG", 0);
        return false;
    }
    LOG_I("Reseeding with %d bytes from HW RNG", kRngReseedSize);
    trusty_rng_add_entropy(rand_seed.get(), kRngReseedSize);

    rng_initialized_ = true;
    return true;
}

keymaster_error_t TrustyKeymasterContext::GenerateRandom(uint8_t* buf, size_t length) const {
    if (!SeedRngIfNeeded() || trusty_rng_secure_rand(buf, length) != 0)
        return KM_ERROR_UNKNOWN_ERROR;
    return KM_ERROR_OK;
}

// Gee wouldn't it be nice if the crypto service headers defined this.
enum DerivationParams {
    DERIVATION_DATA_PARAM = 0,
    OUTPUT_BUFFER_PARAM = 1,
};

keymaster_error_t TrustyKeymasterContext::DeriveMasterKey(KeymasterKeyBlob* master_key) const {
    LOG_D("TODO: Deriving master key not implemented", 0);

    return KM_ERROR_OK;
}

bool TrustyKeymasterContext::InitializeAuthTokenKey() {
    if (GenerateRandom(auth_token_key_, kAuthTokenKeySize) != KM_ERROR_OK)
        return false;
    auth_token_key_initialized_ = true;
    return auth_token_key_initialized_;
}

keymaster_error_t TrustyKeymasterContext::GetAuthTokenKey(keymaster_key_blob_t* key) const {
    if (!auth_token_key_initialized_ &&
        !const_cast<TrustyKeymasterContext*>(this)->InitializeAuthTokenKey())
        return KM_ERROR_UNKNOWN_ERROR;

    key->key_material = auth_token_key_;
    key->key_material_size = kAuthTokenKeySize;
    return KM_ERROR_OK;
}

}  // namespace keymaster

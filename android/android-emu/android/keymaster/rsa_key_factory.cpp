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

#include <keymaster/rsa_key_factory.h>

#include <new>

#include <keymaster/keymaster_context.h>

#include "openssl_err.h"
#include "openssl_utils.h"
#include "rsa_key.h"
#include "rsa_operation.h"

namespace keymaster {

const int kMaximumRsaKeySize = 16 * 1024;  // 16kbits should be enough for anyone.

static RsaSigningOperationFactory sign_factory;
static RsaVerificationOperationFactory verify_factory;
static RsaEncryptionOperationFactory encrypt_factory;
static RsaDecryptionOperationFactory decrypt_factory;

OperationFactory* RsaKeyFactory::GetOperationFactory(keymaster_purpose_t purpose) const {
    switch (purpose) {
    case KM_PURPOSE_SIGN:
        return &sign_factory;
    case KM_PURPOSE_VERIFY:
        return &verify_factory;
    case KM_PURPOSE_ENCRYPT:
        return &encrypt_factory;
    case KM_PURPOSE_DECRYPT:
        return &decrypt_factory;
    default:
        return nullptr;
    }
}

keymaster_error_t RsaKeyFactory::GenerateKey(const AuthorizationSet& key_description,
                                             KeymasterKeyBlob* key_blob,
                                             AuthorizationSet* hw_enforced,
                                             AuthorizationSet* sw_enforced) const {
    if (!key_blob || !hw_enforced || !sw_enforced)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    const AuthorizationSet& authorizations(key_description);

    uint64_t public_exponent;
    if (!authorizations.GetTagValue(TAG_RSA_PUBLIC_EXPONENT, &public_exponent)) {
        LOG_E("%s", "No public exponent specified for RSA key generation");
        return KM_ERROR_INVALID_ARGUMENT;
    }

    uint32_t key_size;
    if (!authorizations.GetTagValue(TAG_KEY_SIZE, &key_size)) {
        LOG_E("No key size specified for RSA key generation", 0);
        return KM_ERROR_UNSUPPORTED_KEY_SIZE;
    }
    if (key_size % 8 != 0 || key_size > kMaximumRsaKeySize) {
        LOG_E("Invalid key size of %u bits specified for RSA key generation", key_size);
        return KM_ERROR_UNSUPPORTED_KEY_SIZE;
    }

    UniquePtr<BIGNUM, BIGNUM_Delete> exponent(BN_new());
    UniquePtr<RSA, RsaKey::RSA_Delete> rsa_key(RSA_new());
    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey(EVP_PKEY_new());
    if (exponent.get() == NULL || rsa_key.get() == NULL || pkey.get() == NULL)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    if (!BN_set_word(exponent.get(), public_exponent) ||
        !RSA_generate_key_ex(rsa_key.get(), key_size, exponent.get(), NULL /* callback */))
        return TranslateLastOpenSslError();

    if (EVP_PKEY_set1_RSA(pkey.get(), rsa_key.get()) != 1)
        return TranslateLastOpenSslError();

    KeymasterKeyBlob key_material;
    keymaster_error_t error = EvpKeyToKeyMaterial(pkey.get(), &key_material);
    if (error != KM_ERROR_OK)
        return error;

    return context_->CreateKeyBlob(authorizations, KM_ORIGIN_GENERATED, key_material, key_blob,
                                   hw_enforced, sw_enforced);
}

keymaster_error_t RsaKeyFactory::ImportKey(const AuthorizationSet& key_description,
                                           keymaster_key_format_t input_key_material_format,
                                           const KeymasterKeyBlob& input_key_material,
                                           KeymasterKeyBlob* output_key_blob,
                                           AuthorizationSet* hw_enforced,
                                           AuthorizationSet* sw_enforced) const {
    if (!output_key_blob || !hw_enforced || !sw_enforced)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    AuthorizationSet authorizations;
    uint64_t public_exponent;
    uint32_t key_size;
    keymaster_error_t error =
        UpdateImportKeyDescription(key_description, input_key_material_format, input_key_material,
                                   &authorizations, &public_exponent, &key_size);
    if (error != KM_ERROR_OK)
        return error;
    return context_->CreateKeyBlob(authorizations, KM_ORIGIN_IMPORTED, input_key_material,
                                   output_key_blob, hw_enforced, sw_enforced);
}

keymaster_error_t RsaKeyFactory::UpdateImportKeyDescription(const AuthorizationSet& key_description,
                                                            keymaster_key_format_t key_format,
                                                            const KeymasterKeyBlob& key_material,
                                                            AuthorizationSet* updated_description,
                                                            uint64_t* public_exponent,
                                                            uint32_t* key_size) const {
    if (!updated_description || !public_exponent || !key_size)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey;
    keymaster_error_t error =
        KeyMaterialToEvpKey(key_format, key_material, keymaster_key_type(), &pkey);
    if (error != KM_ERROR_OK)
        return error;

    UniquePtr<RSA, RsaKey::RSA_Delete> rsa_key(EVP_PKEY_get1_RSA(pkey.get()));
    if (!rsa_key.get())
        return TranslateLastOpenSslError();

    updated_description->Reinitialize(key_description);

    *public_exponent = BN_get_word(rsa_key->e);
    if (*public_exponent == 0xffffffffL)
        return KM_ERROR_INVALID_KEY_BLOB;
    if (!updated_description->GetTagValue(TAG_RSA_PUBLIC_EXPONENT, public_exponent))
        updated_description->push_back(TAG_RSA_PUBLIC_EXPONENT, *public_exponent);
    if (*public_exponent != BN_get_word(rsa_key->e)) {
        LOG_E("Imported public exponent (%u) does not match specified public exponent (%u)",
              *public_exponent, BN_get_word(rsa_key->e));
        return KM_ERROR_IMPORT_PARAMETER_MISMATCH;
    }

    *key_size = RSA_size(rsa_key.get()) * 8;
    if (!updated_description->GetTagValue(TAG_KEY_SIZE, key_size))
        updated_description->push_back(TAG_KEY_SIZE, *key_size);
    if (RSA_size(rsa_key.get()) * 8 != *key_size) {
        LOG_E("Imported key size (%u bits) does not match specified key size (%u bits)",
              RSA_size(rsa_key.get()) * 8, *key_size);
        return KM_ERROR_IMPORT_PARAMETER_MISMATCH;
    }

    keymaster_algorithm_t algorithm = KM_ALGORITHM_RSA;
    if (!updated_description->GetTagValue(TAG_ALGORITHM, &algorithm))
        updated_description->push_back(TAG_ALGORITHM, KM_ALGORITHM_RSA);
    if (algorithm != KM_ALGORITHM_RSA)
        return KM_ERROR_IMPORT_PARAMETER_MISMATCH;

    return KM_ERROR_OK;
}

keymaster_error_t RsaKeyFactory::CreateEmptyKey(const AuthorizationSet& hw_enforced,
                                                const AuthorizationSet& sw_enforced,
                                                UniquePtr<AsymmetricKey>* key) const {
    keymaster_error_t error;
    key->reset(new (std::nothrow) RsaKey(hw_enforced, sw_enforced, &error));
    if (!key->get())
        error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return error;
}

}  // namespace keymaster

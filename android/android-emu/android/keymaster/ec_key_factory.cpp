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

#include <keymaster/ec_key_factory.h>

#include <openssl/evp.h>

#include <keymaster/keymaster_context.h>

#include "ec_key.h"
#include "ecdsa_operation.h"
#include "openssl_err.h"

namespace keymaster {

static EcdsaSignOperationFactory sign_factory;
static EcdsaVerifyOperationFactory verify_factory;

OperationFactory* EcKeyFactory::GetOperationFactory(keymaster_purpose_t purpose) const {
    switch (purpose) {
    case KM_PURPOSE_SIGN:
        return &sign_factory;
    case KM_PURPOSE_VERIFY:
        return &verify_factory;
    default:
        return nullptr;
    }
}

/* static */
keymaster_error_t EcKeyFactory::GetCurveAndSize(const AuthorizationSet& key_description,
                                                keymaster_ec_curve_t* curve,
                                                uint32_t* key_size_bits) {
    if (!key_description.GetTagValue(TAG_EC_CURVE, curve)) {
        // Curve not specified. Fall back to deducing curve from key size.
        if (!key_description.GetTagValue(TAG_KEY_SIZE, key_size_bits)) {
            LOG_E("%s", "No curve or key size specified for EC key generation");
            return KM_ERROR_UNSUPPORTED_KEY_SIZE;
        }
        keymaster_error_t error = EcKeySizeToCurve(*key_size_bits, curve);
        if (error != KM_ERROR_OK) {
            return KM_ERROR_UNSUPPORTED_KEY_SIZE;
        }
    } else {
        keymaster_error_t error = EcCurveToKeySize(*curve, key_size_bits);
        if (error != KM_ERROR_OK) {
            return error;
        }
        uint32_t tag_key_size_bits;
        if (key_description.GetTagValue(TAG_KEY_SIZE, &tag_key_size_bits) &&
            *key_size_bits != tag_key_size_bits) {
            LOG_E("Curve key size %d and specified key size %d don't match", key_size_bits,
                  tag_key_size_bits);
            return KM_ERROR_INVALID_ARGUMENT;
        }
    }

    return KM_ERROR_OK;
}

keymaster_error_t EcKeyFactory::GenerateKey(const AuthorizationSet& key_description,
                                            KeymasterKeyBlob* key_blob,
                                            AuthorizationSet* hw_enforced,
                                            AuthorizationSet* sw_enforced) const {
    if (!key_blob || !hw_enforced || !sw_enforced)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    AuthorizationSet authorizations(key_description);

    keymaster_ec_curve_t ec_curve;
    uint32_t key_size;
    keymaster_error_t error = GetCurveAndSize(authorizations, &ec_curve, &key_size);
    if (error != KM_ERROR_OK) {
        return error;
    } else if (!authorizations.Contains(TAG_KEY_SIZE, key_size)) {
        authorizations.push_back(TAG_KEY_SIZE, key_size);
    } else if (!authorizations.Contains(TAG_EC_CURVE, ec_curve)) {
        authorizations.push_back(TAG_EC_CURVE, ec_curve);
    }

    UniquePtr<EC_KEY, EC_KEY_Delete> ec_key(EC_KEY_new());
    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey(EVP_PKEY_new());
    if (ec_key.get() == NULL || pkey.get() == NULL)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    UniquePtr<EC_GROUP, EC_GROUP_Delete> group(ChooseGroup(ec_curve));
    if (group.get() == NULL) {
        LOG_E("Unable to get EC group for curve %d", ec_curve);
        return KM_ERROR_UNSUPPORTED_KEY_SIZE;
    }

#if !defined(OPENSSL_IS_BORINGSSL)
    EC_GROUP_set_point_conversion_form(group.get(), POINT_CONVERSION_UNCOMPRESSED);
    EC_GROUP_set_asn1_flag(group.get(), OPENSSL_EC_NAMED_CURVE);
#endif

    if (EC_KEY_set_group(ec_key.get(), group.get()) != 1 ||
        EC_KEY_generate_key(ec_key.get()) != 1 || EC_KEY_check_key(ec_key.get()) < 0) {
        return TranslateLastOpenSslError();
    }

    if (EVP_PKEY_set1_EC_KEY(pkey.get(), ec_key.get()) != 1)
        return TranslateLastOpenSslError();

    KeymasterKeyBlob key_material;
    error = EvpKeyToKeyMaterial(pkey.get(), &key_material);
    if (error != KM_ERROR_OK)
        return error;

    return context_->CreateKeyBlob(authorizations, KM_ORIGIN_GENERATED, key_material, key_blob,
                                   hw_enforced, sw_enforced);
}

keymaster_error_t EcKeyFactory::ImportKey(const AuthorizationSet& key_description,
                                          keymaster_key_format_t input_key_material_format,
                                          const KeymasterKeyBlob& input_key_material,
                                          KeymasterKeyBlob* output_key_blob,
                                          AuthorizationSet* hw_enforced,
                                          AuthorizationSet* sw_enforced) const {
    if (!output_key_blob || !hw_enforced || !sw_enforced)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    AuthorizationSet authorizations;
    uint32_t key_size;
    keymaster_error_t error = UpdateImportKeyDescription(
        key_description, input_key_material_format, input_key_material, &authorizations, &key_size);
    if (error != KM_ERROR_OK)
        return error;

    return context_->CreateKeyBlob(authorizations, KM_ORIGIN_IMPORTED, input_key_material,
                                   output_key_blob, hw_enforced, sw_enforced);
}

keymaster_error_t EcKeyFactory::UpdateImportKeyDescription(const AuthorizationSet& key_description,
                                                           keymaster_key_format_t key_format,
                                                           const KeymasterKeyBlob& key_material,
                                                           AuthorizationSet* updated_description,
                                                           uint32_t* key_size_bits) const {
    if (!updated_description || !key_size_bits)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey;
    keymaster_error_t error =
        KeyMaterialToEvpKey(key_format, key_material, keymaster_key_type(), &pkey);
    if (error != KM_ERROR_OK)
        return error;

    UniquePtr<EC_KEY, EC_KEY_Delete> ec_key(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (!ec_key.get())
        return TranslateLastOpenSslError();

    updated_description->Reinitialize(key_description);

    size_t extracted_key_size_bits;
    error = ec_get_group_size(EC_KEY_get0_group(ec_key.get()), &extracted_key_size_bits);
    if (error != KM_ERROR_OK)
        return error;

    *key_size_bits = extracted_key_size_bits;
    if (!updated_description->GetTagValue(TAG_KEY_SIZE, key_size_bits))
        updated_description->push_back(TAG_KEY_SIZE, extracted_key_size_bits);
    if (*key_size_bits != extracted_key_size_bits)
        return KM_ERROR_IMPORT_PARAMETER_MISMATCH;

    keymaster_algorithm_t algorithm = KM_ALGORITHM_EC;
    if (!updated_description->GetTagValue(TAG_ALGORITHM, &algorithm))
        updated_description->push_back(TAG_ALGORITHM, KM_ALGORITHM_EC);
    if (algorithm != KM_ALGORITHM_EC)
        return KM_ERROR_IMPORT_PARAMETER_MISMATCH;

    return KM_ERROR_OK;
}

/* static */
EC_GROUP* EcKeyFactory::ChooseGroup(size_t key_size_bits) {
    switch (key_size_bits) {
    case 224:
        return EC_GROUP_new_by_curve_name(NID_secp224r1);
        break;
    case 256:
        return EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
        break;
    case 384:
        return EC_GROUP_new_by_curve_name(NID_secp384r1);
        break;
    case 521:
        return EC_GROUP_new_by_curve_name(NID_secp521r1);
        break;
    default:
        return NULL;
        break;
    }
}

/* static */
EC_GROUP* EcKeyFactory::ChooseGroup(keymaster_ec_curve_t ec_curve) {
    switch (ec_curve) {
    case KM_EC_CURVE_P_224:
        return EC_GROUP_new_by_curve_name(NID_secp224r1);
        break;
    case KM_EC_CURVE_P_256:
        return EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
        break;
    case KM_EC_CURVE_P_384:
        return EC_GROUP_new_by_curve_name(NID_secp384r1);
        break;
    case KM_EC_CURVE_P_521:
        return EC_GROUP_new_by_curve_name(NID_secp521r1);
        break;
    default:
        return nullptr;
        break;
    }
}

keymaster_error_t EcKeyFactory::CreateEmptyKey(const AuthorizationSet& hw_enforced,
                                               const AuthorizationSet& sw_enforced,
                                               UniquePtr<AsymmetricKey>* key) const {
    keymaster_error_t error;
    key->reset(new (std::nothrow) EcKey(hw_enforced, sw_enforced, &error));
    if (!key->get())
        error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return error;
}

}  // namespace keymaster

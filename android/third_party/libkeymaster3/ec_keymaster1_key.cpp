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

#include "ec_keymaster1_key.h"

#include <memory>

#include <keymaster/logger.h>

#include "ecdsa_keymaster1_operation.h"
#include "ecdsa_operation.h"

using std::unique_ptr;

namespace keymaster {

EcdsaKeymaster1KeyFactory::EcdsaKeymaster1KeyFactory(const SoftKeymasterContext* context,
                                                     const Keymaster1Engine* engine)
    : EcKeyFactory(context), engine_(engine),
      sign_factory_(new EcdsaKeymaster1OperationFactory(KM_PURPOSE_SIGN, engine)),
      // For pubkey ops we can use the normal operation factories.
      verify_factory_(new EcdsaVerifyOperationFactory) {}

static bool is_supported(uint32_t digest) {
    return digest == KM_DIGEST_NONE || digest == KM_DIGEST_SHA_2_256;
}

static void UpdateToWorkAroundUnsupportedDigests(const AuthorizationSet& key_description,
                                                 AuthorizationSet* new_description) {
    bool have_unsupported_digests = false;
    bool have_digest_none = false;
    for (const keymaster_key_param_t& entry : key_description) {
        new_description->push_back(entry);

        if (entry.tag == TAG_DIGEST) {
            if (entry.enumerated == KM_DIGEST_NONE) {
                have_digest_none = true;
            } else if (!is_supported(entry.enumerated)) {
                LOG_D("Found request for unsupported digest %u", entry.enumerated);
                have_unsupported_digests = true;
            }
        }
    }

    if (have_unsupported_digests && !have_digest_none) {
        LOG_I("Adding KM_DIGEST_NONE to key authorization, to enable software digesting", 0);
        new_description->push_back(TAG_DIGEST, KM_DIGEST_NONE);
    }
}

keymaster_error_t EcdsaKeymaster1KeyFactory::GenerateKey(const AuthorizationSet& key_description,
                                                         KeymasterKeyBlob* key_blob,
                                                         AuthorizationSet* hw_enforced,
                                                         AuthorizationSet* sw_enforced) const {
    AuthorizationSet key_params_copy;
    UpdateToWorkAroundUnsupportedDigests(key_description, &key_params_copy);

    keymaster_ec_curve_t ec_curve;
    uint32_t key_size;
    keymaster_error_t error = GetCurveAndSize(key_description, &ec_curve, &key_size);
    if (error != KM_ERROR_OK) {
        return error;
    } else if (!key_description.Contains(TAG_KEY_SIZE, key_size)) {
        key_params_copy.push_back(TAG_KEY_SIZE, key_size);
    }
    return engine_->GenerateKey(key_params_copy, key_blob, hw_enforced, sw_enforced);
}

keymaster_error_t EcdsaKeymaster1KeyFactory::ImportKey(
    const AuthorizationSet& key_description, keymaster_key_format_t input_key_material_format,
    const KeymasterKeyBlob& input_key_material, KeymasterKeyBlob* output_key_blob,
    AuthorizationSet* hw_enforced, AuthorizationSet* sw_enforced) const {
    AuthorizationSet key_params_copy;
    UpdateToWorkAroundUnsupportedDigests(key_description, &key_params_copy);
    return engine_->ImportKey(key_params_copy, input_key_material_format, input_key_material,
                              output_key_blob, hw_enforced, sw_enforced);
}

keymaster_error_t EcdsaKeymaster1KeyFactory::LoadKey(const KeymasterKeyBlob& key_material,
                                                     const AuthorizationSet& additional_params,
                                                     const AuthorizationSet& hw_enforced,
                                                     const AuthorizationSet& sw_enforced,
                                                     UniquePtr<Key>* key) const {
    if (!key)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    keymaster_error_t error;
    unique_ptr<EC_KEY, EC_KEY_Delete> ecdsa(
        engine_->BuildEcKey(key_material, additional_params, &error));
    if (!ecdsa)
        return error;

    key->reset(new (std::nothrow)
                   EcdsaKeymaster1Key(ecdsa.release(), hw_enforced, sw_enforced, &error));
    if (!key->get())
        error = KM_ERROR_MEMORY_ALLOCATION_FAILED;

    if (error != KM_ERROR_OK)
        return error;

    return KM_ERROR_OK;
}

OperationFactory*
EcdsaKeymaster1KeyFactory::GetOperationFactory(keymaster_purpose_t purpose) const {
    switch (purpose) {
    case KM_PURPOSE_SIGN:
        return sign_factory_.get();
    case KM_PURPOSE_VERIFY:
        return verify_factory_.get();
    default:
        return nullptr;
    }
}

}  // namespace keymaster

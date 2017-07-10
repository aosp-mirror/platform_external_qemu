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

#ifndef SYSTEM_KEYMASTER_RSA_KEY_FACTORY_H_
#define SYSTEM_KEYMASTER_RSA_KEY_FACTORY_H_

#include <openssl/evp.h>
#include <openssl/rsa.h>

#include <keymaster/asymmetric_key_factory.h>

namespace keymaster {

class RsaKeyFactory : public AsymmetricKeyFactory {
  public:
    explicit RsaKeyFactory(const KeymasterContext* context) : AsymmetricKeyFactory(context) {}

    keymaster_error_t GenerateKey(const AuthorizationSet& key_description,
                                  KeymasterKeyBlob* key_blob, AuthorizationSet* hw_enforced,
                                  AuthorizationSet* sw_enforced) const override;
    keymaster_error_t ImportKey(const AuthorizationSet& key_description,
                                keymaster_key_format_t input_key_material_format,
                                const KeymasterKeyBlob& input_key_material,
                                KeymasterKeyBlob* output_key_blob, AuthorizationSet* hw_enforced,
                                AuthorizationSet* sw_enforced) const override;

    keymaster_error_t CreateEmptyKey(const AuthorizationSet& hw_enforced,
                                     const AuthorizationSet& sw_enforced,
                                     UniquePtr<AsymmetricKey>* key) const override;

    OperationFactory* GetOperationFactory(keymaster_purpose_t purpose) const override;

    keymaster_algorithm_t keymaster_key_type() const override { return KM_ALGORITHM_RSA; }
    int evp_key_type() const override { return EVP_PKEY_RSA; }

  protected:
    keymaster_error_t UpdateImportKeyDescription(const AuthorizationSet& key_description,
                                                 keymaster_key_format_t import_key_format,
                                                 const KeymasterKeyBlob& import_key_material,
                                                 AuthorizationSet* updated_description,
                                                 uint64_t* public_exponent,
                                                 uint32_t* key_size) const;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_RSA_KEY_FACTORY_H_

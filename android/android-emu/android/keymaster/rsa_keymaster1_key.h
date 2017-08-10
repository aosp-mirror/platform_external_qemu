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

#ifndef SYSTEM_KEYMASTER_RSA_KEYMASTER1_KEY_H_
#define SYSTEM_KEYMASTER_RSA_KEYMASTER1_KEY_H_

#include <openssl/rsa.h>

#include <keymaster/rsa_key_factory.h>

#include "keymaster1_engine.h"
#include "rsa_key.h"

namespace keymaster {

class SoftKeymasterContext;

/**
 * RsaKeymaster1KeyFactory is a KeyFactory that creates and loads keys which are actually backed by
 * a hardware keymaster1 module, but which does not support all keymaster1 digests.  If unsupported
 * digests are found during generation or import, KM_DIGEST_NONE is added to the key description,
 * then the operations handle the unsupported digests in software.
 *
 * If unsupported digests are requested and KM_PAD_RSA_PSS or KM_PAD_RSA_OAEP is also requested, but
 * KM_PAD_NONE is not present KM_PAD_NONE will be added to the description, to allow for
 * software padding as well as software digesting.
 */
class RsaKeymaster1KeyFactory : public RsaKeyFactory {
  public:
    RsaKeymaster1KeyFactory(const SoftKeymasterContext* context, const Keymaster1Engine* engine);

    keymaster_error_t GenerateKey(const AuthorizationSet& key_description,
                                  KeymasterKeyBlob* key_blob, AuthorizationSet* hw_enforced,
                                  AuthorizationSet* sw_enforced) const override;

    keymaster_error_t ImportKey(const AuthorizationSet& key_description,
                                keymaster_key_format_t input_key_material_format,
                                const KeymasterKeyBlob& input_key_material,
                                KeymasterKeyBlob* output_key_blob, AuthorizationSet* hw_enforced,
                                AuthorizationSet* sw_enforced) const override;

    keymaster_error_t LoadKey(const KeymasterKeyBlob& key_material,
                              const AuthorizationSet& additional_params,
                              const AuthorizationSet& hw_enforced,
                              const AuthorizationSet& sw_enforced,
                              UniquePtr<Key>* key) const override;

    OperationFactory* GetOperationFactory(keymaster_purpose_t purpose) const override;

  private:
    const Keymaster1Engine* engine_;

    std::unique_ptr<OperationFactory> sign_factory_;
    std::unique_ptr<OperationFactory> decrypt_factory_;
    std::unique_ptr<OperationFactory> verify_factory_;
    std::unique_ptr<OperationFactory> encrypt_factory_;
};

class RsaKeymaster1Key : public RsaKey {
  public:
    RsaKeymaster1Key(RSA* rsa_key, const AuthorizationSet& hw_enforced,
                     const AuthorizationSet& sw_enforced, keymaster_error_t* error)
        : RsaKey(rsa_key, hw_enforced, sw_enforced, error) {}
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_RSA_KEYMASTER1_KEY_H_

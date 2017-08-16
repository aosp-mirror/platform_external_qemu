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

#ifndef SYSTEM_KEYMASTER_AES_KEY_H_
#define SYSTEM_KEYMASTER_AES_KEY_H_

#include <openssl/aes.h>

#include "symmetric_key.h"

namespace keymaster {

const size_t kMinGcmTagLength = 12 * 8;
const size_t kMaxGcmTagLength = 16 * 8;

class AesKeyFactory : public SymmetricKeyFactory {
  public:
    explicit AesKeyFactory(const KeymasterContext* context) : SymmetricKeyFactory(context) {}

    keymaster_algorithm_t registry_key() const { return KM_ALGORITHM_AES; }

    keymaster_error_t LoadKey(const KeymasterKeyBlob& key_material,
                              const AuthorizationSet& additional_params,
                              const AuthorizationSet& hw_enforced,
                              const AuthorizationSet& sw_enforced,
                              UniquePtr<Key>* key) const override;

    OperationFactory* GetOperationFactory(keymaster_purpose_t purpose) const override;

  private:
    bool key_size_supported(size_t key_size_bits) const override {
        return key_size_bits == 128 || key_size_bits == 192 || key_size_bits == 256;
    }
    keymaster_error_t validate_algorithm_specific_new_key_params(
        const AuthorizationSet& key_description) const override;
};

class AesKey : public SymmetricKey {
  public:
    AesKey(const KeymasterKeyBlob& key_material, const AuthorizationSet& hw_enforced,
           const AuthorizationSet& sw_enforced, keymaster_error_t* error)
        : SymmetricKey(key_material, hw_enforced, sw_enforced, error) {}
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_AES_KEY_H_

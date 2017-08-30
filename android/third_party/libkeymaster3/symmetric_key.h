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

#ifndef SYSTEM_KEYMASTER_SYMMETRIC_KEY_H_
#define SYSTEM_KEYMASTER_SYMMETRIC_KEY_H_

#include <keymaster/key_factory.h>

#include "key.h"

namespace keymaster {

class SymmetricKey;

class SymmetricKeyFactory : public KeyFactory {
  public:
    explicit SymmetricKeyFactory(const KeymasterContext* context) : KeyFactory(context) {}

    keymaster_error_t GenerateKey(const AuthorizationSet& key_description,
                                  KeymasterKeyBlob* key_blob, AuthorizationSet* hw_enforced,
                                  AuthorizationSet* sw_enforced) const override;
    keymaster_error_t ImportKey(const AuthorizationSet& key_description,
                                keymaster_key_format_t input_key_material_format,
                                const KeymasterKeyBlob& input_key_material,
                                KeymasterKeyBlob* output_key_blob, AuthorizationSet* hw_enforced,
                                AuthorizationSet* sw_enforced) const override;

    virtual const keymaster_key_format_t* SupportedImportFormats(size_t* format_count) const;
    virtual const keymaster_key_format_t* SupportedExportFormats(size_t* format_count) const {
        return NoFormats(format_count);
    };

  private:
    virtual bool key_size_supported(size_t key_size_bits) const = 0;
    virtual keymaster_error_t
    validate_algorithm_specific_new_key_params(const AuthorizationSet& key_description) const = 0;

    const keymaster_key_format_t* NoFormats(size_t* format_count) const {
        *format_count = 0;
        return NULL;
    }
};

class SymmetricKey : public Key {
  public:
    ~SymmetricKey();

    virtual keymaster_error_t key_material(UniquePtr<uint8_t[]>* key_material, size_t* size) const;
    virtual keymaster_error_t formatted_key_material(keymaster_key_format_t, UniquePtr<uint8_t[]>*,
                                                     size_t*) const {
        return KM_ERROR_UNSUPPORTED_KEY_FORMAT;
    }

    const uint8_t* key_data() const { return key_data_.get(); }
    size_t key_data_size() const { return key_data_size_; }

  protected:
    SymmetricKey(const KeymasterKeyBlob& key_material, const AuthorizationSet& hw_enforced,
                 const AuthorizationSet& sw_enforced, keymaster_error_t* error);

  private:
    size_t key_data_size_;
    UniquePtr<uint8_t[]> key_data_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_AES_KEY_H_

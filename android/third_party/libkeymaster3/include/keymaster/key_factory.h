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

#ifndef SYSTEM_KEYMASTER_KEY_FACTORY_H_
#define SYSTEM_KEYMASTER_KEY_FACTORY_H_

#include <hardware/keymaster_defs.h>
#include <keymaster/authorization_set.h>

namespace keymaster {

class Key;
class KeymasterContext;
class OperationFactory;
struct KeymasterKeyBlob;

/**
 * KeyFactory is a abstraction that encapsulats the knowledge of how to build and parse a specifiec
 * subclass of Key.
 */
class KeyFactory {
  public:
    explicit KeyFactory(const KeymasterContext* context) : context_(context) {}
    virtual ~KeyFactory() {}

    // Factory methods.
    virtual keymaster_error_t GenerateKey(const AuthorizationSet& key_description,
                                          KeymasterKeyBlob* key_blob, AuthorizationSet* hw_enforced,
                                          AuthorizationSet* sw_enforced) const = 0;

    virtual keymaster_error_t ImportKey(const AuthorizationSet& key_description,
                                        keymaster_key_format_t input_key_material_format,
                                        const KeymasterKeyBlob& input_key_material,
                                        KeymasterKeyBlob* output_key_blob,
                                        AuthorizationSet* hw_enforced,
                                        AuthorizationSet* sw_enforced) const = 0;

    virtual keymaster_error_t LoadKey(const KeymasterKeyBlob& key_material,
                                      const AuthorizationSet& additional_params,
                                      const AuthorizationSet& hw_enforced,
                                      const AuthorizationSet& sw_enforced,
                                      UniquePtr<Key>* key) const = 0;

    virtual OperationFactory* GetOperationFactory(keymaster_purpose_t purpose) const = 0;

    // Informational methods.
    virtual const keymaster_key_format_t* SupportedImportFormats(size_t* format_count) const = 0;
    virtual const keymaster_key_format_t* SupportedExportFormats(size_t* format_count) const = 0;

  protected:
    const KeymasterContext* context_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEY_FACTORY_H_

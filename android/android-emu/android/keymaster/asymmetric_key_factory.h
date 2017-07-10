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

#ifndef SYSTEM_KEYMASTER_ASYMMETRIC_KEY_FACTORY_H_
#define SYSTEM_KEYMASTER_ASYMMETRIC_KEY_FACTORY_H_

#include <keymaster/key_factory.h>

namespace keymaster {

/**
 * Abstract base for KeyFactories that handle asymmetric keys.
 */
class AsymmetricKey;
class AsymmetricKeyFactory : public KeyFactory {
  public:
    explicit AsymmetricKeyFactory(const KeymasterContext* context) : KeyFactory(context) {}

    keymaster_error_t LoadKey(const KeymasterKeyBlob& key_material,
                              const AuthorizationSet& additional_params,
                              const AuthorizationSet& hw_enforced,
                              const AuthorizationSet& sw_enforced,
                              UniquePtr<Key>* key) const override;

    virtual keymaster_error_t CreateEmptyKey(const AuthorizationSet& hw_enforced,
                                             const AuthorizationSet& sw_enforced,
                                             UniquePtr<AsymmetricKey>* key) const = 0;

    virtual keymaster_algorithm_t keymaster_key_type() const = 0;
    virtual int evp_key_type() const = 0;

    virtual const keymaster_key_format_t* SupportedImportFormats(size_t* format_count) const override;
    virtual const keymaster_key_format_t* SupportedExportFormats(size_t* format_count) const override;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ASYMMETRIC_KEY_FACTORY_H_

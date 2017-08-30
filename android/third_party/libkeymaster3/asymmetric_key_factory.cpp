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

#include <keymaster/asymmetric_key_factory.h>

#include <keymaster/android_keymaster_utils.h>

#include "asymmetric_key.h"
#include "openssl_err.h"
#include "openssl_utils.h"

namespace keymaster {

static const keymaster_key_format_t supported_import_formats[] = {KM_KEY_FORMAT_PKCS8};
const keymaster_key_format_t*
AsymmetricKeyFactory::SupportedImportFormats(size_t* format_count) const {
    *format_count = array_length(supported_import_formats);
    return supported_import_formats;
}

static const keymaster_key_format_t supported_export_formats[] = {KM_KEY_FORMAT_X509};
const keymaster_key_format_t*
AsymmetricKeyFactory::SupportedExportFormats(size_t* format_count) const {
    *format_count = array_length(supported_export_formats);
    return supported_export_formats;
}

keymaster_error_t AsymmetricKeyFactory::LoadKey(const KeymasterKeyBlob& key_material,
                                                const AuthorizationSet& /* additional_params */,
                                                const AuthorizationSet& hw_enforced,
                                                const AuthorizationSet& sw_enforced,
                                                UniquePtr<Key>* key) const {
    UniquePtr<AsymmetricKey> asymmetric_key;
    keymaster_error_t error = CreateEmptyKey(hw_enforced, sw_enforced, &asymmetric_key);
    if (error != KM_ERROR_OK)
        return error;

    const uint8_t* tmp = key_material.key_material;
    EVP_PKEY* pkey =
        d2i_PrivateKey(evp_key_type(), NULL /* pkey */, &tmp, key_material.key_material_size);
    if (!pkey)
        return TranslateLastOpenSslError();
    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey_deleter(pkey);

    if (!asymmetric_key->EvpToInternal(pkey))
        error = TranslateLastOpenSslError();
    else
        key->reset(asymmetric_key.release());

    return error;
}

}  // namespace keymaster

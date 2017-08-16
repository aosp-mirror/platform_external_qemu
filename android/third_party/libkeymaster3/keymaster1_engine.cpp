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

#include "keymaster1_engine.h"

#include <assert.h>

#include <algorithm>
#include <memory>

#define LOG_TAG "Keymaster1Engine"
#include <cutils/log.h>

#include "keymaster/android_keymaster_utils.h"

#include <openssl/bn.h>
#include <openssl/ec_key.h>
#include <openssl/ecdsa.h>

#include "openssl_err.h"
#include "openssl_utils.h"

using std::shared_ptr;
using std::unique_ptr;

namespace keymaster {

Keymaster1Engine* Keymaster1Engine::instance_ = nullptr;

Keymaster1Engine::Keymaster1Engine(const keymaster1_device_t* keymaster1_device)
    : keymaster1_device_(keymaster1_device), engine_(ENGINE_new()),
      rsa_index_(RSA_get_ex_new_index(0 /* argl */, NULL /* argp */, NULL /* new_func */,
                                      Keymaster1Engine::duplicate_key_data,
                                      Keymaster1Engine::free_key_data)),
      ec_key_index_(EC_KEY_get_ex_new_index(0 /* argl */, NULL /* argp */, NULL /* new_func */,
                                            Keymaster1Engine::duplicate_key_data,
                                            Keymaster1Engine::free_key_data)),
      rsa_method_(BuildRsaMethod()), ecdsa_method_(BuildEcdsaMethod()) {
    assert(rsa_index_ != -1);
    assert(ec_key_index_ != -1);
    assert(keymaster1_device);
    assert(!instance_);

    instance_ = this;

    ENGINE_set_RSA_method(engine_.get(), &rsa_method_, sizeof(rsa_method_));
    ENGINE_set_ECDSA_method(engine_.get(), &ecdsa_method_, sizeof(ecdsa_method_));
}

Keymaster1Engine::~Keymaster1Engine() {
    keymaster1_device_->common.close(
        reinterpret_cast<hw_device_t*>(const_cast<keymaster1_device_t*>(keymaster1_device_)));
    instance_ = nullptr;
}

static void ConvertCharacteristics(keymaster_key_characteristics_t* characteristics,
                                   AuthorizationSet* hw_enforced, AuthorizationSet* sw_enforced) {
    unique_ptr<keymaster_key_characteristics_t, Characteristics_Delete> characteristics_deleter(
        characteristics);
    if (hw_enforced)
        hw_enforced->Reinitialize(characteristics->hw_enforced);
    if (sw_enforced)
        sw_enforced->Reinitialize(characteristics->sw_enforced);
}

keymaster_error_t Keymaster1Engine::GenerateKey(const AuthorizationSet& key_description,
                                                KeymasterKeyBlob* key_blob,
                                                AuthorizationSet* hw_enforced,
                                                AuthorizationSet* sw_enforced) const {
    assert(key_blob);

    keymaster_key_characteristics_t* characteristics;
    keymaster_key_blob_t blob;
    keymaster_error_t error = keymaster1_device_->generate_key(keymaster1_device_, &key_description,
                                                               &blob, &characteristics);
    if (error != KM_ERROR_OK)
        return error;
    unique_ptr<uint8_t, Malloc_Delete> blob_deleter(const_cast<uint8_t*>(blob.key_material));
    key_blob->key_material = dup_buffer(blob.key_material, blob.key_material_size);
    key_blob->key_material_size = blob.key_material_size;

    ConvertCharacteristics(characteristics, hw_enforced, sw_enforced);
    return error;
}

keymaster_error_t Keymaster1Engine::ImportKey(const AuthorizationSet& key_description,
                                              keymaster_key_format_t input_key_material_format,
                                              const KeymasterKeyBlob& input_key_material,
                                              KeymasterKeyBlob* output_key_blob,
                                              AuthorizationSet* hw_enforced,
                                              AuthorizationSet* sw_enforced) const {
    assert(output_key_blob);

    keymaster_key_characteristics_t* characteristics;
    const keymaster_blob_t input_key = {input_key_material.key_material,
                                        input_key_material.key_material_size};
    keymaster_key_blob_t blob;
    keymaster_error_t error = keymaster1_device_->import_key(keymaster1_device_, &key_description,
                                                             input_key_material_format, &input_key,
                                                             &blob, &characteristics);
    if (error != KM_ERROR_OK)
        return error;
    unique_ptr<uint8_t, Malloc_Delete> blob_deleter(const_cast<uint8_t*>(blob.key_material));
    output_key_blob->key_material = dup_buffer(blob.key_material, blob.key_material_size);
    output_key_blob->key_material_size = blob.key_material_size;

    ConvertCharacteristics(characteristics, hw_enforced, sw_enforced);
    return error;
}

keymaster_error_t Keymaster1Engine::DeleteKey(const KeymasterKeyBlob& blob) const {
    if (!keymaster1_device_->delete_key)
        return KM_ERROR_OK;
    return keymaster1_device_->delete_key(keymaster1_device_, &blob);
}

keymaster_error_t Keymaster1Engine::DeleteAllKeys() const {
    if (!keymaster1_device_->delete_all_keys)
        return KM_ERROR_OK;
    return keymaster1_device_->delete_all_keys(keymaster1_device_);
}

RSA* Keymaster1Engine::BuildRsaKey(const KeymasterKeyBlob& blob,
                                   const AuthorizationSet& additional_params,
                                   keymaster_error_t* error) const {
    // Create new RSA key (with engine methods) and add metadata
    unique_ptr<RSA, RSA_Delete> rsa(RSA_new_method(engine_.get()));
    if (!rsa) {
        *error = TranslateLastOpenSslError();
        return nullptr;
    }

    KeyData* key_data = new KeyData(blob, additional_params);
    if (!RSA_set_ex_data(rsa.get(), rsa_index_, key_data)) {
        *error = TranslateLastOpenSslError();
        delete key_data;
        return nullptr;
    }

    // Copy public key into new RSA key
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> pkey(
        GetKeymaster1PublicKey(key_data->key_material, key_data->begin_params, error));
    if (*error != KM_ERROR_OK)
        return nullptr;

    unique_ptr<RSA, RSA_Delete> public_rsa(EVP_PKEY_get1_RSA(pkey.get()));
    if (!public_rsa) {
        *error = TranslateLastOpenSslError();
        return nullptr;
    }

    rsa->n = BN_dup(public_rsa->n);
    rsa->e = BN_dup(public_rsa->e);
    if (!rsa->n || !rsa->e) {
        *error = TranslateLastOpenSslError();
        return nullptr;
    }

    *error = KM_ERROR_OK;
    return rsa.release();
}

EC_KEY* Keymaster1Engine::BuildEcKey(const KeymasterKeyBlob& blob,
                                     const AuthorizationSet& additional_params,
                                     keymaster_error_t* error) const {
    // Create new EC key (with engine methods) and insert blob
    unique_ptr<EC_KEY, EC_KEY_Delete> ec_key(EC_KEY_new_method(engine_.get()));
    if (!ec_key) {
        *error = TranslateLastOpenSslError();
        return nullptr;
    }

    KeyData* key_data = new KeyData(blob, additional_params);
    if (!EC_KEY_set_ex_data(ec_key.get(), ec_key_index_, key_data)) {
        *error = TranslateLastOpenSslError();
        delete key_data;
        return nullptr;
    }

    // Copy public key into new EC key
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> pkey(
        GetKeymaster1PublicKey(blob, additional_params, error));
    if (*error != KM_ERROR_OK)
        return nullptr;

    unique_ptr<EC_KEY, EC_KEY_Delete> public_ec_key(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (!public_ec_key) {
        *error = TranslateLastOpenSslError();
        return nullptr;
    }

    if (!EC_KEY_set_group(ec_key.get(), EC_KEY_get0_group(public_ec_key.get())) ||
        !EC_KEY_set_public_key(ec_key.get(), EC_KEY_get0_public_key(public_ec_key.get()))) {
        *error = TranslateLastOpenSslError();
        return nullptr;
    }

    *error = KM_ERROR_OK;
    return ec_key.release();
}

Keymaster1Engine::KeyData* Keymaster1Engine::GetData(EVP_PKEY* key) const {
    switch (EVP_PKEY_type(key->type)) {
    case EVP_PKEY_RSA: {
        unique_ptr<RSA, RSA_Delete> rsa(EVP_PKEY_get1_RSA(key));
        return GetData(rsa.get());
    }

    case EVP_PKEY_EC: {
        unique_ptr<EC_KEY, EC_KEY_Delete> ec_key(EVP_PKEY_get1_EC_KEY(key));
        return GetData(ec_key.get());
    }

    default:
        return nullptr;
    };
}

Keymaster1Engine::KeyData* Keymaster1Engine::GetData(const RSA* rsa) const {
    if (!rsa)
        return nullptr;
    return reinterpret_cast<KeyData*>(RSA_get_ex_data(rsa, rsa_index_));
}

Keymaster1Engine::KeyData* Keymaster1Engine::GetData(const EC_KEY* ec_key) const {
    if (!ec_key)
        return nullptr;
    return reinterpret_cast<KeyData*>(EC_KEY_get_ex_data(ec_key, ec_key_index_));
}

/* static */
int Keymaster1Engine::duplicate_key_data(CRYPTO_EX_DATA* /* to */, const CRYPTO_EX_DATA* /* from */,
                                         void** from_d, int /* index */, long /* argl */,
                                         void* /* argp */) {
    KeyData* data = reinterpret_cast<KeyData*>(*from_d);
    if (!data)
        return 1;

    // Default copy ctor is good.
    *from_d = new KeyData(*data);
    if (*from_d)
        return 1;
    return 0;
}

/* static */
void Keymaster1Engine::free_key_data(void* /* parent */, void* ptr, CRYPTO_EX_DATA* /* data */,
                                     int /* index*/, long /* argl */, void* /* argp */) {
    delete reinterpret_cast<KeyData*>(ptr);
}

keymaster_error_t Keymaster1Engine::Keymaster1Finish(const KeyData* key_data,
                                                     const keymaster_blob_t& input,
                                                     keymaster_blob_t* output) {
    if (key_data->op_handle == 0)
        return KM_ERROR_UNKNOWN_ERROR;

    size_t input_consumed;
    // Note: devices are required to consume all input in a single update call for undigested
    // signing operations and encryption operations.  No need to loop here.
    keymaster_error_t error =
        device()->update(device(), key_data->op_handle, &key_data->finish_params, &input,
                         &input_consumed, nullptr /* out_params */, nullptr /* output */);
    if (error != KM_ERROR_OK)
        return error;

    return device()->finish(device(), key_data->op_handle, &key_data->finish_params,
                            nullptr /* signature */, nullptr /* out_params */, output);
}

/* static */
int Keymaster1Engine::rsa_sign_raw(RSA* rsa, size_t* out_len, uint8_t* out, size_t max_out,
                                   const uint8_t* in, size_t in_len, int padding) {
    KeyData* key_data = instance_->GetData(rsa);
    if (!key_data)
        return 0;

    if (padding != key_data->expected_openssl_padding) {
        LOG_E("Expected sign_raw with padding %d but got padding %d",
              key_data->expected_openssl_padding, padding);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    keymaster_blob_t input = {in, in_len};
    keymaster_blob_t output;
    key_data->error = instance_->Keymaster1Finish(key_data, input, &output);
    if (key_data->error != KM_ERROR_OK)
        return 0;
    unique_ptr<uint8_t, Malloc_Delete> output_deleter(const_cast<uint8_t*>(output.data));

    *out_len = std::min(output.data_length, max_out);
    memcpy(out, output.data, *out_len);
    return 1;
}

/* static */
int Keymaster1Engine::rsa_decrypt(RSA* rsa, size_t* out_len, uint8_t* out, size_t max_out,
                                  const uint8_t* in, size_t in_len, int padding) {
    KeyData* key_data = instance_->GetData(rsa);
    if (!key_data)
        return 0;

    if (padding != key_data->expected_openssl_padding) {
        LOG_E("Expected sign_raw with padding %d but got padding %d",
              key_data->expected_openssl_padding, padding);
        return KM_ERROR_UNKNOWN_ERROR;
    }

    keymaster_blob_t input = {in, in_len};
    keymaster_blob_t output;
    key_data->error = instance_->Keymaster1Finish(key_data, input, &output);
    if (key_data->error != KM_ERROR_OK)
        return 0;
    unique_ptr<uint8_t, Malloc_Delete> output_deleter(const_cast<uint8_t*>(output.data));

    *out_len = std::min(output.data_length, max_out);
    memcpy(out, output.data, *out_len);
    return 1;
}

/* static */
int Keymaster1Engine::ecdsa_sign(const uint8_t* digest, size_t digest_len, uint8_t* sig,
                                 unsigned int* sig_len, EC_KEY* ec_key) {
    KeyData* key_data = instance_->GetData(ec_key);
    if (!key_data)
        return 0;

    // Truncate digest if it's too long
    size_t max_input_len = (ec_group_size_bits(ec_key) + 7) / 8;
    if (digest_len > max_input_len)
        digest_len = max_input_len;

    keymaster_blob_t input = {digest, digest_len};
    keymaster_blob_t output;
    key_data->error = instance_->Keymaster1Finish(key_data, input, &output);
    if (key_data->error != KM_ERROR_OK)
        return 0;
    unique_ptr<uint8_t, Malloc_Delete> output_deleter(const_cast<uint8_t*>(output.data));

    *sig_len = std::min(output.data_length, ECDSA_size(ec_key));
    memcpy(sig, output.data, *sig_len);
    return 1;
}

EVP_PKEY* Keymaster1Engine::GetKeymaster1PublicKey(const KeymasterKeyBlob& blob,
                                                   const AuthorizationSet& additional_params,
                                                   keymaster_error_t* error) const {
    keymaster_blob_t client_id = {nullptr, 0};
    keymaster_blob_t app_data = {nullptr, 0};
    keymaster_blob_t* client_id_ptr = nullptr;
    keymaster_blob_t* app_data_ptr = nullptr;
    if (additional_params.GetTagValue(TAG_APPLICATION_ID, &client_id))
        client_id_ptr = &client_id;
    if (additional_params.GetTagValue(TAG_APPLICATION_DATA, &app_data))
        app_data_ptr = &app_data;

    keymaster_blob_t export_data = {nullptr, 0};
    *error = keymaster1_device_->export_key(keymaster1_device_, KM_KEY_FORMAT_X509, &blob,
                                            client_id_ptr, app_data_ptr, &export_data);
    if (*error != KM_ERROR_OK)
        return nullptr;

    unique_ptr<uint8_t, Malloc_Delete> pub_key(const_cast<uint8_t*>(export_data.data));

    const uint8_t* p = export_data.data;
    auto result = d2i_PUBKEY(nullptr /* allocate new struct */, &p, export_data.data_length);
    if (!result) {
        *error = TranslateLastOpenSslError();
    }
    return result;
}

RSA_METHOD Keymaster1Engine::BuildRsaMethod() {
    RSA_METHOD method = {};

    method.common.is_static = 1;
    method.sign_raw = Keymaster1Engine::rsa_sign_raw;
    method.decrypt = Keymaster1Engine::rsa_decrypt;
    method.flags = RSA_FLAG_OPAQUE;

    return method;
}

ECDSA_METHOD Keymaster1Engine::BuildEcdsaMethod() {
    ECDSA_METHOD method = {};

    method.common.is_static = 1;
    method.sign = Keymaster1Engine::ecdsa_sign;
    method.flags = ECDSA_FLAG_OPAQUE;

    return method;
}

}  // namespace keymaster

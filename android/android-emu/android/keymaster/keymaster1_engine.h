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

#ifndef SYSTEM_KEYMASTER_KEYMASTER1_ENGINE_H_
#define SYSTEM_KEYMASTER_KEYMASTER1_ENGINE_H_

#include <memory>

#include <openssl/ec.h>
#include <openssl/engine.h>
#include <openssl/ex_data.h>
#include <openssl/rsa.h>

#include <hardware/keymaster1.h>
#include <hardware/keymaster_defs.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>

#include "openssl_utils.h"

namespace keymaster {

class Keymaster1Engine {
  public:
    /**
     * Create a Keymaster1Engine, wrapping the provided keymaster1_device.  The engine takes
     * ownership of the device, and will close it during destruction.
     */
    explicit Keymaster1Engine(const keymaster1_device_t* keymaster1_device);
    ~Keymaster1Engine();

    keymaster_error_t GenerateKey(const AuthorizationSet& key_description,
                                  KeymasterKeyBlob* key_material, AuthorizationSet* hw_enforced,
                                  AuthorizationSet* sw_enforced) const;

    keymaster_error_t ImportKey(const AuthorizationSet& key_description,
                                keymaster_key_format_t input_key_material_format,
                                const KeymasterKeyBlob& input_key_material,
                                KeymasterKeyBlob* output_key_blob, AuthorizationSet* hw_enforced,
                                AuthorizationSet* sw_enforced) const;
    keymaster_error_t DeleteKey(const KeymasterKeyBlob& blob) const;
    keymaster_error_t DeleteAllKeys() const;

    struct KeyData {
        KeyData(const KeymasterKeyBlob& blob, const AuthorizationSet& params)
            : op_handle(0), begin_params(params), key_material(blob), error(KM_ERROR_OK),
              expected_openssl_padding(-1) {}

        keymaster_operation_handle_t op_handle;
        AuthorizationSet begin_params;
        AuthorizationSet finish_params;
        KeymasterKeyBlob key_material;
        keymaster_error_t error;
        int expected_openssl_padding;
    };

    RSA* BuildRsaKey(const KeymasterKeyBlob& blob, const AuthorizationSet& additional_params,
                     keymaster_error_t* error) const;
    EC_KEY* BuildEcKey(const KeymasterKeyBlob& blob, const AuthorizationSet& additional_params,
                       keymaster_error_t* error) const;

    KeyData* GetData(EVP_PKEY* key) const;
    KeyData* GetData(const RSA* rsa) const;
    KeyData* GetData(const EC_KEY* rsa) const;

    const keymaster1_device_t* device() const { return keymaster1_device_; }

    EVP_PKEY* GetKeymaster1PublicKey(const KeymasterKeyBlob& blob,
                                     const AuthorizationSet& additional_params,
                                     keymaster_error_t* error) const;

  private:
    Keymaster1Engine(const Keymaster1Engine&);  // Uncopyable
    void operator=(const Keymaster1Engine&);    // Unassignable

    RSA_METHOD BuildRsaMethod();
    ECDSA_METHOD BuildEcdsaMethod();
    void ConfigureEngineForRsa();
    void ConfigureEngineForEcdsa();

    keymaster_error_t Keymaster1Finish(const KeyData* key_data, const keymaster_blob_t& input,
                                       keymaster_blob_t* output);

    static int duplicate_key_data(CRYPTO_EX_DATA* to, const CRYPTO_EX_DATA* from, void** from_d,
                                  int index, long argl, void* argp);
    static void free_key_data(void* parent, void* ptr, CRYPTO_EX_DATA* data, int index, long argl,
                              void* argp);

    static int rsa_sign_raw(RSA* rsa, size_t* out_len, uint8_t* out, size_t max_out,
                            const uint8_t* in, size_t in_len, int padding);
    static int rsa_decrypt(RSA* rsa, size_t* out_len, uint8_t* out, size_t max_out,
                           const uint8_t* in, size_t in_len, int padding);
    static int ecdsa_sign(const uint8_t* digest, size_t digest_len, uint8_t* sig,
                          unsigned int* sig_len, EC_KEY* ec_key);

    const keymaster1_device_t* const keymaster1_device_;
    const std::unique_ptr<ENGINE, ENGINE_Delete> engine_;
    const int rsa_index_;
    const int ec_key_index_;

    const RSA_METHOD rsa_method_;
    const ECDSA_METHOD ecdsa_method_;

    static Keymaster1Engine* instance_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEYMASTER1_ENGINE_H_

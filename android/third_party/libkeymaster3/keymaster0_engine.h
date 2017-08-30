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

#ifndef SYSTEM_KEYMASTER_KEYMASTER0_ENGINE_H_
#define SYSTEM_KEYMASTER_KEYMASTER0_ENGINE_H_

#include <memory>

#include <openssl/ec.h>
#include <openssl/engine.h>
#include <openssl/ex_data.h>
#include <openssl/rsa.h>

#include <hardware/keymaster0.h>
#include <hardware/keymaster_defs.h>

namespace keymaster {

struct KeymasterKeyBlob;

/* Keymaster0Engine is a BoringSSL ENGINE that implements RSA & EC by forwarding the requested
 * operations to a keymaster0 module. */
class Keymaster0Engine {
  public:
    /**
     * Create a Keymaster0Engine, wrapping the provided keymaster0_device.  The engine takes
     * ownership of the device, and will close it during destruction.
     */
    explicit Keymaster0Engine(const keymaster0_device_t* keymaster0_device);
    ~Keymaster0Engine();

    bool supports_ec() const { return supports_ec_; }

    bool GenerateRsaKey(uint64_t public_exponent, uint32_t public_modulus,
                        KeymasterKeyBlob* key_material) const;
    bool GenerateEcKey(uint32_t key_size, KeymasterKeyBlob* key_material) const;

    bool ImportKey(keymaster_key_format_t key_format, const KeymasterKeyBlob& to_import,
                   KeymasterKeyBlob* imported_key_material) const;
    bool DeleteKey(const KeymasterKeyBlob& blob) const;
    bool DeleteAllKeys() const;

    RSA* BlobToRsaKey(const KeymasterKeyBlob& blob) const;
    EC_KEY* BlobToEcKey(const KeymasterKeyBlob& blob) const;

    const keymaster_key_blob_t* RsaKeyToBlob(const RSA* rsa) const;
    const keymaster_key_blob_t* EcKeyToBlob(const EC_KEY* rsa) const;

    const keymaster0_device_t* device() { return keymaster0_device_; }

    EVP_PKEY* GetKeymaster0PublicKey(const KeymasterKeyBlob& blob) const;

  private:
    Keymaster0Engine(const Keymaster0Engine&);  // Uncopyable
    void operator=(const Keymaster0Engine&);    // Unassignable

    static int keyblob_dup(CRYPTO_EX_DATA* to, const CRYPTO_EX_DATA* from, void** from_d, int index,
                           long argl, void* argp);
    static void keyblob_free(void* parent, void* ptr, CRYPTO_EX_DATA* data, int index, long argl,
                             void* argp);
    static int rsa_private_transform(RSA* rsa, uint8_t* out, const uint8_t* in, size_t len);
    static int ecdsa_sign(const uint8_t* digest, size_t digest_len, uint8_t* sig,
                          unsigned int* sig_len, EC_KEY* ec_key);

    struct Malloc_Delete {
        void operator()(void* p) { free(p); }
    };

    bool Keymaster0Sign(const void* signing_params, const keymaster_key_blob_t& key_blob,
                        const uint8_t* data, const size_t data_length,
                        std::unique_ptr<uint8_t[], Malloc_Delete>* signature,
                        size_t* signature_length) const;

    int RsaPrivateTransform(RSA* rsa, uint8_t* out, const uint8_t* in, size_t len) const;
    int EcdsaSign(const uint8_t* digest, size_t digest_len, uint8_t* sig, unsigned int* sig_len,
                  EC_KEY* ec_key) const;

    const keymaster0_device_t* keymaster0_device_;
    ENGINE* const engine_;
    int rsa_index_, ec_key_index_;
    bool supports_ec_;
    RSA_METHOD rsa_method_;
    ECDSA_METHOD ecdsa_method_;

    static Keymaster0Engine* instance_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEYMASTER0_ENGINE_H_

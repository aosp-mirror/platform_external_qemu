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

#include "keymaster0_engine.h"

#include <assert.h>
#include <string.h>

#include <memory>

#define LOG_TAG "Keymaster0Engine"
#include <cutils/log.h>

#include "keymaster/android_keymaster_utils.h"

#include <openssl/bn.h>
#include <openssl/ec_key.h>
#include <openssl/ecdsa.h>

#include "openssl_utils.h"

using std::shared_ptr;
using std::unique_ptr;

namespace keymaster {

Keymaster0Engine* Keymaster0Engine::instance_ = nullptr;

Keymaster0Engine::Keymaster0Engine(const keymaster0_device_t* keymaster0_device)
    : keymaster0_device_(keymaster0_device), engine_(ENGINE_new()), supports_ec_(false) {
    assert(!instance_);
    instance_ = this;

    rsa_index_ = RSA_get_ex_new_index(0 /* argl */, NULL /* argp */, NULL /* new_func */,
                                      keyblob_dup, keyblob_free);
    ec_key_index_ = EC_KEY_get_ex_new_index(0 /* argl */, NULL /* argp */, NULL /* new_func */,
                                            keyblob_dup, keyblob_free);

    memset(&rsa_method_, 0, sizeof(rsa_method_));
    rsa_method_.common.is_static = 1;
    rsa_method_.private_transform = Keymaster0Engine::rsa_private_transform;
    rsa_method_.flags = RSA_FLAG_OPAQUE;

    ENGINE_set_RSA_method(engine_, &rsa_method_, sizeof(rsa_method_));

    if ((keymaster0_device_->flags & KEYMASTER_SUPPORTS_EC) != 0) {
        supports_ec_ = true;

        memset(&ecdsa_method_, 0, sizeof(ecdsa_method_));
        ecdsa_method_.common.is_static = 1;
        ecdsa_method_.sign = Keymaster0Engine::ecdsa_sign;
        ecdsa_method_.flags = ECDSA_FLAG_OPAQUE;

        ENGINE_set_ECDSA_method(engine_, &ecdsa_method_, sizeof(ecdsa_method_));
    }
}

Keymaster0Engine::~Keymaster0Engine() {
    if (keymaster0_device_)
        keymaster0_device_->common.close(
            reinterpret_cast<hw_device_t*>(const_cast<keymaster0_device_t*>(keymaster0_device_)));
    ENGINE_free(engine_);
    instance_ = nullptr;
}

bool Keymaster0Engine::GenerateRsaKey(uint64_t public_exponent, uint32_t public_modulus,
                                      KeymasterKeyBlob* key_material) const {
    assert(key_material);
    keymaster_rsa_keygen_params_t params;
    params.public_exponent = public_exponent;
    params.modulus_size = public_modulus;

    uint8_t* key_blob = 0;
    if (keymaster0_device_->generate_keypair(keymaster0_device_, TYPE_RSA, &params, &key_blob,
                                             &key_material->key_material_size) < 0) {
        ALOGE("Error generating RSA key pair with keymaster0 device");
        return false;
    }
    unique_ptr<uint8_t, Malloc_Delete> key_blob_deleter(key_blob);
    key_material->key_material = dup_buffer(key_blob, key_material->key_material_size);
    return true;
}

bool Keymaster0Engine::GenerateEcKey(uint32_t key_size, KeymasterKeyBlob* key_material) const {
    assert(key_material);
    keymaster_ec_keygen_params_t params;
    params.field_size = key_size;

    uint8_t* key_blob = 0;
    if (keymaster0_device_->generate_keypair(keymaster0_device_, TYPE_EC, &params, &key_blob,
                                             &key_material->key_material_size) < 0) {
        ALOGE("Error generating EC key pair with keymaster0 device");
        return false;
    }
    unique_ptr<uint8_t, Malloc_Delete> key_blob_deleter(key_blob);
    key_material->key_material = dup_buffer(key_blob, key_material->key_material_size);
    return true;
}

bool Keymaster0Engine::ImportKey(keymaster_key_format_t key_format,
                                 const KeymasterKeyBlob& to_import,
                                 KeymasterKeyBlob* imported_key) const {
    assert(imported_key);
    if (key_format != KM_KEY_FORMAT_PKCS8)
        return false;

    uint8_t* key_blob = 0;
    if (keymaster0_device_->import_keypair(keymaster0_device_, to_import.key_material,
                                           to_import.key_material_size, &key_blob,
                                           &imported_key->key_material_size) < 0) {
        ALOGW("Error importing keypair with keymaster0 device");
        return false;
    }
    unique_ptr<uint8_t, Malloc_Delete> key_blob_deleter(key_blob);
    imported_key->key_material = dup_buffer(key_blob, imported_key->key_material_size);
    return true;
}

bool Keymaster0Engine::DeleteKey(const KeymasterKeyBlob& blob) const {
    if (!keymaster0_device_->delete_keypair)
        return true;
    return (keymaster0_device_->delete_keypair(keymaster0_device_, blob.key_material,
                                               blob.key_material_size) == 0);
}

bool Keymaster0Engine::DeleteAllKeys() const {
    if (!keymaster0_device_->delete_all)
        return true;
    return (keymaster0_device_->delete_all(keymaster0_device_) == 0);
}

static keymaster_key_blob_t* duplicate_blob(const uint8_t* key_data, size_t key_data_size) {
    unique_ptr<uint8_t[]> key_material_copy(dup_buffer(key_data, key_data_size));
    if (!key_material_copy)
        return nullptr;

    unique_ptr<keymaster_key_blob_t> blob_copy(new (std::nothrow) keymaster_key_blob_t);
    if (!blob_copy.get())
        return nullptr;
    blob_copy->key_material_size = key_data_size;
    blob_copy->key_material = key_material_copy.release();
    return blob_copy.release();
}

inline keymaster_key_blob_t* duplicate_blob(const keymaster_key_blob_t& blob) {
    return duplicate_blob(blob.key_material, blob.key_material_size);
}

RSA* Keymaster0Engine::BlobToRsaKey(const KeymasterKeyBlob& blob) const {
    // Create new RSA key (with engine methods) and insert blob
    unique_ptr<RSA, RSA_Delete> rsa(RSA_new_method(engine_));
    if (!rsa)
        return nullptr;

    keymaster_key_blob_t* blob_copy = duplicate_blob(blob);
    if (!blob_copy->key_material || !RSA_set_ex_data(rsa.get(), rsa_index_, blob_copy))
        return nullptr;

    // Copy public key into new RSA key
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> pkey(GetKeymaster0PublicKey(blob));
    if (!pkey)
        return nullptr;
    unique_ptr<RSA, RSA_Delete> public_rsa(EVP_PKEY_get1_RSA(pkey.get()));
    if (!public_rsa)
        return nullptr;
    rsa->n = BN_dup(public_rsa->n);
    rsa->e = BN_dup(public_rsa->e);
    if (!rsa->n || !rsa->e)
        return nullptr;

    return rsa.release();
}

EC_KEY* Keymaster0Engine::BlobToEcKey(const KeymasterKeyBlob& blob) const {
    // Create new EC key (with engine methods) and insert blob
    unique_ptr<EC_KEY, EC_KEY_Delete> ec_key(EC_KEY_new_method(engine_));
    if (!ec_key)
        return nullptr;

    keymaster_key_blob_t* blob_copy = duplicate_blob(blob);
    if (!blob_copy->key_material || !EC_KEY_set_ex_data(ec_key.get(), ec_key_index_, blob_copy))
        return nullptr;

    // Copy public key into new EC key
    unique_ptr<EVP_PKEY, EVP_PKEY_Delete> pkey(GetKeymaster0PublicKey(blob));
    if (!pkey)
        return nullptr;

    unique_ptr<EC_KEY, EC_KEY_Delete> public_ec_key(EVP_PKEY_get1_EC_KEY(pkey.get()));
    if (!public_ec_key)
        return nullptr;

    if (!EC_KEY_set_group(ec_key.get(), EC_KEY_get0_group(public_ec_key.get())) ||
        !EC_KEY_set_public_key(ec_key.get(), EC_KEY_get0_public_key(public_ec_key.get())))
        return nullptr;

    return ec_key.release();
}

const keymaster_key_blob_t* Keymaster0Engine::RsaKeyToBlob(const RSA* rsa) const {
    return reinterpret_cast<keymaster_key_blob_t*>(RSA_get_ex_data(rsa, rsa_index_));
}

const keymaster_key_blob_t* Keymaster0Engine::EcKeyToBlob(const EC_KEY* ec_key) const {
    return reinterpret_cast<keymaster_key_blob_t*>(EC_KEY_get_ex_data(ec_key, ec_key_index_));
}

/* static */
int Keymaster0Engine::keyblob_dup(CRYPTO_EX_DATA* /* to */, const CRYPTO_EX_DATA* /* from */,
                                  void** from_d, int /* index */, long /* argl */,
                                  void* /* argp */) {
    keymaster_key_blob_t* blob = reinterpret_cast<keymaster_key_blob_t*>(*from_d);
    if (!blob)
        return 1;
    *from_d = duplicate_blob(*blob);
    if (*from_d)
        return 1;
    return 0;
}

/* static */
void Keymaster0Engine::keyblob_free(void* /* parent */, void* ptr, CRYPTO_EX_DATA* /* data */,
                                    int /* index*/, long /* argl */, void* /* argp */) {
    keymaster_key_blob_t* blob = reinterpret_cast<keymaster_key_blob_t*>(ptr);
    if (blob) {
        delete[] blob->key_material;
        delete blob;
    }
}

/* static */
int Keymaster0Engine::rsa_private_transform(RSA* rsa, uint8_t* out, const uint8_t* in, size_t len) {
    ALOGV("rsa_private_transform(%p, %p, %p, %u)", rsa, out, in, (unsigned)len);

    assert(instance_);
    return instance_->RsaPrivateTransform(rsa, out, in, len);
}

/* static */
int Keymaster0Engine::ecdsa_sign(const uint8_t* digest, size_t digest_len, uint8_t* sig,
                                 unsigned int* sig_len, EC_KEY* ec_key) {
    ALOGV("ecdsa_sign(%p, %u, %p)", digest, (unsigned)digest_len, ec_key);
    assert(instance_);
    return instance_->EcdsaSign(digest, digest_len, sig, sig_len, ec_key);
}

bool Keymaster0Engine::Keymaster0Sign(const void* signing_params, const keymaster_key_blob_t& blob,
                                      const uint8_t* data, const size_t data_length,
                                      unique_ptr<uint8_t[], Malloc_Delete>* signature,
                                      size_t* signature_length) const {
    uint8_t* signed_data;
    int err = keymaster0_device_->sign_data(keymaster0_device_, signing_params, blob.key_material,
                                            blob.key_material_size, data, data_length, &signed_data,
                                            signature_length);
    if (err < 0) {
        ALOGE("Keymaster0 signing failed with error %d", err);
        return false;
    }

    signature->reset(signed_data);
    return true;
}

EVP_PKEY* Keymaster0Engine::GetKeymaster0PublicKey(const KeymasterKeyBlob& blob) const {
    uint8_t* pub_key_data;
    size_t pub_key_data_length;
    int err = keymaster0_device_->get_keypair_public(keymaster0_device_, blob.key_material,
                                                     blob.key_material_size, &pub_key_data,
                                                     &pub_key_data_length);
    if (err < 0) {
        ALOGE("Error %d extracting public key", err);
        return nullptr;
    }
    unique_ptr<uint8_t, Malloc_Delete> pub_key(pub_key_data);

    const uint8_t* p = pub_key_data;
    return d2i_PUBKEY(nullptr /* allocate new struct */, &p, pub_key_data_length);
}

static bool data_too_large_for_public_modulus(const uint8_t* data, size_t len, const RSA* rsa) {
    unique_ptr<BIGNUM, BIGNUM_Delete> input_as_bn(
        BN_bin2bn(data, len, nullptr /* allocate result */));
    return input_as_bn && BN_ucmp(input_as_bn.get(), rsa->n) >= 0;
}

int Keymaster0Engine::RsaPrivateTransform(RSA* rsa, uint8_t* out, const uint8_t* in,
                                          size_t len) const {
    const keymaster_key_blob_t* key_blob = RsaKeyToBlob(rsa);
    if (key_blob == NULL) {
        ALOGE("key had no key_blob!");
        return 0;
    }

    keymaster_rsa_sign_params_t sign_params = {DIGEST_NONE, PADDING_NONE};
    unique_ptr<uint8_t[], Malloc_Delete> signature;
    size_t signature_length;
    if (!Keymaster0Sign(&sign_params, *key_blob, in, len, &signature, &signature_length)) {
        if (data_too_large_for_public_modulus(in, len, rsa)) {
            ALOGE("Keymaster0 signing failed because data is too large.");
            OPENSSL_PUT_ERROR(RSA, RSA_R_DATA_TOO_LARGE_FOR_MODULUS);
        } else {
            // We don't know what error code is correct; force an "unknown error" return
            OPENSSL_PUT_ERROR(USER, KM_ERROR_UNKNOWN_ERROR);
        }
        return 0;
    }
    Eraser eraser(signature.get(), signature_length);

    if (signature_length > len) {
        /* The result of the RSA operation can never be larger than the size of
         * the modulus so we assume that the result has extra zeros on the
         * left. This provides attackers with an oracle, but there's nothing
         * that we can do about it here. */
        memcpy(out, signature.get() + signature_length - len, len);
    } else if (signature_length < len) {
        /* If the keymaster0 implementation returns a short value we assume that
         * it's because it removed leading zeros from the left side. This is
         * bad because it provides attackers with an oracle but we cannot do
         * anything about a broken keymaster0 implementation here. */
        memset(out, 0, len);
        memcpy(out + len - signature_length, signature.get(), signature_length);
    } else {
        memcpy(out, signature.get(), len);
    }

    ALOGV("rsa=%p keystore_rsa_priv_dec successful", rsa);
    return 1;
}

int Keymaster0Engine::EcdsaSign(const uint8_t* digest, size_t digest_len, uint8_t* sig,
                                unsigned int* sig_len, EC_KEY* ec_key) const {
    const keymaster_key_blob_t* key_blob = EcKeyToBlob(ec_key);
    if (key_blob == NULL) {
        ALOGE("key had no key_blob!");
        return 0;
    }

    // Truncate digest if it's too long
    size_t max_input_len = (ec_group_size_bits(ec_key) + 7) / 8;
    if (digest_len > max_input_len)
        digest_len = max_input_len;

    keymaster_ec_sign_params_t sign_params = {DIGEST_NONE};
    unique_ptr<uint8_t[], Malloc_Delete> signature;
    size_t signature_length;
    if (!Keymaster0Sign(&sign_params, *key_blob, digest, digest_len, &signature,
                        &signature_length)) {
        // We don't know what error code is correct; force an "unknown error" return
        OPENSSL_PUT_ERROR(USER, KM_ERROR_UNKNOWN_ERROR);
        return 0;
    }
    Eraser eraser(signature.get(), signature_length);

    if (signature_length == 0) {
        ALOGW("No valid signature returned");
        return 0;
    } else if (signature_length > ECDSA_size(ec_key)) {
        ALOGW("Signature is too large");
        return 0;
    } else {
        memcpy(sig, signature.get(), signature_length);
        *sig_len = signature_length;
    }

    ALOGV("ecdsa_sign(%p, %u, %p) => success", digest, (unsigned)digest_len, ec_key);
    return 1;
}

}  // namespace keymaster

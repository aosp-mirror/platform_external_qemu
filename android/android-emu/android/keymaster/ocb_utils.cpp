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

#include "ocb_utils.h"

#include <assert.h>

#include <new>

#include <openssl/aes.h>
#include <openssl/sha.h>

#include <hardware/keymaster_defs.h>
#include <keymaster/authorization_set.h>
#include <keymaster/android_keymaster_utils.h>

#include "openssl_err.h"

namespace keymaster {

class AeCtx {
  public:
    AeCtx() : ctx_(ae_allocate(NULL)) {}
    ~AeCtx() {
        ae_clear(ctx_);
        ae_free(ctx_);
    }

    ae_ctx* get() { return ctx_; }

  private:
    ae_ctx* ctx_;
};

static keymaster_error_t BuildDerivationData(const AuthorizationSet& hw_enforced,
                                             const AuthorizationSet& sw_enforced,
                                             const AuthorizationSet& hidden,
                                             UniquePtr<uint8_t[]>* derivation_data,
                                             size_t* derivation_data_length) {
    *derivation_data_length =
        hidden.SerializedSize() + hw_enforced.SerializedSize() + sw_enforced.SerializedSize();
    derivation_data->reset(new (std::nothrow) uint8_t[*derivation_data_length]);
    if (!derivation_data->get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    uint8_t* buf = derivation_data->get();
    uint8_t* end = derivation_data->get() + *derivation_data_length;
    buf = hidden.Serialize(buf, end);
    buf = hw_enforced.Serialize(buf, end);
    buf = sw_enforced.Serialize(buf, end);

    return KM_ERROR_OK;
}

static keymaster_error_t InitializeKeyWrappingContext(const AuthorizationSet& hw_enforced,
                                                      const AuthorizationSet& sw_enforced,
                                                      const AuthorizationSet& hidden,
                                                      const KeymasterKeyBlob& master_key,
                                                      AeCtx* ctx) {
    size_t derivation_data_length;
    UniquePtr<uint8_t[]> derivation_data;
    keymaster_error_t error = BuildDerivationData(hw_enforced, sw_enforced, hidden,
                                                  &derivation_data, &derivation_data_length);
    if (error != KM_ERROR_OK)
        return error;

    SHA256_CTX sha256_ctx;
    UniquePtr<uint8_t[]> hash_buf(new (std::nothrow) uint8_t[SHA256_DIGEST_LENGTH]);
    if (!hash_buf.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    Eraser hash_eraser(hash_buf.get(), SHA256_DIGEST_LENGTH);
    UniquePtr<uint8_t[]> derived_key(new (std::nothrow) uint8_t[AES_BLOCK_SIZE]);
    if (!derived_key.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    Eraser derived_key_eraser(derived_key.get(), AES_BLOCK_SIZE);

    if (!ctx->get() || !hash_buf.get() || !derived_key.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    // Hash derivation data.
    Eraser sha256_ctx_eraser(sha256_ctx);
    SHA256_Init(&sha256_ctx);
    SHA256_Update(&sha256_ctx, derivation_data.get(), derivation_data_length);
    SHA256_Final(hash_buf.get(), &sha256_ctx);

    // Encrypt hash with master key to build derived key.
    AES_KEY aes_key;
    Eraser aes_key_eraser(AES_KEY);
    if (0 !=
        AES_set_encrypt_key(master_key.key_material, master_key.key_material_size * 8, &aes_key))
        return TranslateLastOpenSslError();

    AES_encrypt(hash_buf.get(), derived_key.get(), &aes_key);

    // Set up AES OCB context using derived key.
    if (ae_init(ctx->get(), derived_key.get(), AES_BLOCK_SIZE /* key length */, OCB_NONCE_LENGTH,
                OCB_TAG_LENGTH) != AE_SUCCESS) {
        memset_s(ctx->get(), 0, ae_ctx_sizeof());
        return KM_ERROR_UNKNOWN_ERROR;
    }

    return KM_ERROR_OK;
}

keymaster_error_t OcbEncryptKey(const AuthorizationSet& hw_enforced,
                                const AuthorizationSet& sw_enforced, const AuthorizationSet& hidden,
                                const KeymasterKeyBlob& master_key,
                                const KeymasterKeyBlob& plaintext, const Buffer& nonce,
                                KeymasterKeyBlob* ciphertext, Buffer* tag) {
    assert(ciphertext && tag);

    if (nonce.available_read() != OCB_NONCE_LENGTH)
        return KM_ERROR_INVALID_ARGUMENT;

    AeCtx ctx;
    if (!ctx.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    keymaster_error_t error =
        InitializeKeyWrappingContext(hw_enforced, sw_enforced, hidden, master_key, &ctx);
    if (error != KM_ERROR_OK)
        return error;

    if (!ciphertext->Reset(plaintext.key_material_size))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    int ae_err = ae_encrypt(ctx.get(), nonce.peek_read(), plaintext.key_material,
                            plaintext.key_material_size, NULL /* additional data */,
                            0 /* additional data length */, ciphertext->writable_data(),
                            tag->peek_write(), 1 /* final */);
    if (ae_err < 0) {
        LOG_E("Error %d while encrypting key", ae_err);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    if (!tag->advance_write(OCB_TAG_LENGTH))
        return KM_ERROR_UNKNOWN_ERROR;
    assert(ae_err == static_cast<int>(plaintext.key_material_size));
    return KM_ERROR_OK;
}

keymaster_error_t OcbDecryptKey(const AuthorizationSet& hw_enforced,
                                const AuthorizationSet& sw_enforced, const AuthorizationSet& hidden,
                                const KeymasterKeyBlob& master_key,
                                const KeymasterKeyBlob& ciphertext, const Buffer& nonce,
                                const Buffer& tag, KeymasterKeyBlob* plaintext) {
    assert(plaintext);

    if (nonce.available_read() != OCB_NONCE_LENGTH || tag.available_read() != OCB_TAG_LENGTH)
        return KM_ERROR_INVALID_ARGUMENT;

    AeCtx ctx;
    if (!ctx.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    keymaster_error_t error =
        InitializeKeyWrappingContext(hw_enforced, sw_enforced, hidden, master_key, &ctx);
    if (error != KM_ERROR_OK)
        return error;

    if (!plaintext->Reset(ciphertext.key_material_size))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    int ae_err = ae_decrypt(ctx.get(), nonce.peek_read(), ciphertext.key_material,
                            ciphertext.key_material_size, NULL /* additional data */,
                            0 /* additional data length */, plaintext->writable_data(),
                            tag.peek_read(), 1 /* final */);
    if (ae_err == AE_INVALID) {
        // Authentication failed!  Decryption probably succeeded(ish), but we don't want to return
        // any data when the authentication fails, so clear it.
        plaintext->Clear();
        LOG_E("Failed to validate authentication tag during key decryption", 0);
        return KM_ERROR_INVALID_KEY_BLOB;
    } else if (ae_err < 0) {
        LOG_E("Failed to decrypt key, error: %d", ae_err);
        return KM_ERROR_UNKNOWN_ERROR;
    }
    assert(ae_err == static_cast<int>(ciphertext.key_material_size));
    return KM_ERROR_OK;
}

}  // namespace keymaster

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

#include "integrity_assured_key_blob.h"

#include <assert.h>

#include <new>

#include <openssl/hmac.h>
#include <openssl/mem.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>

#include "openssl_err.h"

namespace keymaster {

static const uint8_t BLOB_VERSION = 0;
static const size_t HMAC_SIZE = 8;
static const char HMAC_KEY[] = "IntegrityAssuredBlob0";

inline size_t min(size_t a, size_t b) {
    if (a < b)
        return a;
    return b;
}

class HmacCleanup {
  public:
    explicit HmacCleanup(HMAC_CTX* ctx) : ctx_(ctx) {}
    ~HmacCleanup() { HMAC_CTX_cleanup(ctx_); }

  private:
    HMAC_CTX* ctx_;
};

static keymaster_error_t ComputeHmac(const uint8_t* serialized_data, size_t serialized_data_size,
                                     const AuthorizationSet& hidden, uint8_t hmac[HMAC_SIZE]) {
    size_t hidden_bytes_size = hidden.SerializedSize();
    UniquePtr<uint8_t[]> hidden_bytes(new (std::nothrow) uint8_t[hidden_bytes_size]);
    if (!hidden_bytes.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    hidden.Serialize(hidden_bytes.get(), hidden_bytes.get() + hidden_bytes_size);

    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    const EVP_MD* md = EVP_sha256();
    if (!HMAC_Init_ex(&ctx, HMAC_KEY, sizeof(HMAC_KEY), md, NULL /* engine */))
        return TranslateLastOpenSslError();
    HmacCleanup cleanup(&ctx);

    uint8_t tmp[EVP_MAX_MD_SIZE];
    unsigned tmp_len;
    if (!HMAC_Update(&ctx, serialized_data, serialized_data_size) ||
        !HMAC_Update(&ctx, hidden_bytes.get(), hidden_bytes_size) ||  //
        !HMAC_Final(&ctx, tmp, &tmp_len))
        return TranslateLastOpenSslError();

    assert(tmp_len >= HMAC_SIZE);
    memcpy(hmac, tmp, min(HMAC_SIZE, tmp_len));

    return KM_ERROR_OK;
}

keymaster_error_t SerializeIntegrityAssuredBlob(const KeymasterKeyBlob& key_material,
                                                const AuthorizationSet& hidden,
                                                const AuthorizationSet& hw_enforced,
                                                const AuthorizationSet& sw_enforced,
                                                KeymasterKeyBlob* key_blob) {
    size_t size = 1 /* version */ +                //
                  key_material.SerializedSize() +  //
                  hw_enforced.SerializedSize() +   //
                  sw_enforced.SerializedSize() +   //
                  HMAC_SIZE;

    if (!key_blob->Reset(size))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    uint8_t* p = key_blob->writable_data();
    *p++ = BLOB_VERSION;
    p = key_material.Serialize(p, key_blob->end());
    p = hw_enforced.Serialize(p, key_blob->end());
    p = sw_enforced.Serialize(p, key_blob->end());

    return ComputeHmac(key_blob->key_material, p - key_blob->key_material, hidden, p);
}

keymaster_error_t DeserializeIntegrityAssuredBlob(const KeymasterKeyBlob& key_blob,
                                                  const AuthorizationSet& hidden,
                                                  KeymasterKeyBlob* key_material,
                                                  AuthorizationSet* hw_enforced,
                                                  AuthorizationSet* sw_enforced) {
    const uint8_t* p = key_blob.begin();
    const uint8_t* end = key_blob.end();

    if (p > end || p + HMAC_SIZE > end)
        return KM_ERROR_INVALID_KEY_BLOB;

    uint8_t computed_hmac[HMAC_SIZE];
    keymaster_error_t error = ComputeHmac(key_blob.begin(), key_blob.key_material_size - HMAC_SIZE,
                                          hidden, computed_hmac);
    if (error != KM_ERROR_OK)
        return error;

    if (CRYPTO_memcmp(key_blob.end() - HMAC_SIZE, computed_hmac, HMAC_SIZE) != 0)
        return KM_ERROR_INVALID_KEY_BLOB;

    return DeserializeIntegrityAssuredBlob_NoHmacCheck(key_blob, key_material, hw_enforced,
                                                       sw_enforced);
}

keymaster_error_t DeserializeIntegrityAssuredBlob_NoHmacCheck(const KeymasterKeyBlob& key_blob,
                                                              KeymasterKeyBlob* key_material,
                                                              AuthorizationSet* hw_enforced,
                                                              AuthorizationSet* sw_enforced) {
    const uint8_t* p = key_blob.begin();
    const uint8_t* end = key_blob.end() - HMAC_SIZE;

    if (p > end)
        return KM_ERROR_INVALID_KEY_BLOB;

    if (*p != BLOB_VERSION)
        return KM_ERROR_INVALID_KEY_BLOB;
    ++p;

    if (!key_material->Deserialize(&p, end) ||  //
        !hw_enforced->Deserialize(&p, end) ||   //
        !sw_enforced->Deserialize(&p, end))
        return KM_ERROR_INVALID_KEY_BLOB;

    return KM_ERROR_OK;
}

}  // namespace keymaster;

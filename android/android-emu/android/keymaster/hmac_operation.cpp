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

#include "hmac_operation.h"

#include <new>

#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "hmac_key.h"
#include "openssl_err.h"

#if defined(OPENSSL_IS_BORINGSSL)
#include <openssl/mem.h>
typedef size_t openssl_size_t;
#else
typedef int openssl_size_t;
#endif

namespace keymaster {

Operation* HmacOperationFactory::CreateOperation(const Key& key,
                                                 const AuthorizationSet& begin_params,
                                                 keymaster_error_t* error) {
    uint32_t min_mac_length_bits;
    if (!key.authorizations().GetTagValue(TAG_MIN_MAC_LENGTH, &min_mac_length_bits)) {
        LOG_E("HMAC key must have KM_TAG_MIN_MAC_LENGTH", 0);
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return nullptr;
    }

    uint32_t mac_length_bits = UINT32_MAX;
    if (begin_params.GetTagValue(TAG_MAC_LENGTH, &mac_length_bits)) {
        if (purpose() == KM_PURPOSE_VERIFY) {
            LOG_E("MAC length may not be specified for verify", 0);
            *error = KM_ERROR_INVALID_ARGUMENT;
            return nullptr;
        }
    } else {
        if (purpose() == KM_PURPOSE_SIGN) {
            *error = KM_ERROR_MISSING_MAC_LENGTH;
            return nullptr;
        }
    }

    keymaster_digest_t digest;
    if (!key.authorizations().GetTagValue(TAG_DIGEST, &digest)) {
        LOG_E("%d digests found in HMAC key authorizations; must be exactly 1",
              begin_params.GetTagCount(TAG_DIGEST));
        *error = KM_ERROR_INVALID_KEY_BLOB;
        return nullptr;
    }

    const SymmetricKey* symmetric_key = static_cast<const SymmetricKey*>(&key);
    UniquePtr<HmacOperation> op(new (std::nothrow) HmacOperation(
        purpose(), symmetric_key->key_data(), symmetric_key->key_data_size(), digest,
        mac_length_bits / 8, min_mac_length_bits / 8));
    if (!op.get())
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    else
        *error = op->error();

    if (*error != KM_ERROR_OK)
        return nullptr;

    return op.release();
}

static keymaster_digest_t supported_digests[] = {KM_DIGEST_SHA1, KM_DIGEST_SHA_2_224,
                                                 KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384,
                                                 KM_DIGEST_SHA_2_512};
const keymaster_digest_t* HmacOperationFactory::SupportedDigests(size_t* digest_count) const {
    *digest_count = array_length(supported_digests);
    return supported_digests;
}

HmacOperation::HmacOperation(keymaster_purpose_t purpose, const uint8_t* key_data,
                             size_t key_data_size, keymaster_digest_t digest, size_t mac_length,
                             size_t min_mac_length)
    : Operation(purpose), error_(KM_ERROR_OK), mac_length_(mac_length),
      min_mac_length_(min_mac_length) {
    // Initialize CTX first, so dtor won't crash even if we error out later.
    HMAC_CTX_init(&ctx_);

    const EVP_MD* md = nullptr;
    switch (digest) {
    case KM_DIGEST_NONE:
    case KM_DIGEST_MD5:
        error_ = KM_ERROR_UNSUPPORTED_DIGEST;
        break;
    case KM_DIGEST_SHA1:
        md = EVP_sha1();
        break;
    case KM_DIGEST_SHA_2_224:
        md = EVP_sha224();
        break;
    case KM_DIGEST_SHA_2_256:
        md = EVP_sha256();
        break;
    case KM_DIGEST_SHA_2_384:
        md = EVP_sha384();
        break;
    case KM_DIGEST_SHA_2_512:
        md = EVP_sha512();
        break;
    }

    if (md == nullptr) {
        error_ = KM_ERROR_UNSUPPORTED_DIGEST;
        return;
    }

    if (purpose == KM_PURPOSE_SIGN) {
        if (mac_length > EVP_MD_size(md) || mac_length < kMinHmacLengthBits / 8) {
            error_ = KM_ERROR_UNSUPPORTED_MAC_LENGTH;
            return;
        }
        if (mac_length < min_mac_length) {
            error_ = KM_ERROR_INVALID_MAC_LENGTH;
            return;
        }
    }

    HMAC_Init_ex(&ctx_, key_data, key_data_size, md, NULL /* engine */);
}

HmacOperation::~HmacOperation() {
    HMAC_CTX_cleanup(&ctx_);
}

keymaster_error_t HmacOperation::Begin(const AuthorizationSet& /* input_params */,
                                       AuthorizationSet* /* output_params */) {
    return error_;
}

keymaster_error_t HmacOperation::Update(const AuthorizationSet& /* additional_params */,
                                        const Buffer& input, AuthorizationSet* /* output_params */,
                                        Buffer* /* output */, size_t* input_consumed) {
    if (!HMAC_Update(&ctx_, input.peek_read(), input.available_read()))
        return TranslateLastOpenSslError();
    *input_consumed = input.available_read();
    return KM_ERROR_OK;
}

keymaster_error_t HmacOperation::Abort() {
    return KM_ERROR_OK;
}

keymaster_error_t HmacOperation::Finish(const AuthorizationSet& additional_params,
                                        const Buffer& input, const Buffer& signature,
                                        AuthorizationSet* /* output_params */, Buffer* output) {
    keymaster_error_t error = UpdateForFinish(additional_params, input);
    if (error != KM_ERROR_OK)
        return error;

    uint8_t digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    if (!HMAC_Final(&ctx_, digest, &digest_len))
        return TranslateLastOpenSslError();

    switch (purpose()) {
    case KM_PURPOSE_SIGN:
        if (mac_length_ > digest_len)
            return KM_ERROR_UNSUPPORTED_MAC_LENGTH;
        if (!output->reserve(mac_length_) || !output->write(digest, mac_length_))
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return KM_ERROR_OK;
    case KM_PURPOSE_VERIFY: {
        size_t siglen = signature.available_read();
        if (siglen > digest_len || siglen < kMinHmacLengthBits / 8)
            return KM_ERROR_UNSUPPORTED_MAC_LENGTH;
        if (siglen < min_mac_length_)
            return KM_ERROR_INVALID_MAC_LENGTH;
        if (CRYPTO_memcmp(signature.peek_read(), digest, siglen) != 0)
            return KM_ERROR_VERIFICATION_FAILED;
        return KM_ERROR_OK;
    }
    default:
        return KM_ERROR_UNSUPPORTED_PURPOSE;
    }
}

}  // namespace keymaster

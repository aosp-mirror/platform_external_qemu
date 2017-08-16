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

#include "ecdsa_operation.h"

#include <openssl/ecdsa.h>

#include "ec_key.h"
#include "openssl_err.h"
#include "openssl_utils.h"

namespace keymaster {

static const keymaster_digest_t supported_digests[] = {KM_DIGEST_NONE,      KM_DIGEST_SHA1,
                                                       KM_DIGEST_SHA_2_224, KM_DIGEST_SHA_2_256,
                                                       KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512};

Operation* EcdsaOperationFactory::CreateOperation(const Key& key,
                                                  const AuthorizationSet& begin_params,
                                                  keymaster_error_t* error) {
    const EcKey* ecdsa_key = static_cast<const EcKey*>(&key);
    if (!ecdsa_key) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }

    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey(EVP_PKEY_new());
    if (!ecdsa_key->InternalToEvp(pkey.get())) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }

    keymaster_digest_t digest;
    if (!GetAndValidateDigest(begin_params, key, &digest, error))
        return nullptr;

    *error = KM_ERROR_OK;
    Operation* op = InstantiateOperation(digest, pkey.release());
    if (!op)
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return op;
}

const keymaster_digest_t* EcdsaOperationFactory::SupportedDigests(size_t* digest_count) const {
    *digest_count = array_length(supported_digests);
    return supported_digests;
}

EcdsaOperation::~EcdsaOperation() {
    if (ecdsa_key_ != NULL)
        EVP_PKEY_free(ecdsa_key_);
    EVP_MD_CTX_cleanup(&digest_ctx_);
}

keymaster_error_t EcdsaOperation::InitDigest() {
    switch (digest_) {
    case KM_DIGEST_NONE:
        return KM_ERROR_OK;
    case KM_DIGEST_MD5:
        return KM_ERROR_UNSUPPORTED_DIGEST;
    case KM_DIGEST_SHA1:
        digest_algorithm_ = EVP_sha1();
        return KM_ERROR_OK;
    case KM_DIGEST_SHA_2_224:
        digest_algorithm_ = EVP_sha224();
        return KM_ERROR_OK;
    case KM_DIGEST_SHA_2_256:
        digest_algorithm_ = EVP_sha256();
        return KM_ERROR_OK;
    case KM_DIGEST_SHA_2_384:
        digest_algorithm_ = EVP_sha384();
        return KM_ERROR_OK;
    case KM_DIGEST_SHA_2_512:
        digest_algorithm_ = EVP_sha512();
        return KM_ERROR_OK;
    default:
        return KM_ERROR_UNSUPPORTED_DIGEST;
    }
}

inline size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

keymaster_error_t EcdsaOperation::StoreData(const Buffer& input, size_t* input_consumed) {
    if (!data_.reserve((EVP_PKEY_bits(ecdsa_key_) + 7) / 8))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    if (!data_.write(input.peek_read(), min(data_.available_write(), input.available_read())))
        return KM_ERROR_UNKNOWN_ERROR;

    *input_consumed = input.available_read();
    return KM_ERROR_OK;
}

keymaster_error_t EcdsaSignOperation::Begin(const AuthorizationSet& /* input_params */,
                                            AuthorizationSet* /* output_params */) {
    keymaster_error_t error = InitDigest();
    if (error != KM_ERROR_OK)
        return error;

    if (digest_ == KM_DIGEST_NONE)
        return KM_ERROR_OK;

    EVP_PKEY_CTX* pkey_ctx;
    if (EVP_DigestSignInit(&digest_ctx_, &pkey_ctx, digest_algorithm_, nullptr /* engine */,
                           ecdsa_key_) != 1)
        return TranslateLastOpenSslError();
    return KM_ERROR_OK;
}

keymaster_error_t EcdsaSignOperation::Update(const AuthorizationSet& /* additional_params */,
                                             const Buffer& input,
                                             AuthorizationSet* /* output_params */,
                                             Buffer* /* output */, size_t* input_consumed) {
    if (digest_ == KM_DIGEST_NONE)
        return StoreData(input, input_consumed);

    if (EVP_DigestSignUpdate(&digest_ctx_, input.peek_read(), input.available_read()) != 1)
        return TranslateLastOpenSslError();
    *input_consumed = input.available_read();
    return KM_ERROR_OK;
}

keymaster_error_t EcdsaSignOperation::Finish(const AuthorizationSet& additional_params,
                                             const Buffer& input, const Buffer& /* signature */,
                                             AuthorizationSet* /* output_params */,
                                             Buffer* output) {
    if (!output)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    keymaster_error_t error = UpdateForFinish(additional_params, input);
    if (error != KM_ERROR_OK)
        return error;

    size_t siglen;
    if (digest_ == KM_DIGEST_NONE) {
        UniquePtr<EC_KEY, EC_KEY_Delete> ecdsa(EVP_PKEY_get1_EC_KEY(ecdsa_key_));
        if (!ecdsa.get())
            return TranslateLastOpenSslError();

        output->Reinitialize(ECDSA_size(ecdsa.get()));
        unsigned int siglen_tmp;
        if (!ECDSA_sign(0 /* type -- ignored */, data_.peek_read(), data_.available_read(),
                        output->peek_write(), &siglen_tmp, ecdsa.get()))
            return TranslateLastOpenSslError();
        siglen = siglen_tmp;
    } else {
        if (EVP_DigestSignFinal(&digest_ctx_, nullptr /* signature */, &siglen) != 1)
            return TranslateLastOpenSslError();
        if (!output->Reinitialize(siglen))
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
        if (EVP_DigestSignFinal(&digest_ctx_, output->peek_write(), &siglen) <= 0)
            return TranslateLastOpenSslError();
    }
    if (!output->advance_write(siglen))
        return KM_ERROR_UNKNOWN_ERROR;
    return KM_ERROR_OK;
}

keymaster_error_t EcdsaVerifyOperation::Begin(const AuthorizationSet& /* input_params */,
                                              AuthorizationSet* /* output_params */) {
    keymaster_error_t error = InitDigest();
    if (error != KM_ERROR_OK)
        return error;

    if (digest_ == KM_DIGEST_NONE)
        return KM_ERROR_OK;

    EVP_PKEY_CTX* pkey_ctx;
    if (EVP_DigestVerifyInit(&digest_ctx_, &pkey_ctx, digest_algorithm_, nullptr /* engine */,
                             ecdsa_key_) != 1)
        return TranslateLastOpenSslError();
    return KM_ERROR_OK;
}

keymaster_error_t EcdsaVerifyOperation::Update(const AuthorizationSet& /* additional_params */,
                                               const Buffer& input,
                                               AuthorizationSet* /* output_params */,
                                               Buffer* /* output */, size_t* input_consumed) {
    if (digest_ == KM_DIGEST_NONE)
        return StoreData(input, input_consumed);

    if (EVP_DigestVerifyUpdate(&digest_ctx_, input.peek_read(), input.available_read()) != 1)
        return TranslateLastOpenSslError();
    *input_consumed = input.available_read();
    return KM_ERROR_OK;
}

keymaster_error_t EcdsaVerifyOperation::Finish(const AuthorizationSet& additional_params,
                                               const Buffer& input, const Buffer& signature,
                                               AuthorizationSet* /* output_params */,
                                               Buffer* /* output */) {
    keymaster_error_t error = UpdateForFinish(additional_params, input);
    if (error != KM_ERROR_OK)
        return error;

    if (digest_ == KM_DIGEST_NONE) {
        UniquePtr<EC_KEY, EC_KEY_Delete> ecdsa(EVP_PKEY_get1_EC_KEY(ecdsa_key_));
        if (!ecdsa.get())
            return TranslateLastOpenSslError();

        int result =
            ECDSA_verify(0 /* type -- ignored */, data_.peek_read(), data_.available_read(),
                         signature.peek_read(), signature.available_read(), ecdsa.get());
        if (result < 0)
            return TranslateLastOpenSslError();
        else if (result == 0)
            return KM_ERROR_VERIFICATION_FAILED;
    } else if (!EVP_DigestVerifyFinal(&digest_ctx_, signature.peek_read(),
                                      signature.available_read()))
        return KM_ERROR_VERIFICATION_FAILED;

    return KM_ERROR_OK;
}

}  // namespace keymaster

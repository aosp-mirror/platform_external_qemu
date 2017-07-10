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

#include "rsa_operation.h"

#include <limits.h>

#include <new>

#include <openssl/err.h>

#include <keymaster/logger.h>

#include "openssl_err.h"
#include "openssl_utils.h"
#include "rsa_key.h"

namespace keymaster {

const size_t kPssOverhead = 2;

// Overhead for PKCS#1 v1.5 signature padding of undigested messages.  Digested messages have
// additional overhead, for the digest algorithmIdentifier required by PKCS#1.
const size_t kPkcs1UndigestedSignaturePaddingOverhead = 11;

/* static */
EVP_PKEY* RsaOperationFactory::GetRsaKey(const Key& key, keymaster_error_t* error) {
    const RsaKey* rsa_key = static_cast<const RsaKey*>(&key);
    assert(rsa_key);
    if (!rsa_key || !rsa_key->key()) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }

    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> pkey(EVP_PKEY_new());
    if (!rsa_key->InternalToEvp(pkey.get())) {
        *error = KM_ERROR_UNKNOWN_ERROR;
        return nullptr;
    }
    return pkey.release();
}

static const keymaster_digest_t supported_digests[] = {
    KM_DIGEST_NONE,      KM_DIGEST_MD5,       KM_DIGEST_SHA1,     KM_DIGEST_SHA_2_224,
    KM_DIGEST_SHA_2_256, KM_DIGEST_SHA_2_384, KM_DIGEST_SHA_2_512};

const keymaster_digest_t* RsaOperationFactory::SupportedDigests(size_t* digest_count) const {
    *digest_count = array_length(supported_digests);
    return supported_digests;
}

RsaOperation* RsaOperationFactory::CreateRsaOperation(const Key& key,
                                                      const AuthorizationSet& begin_params,
                                                      keymaster_error_t* error) {
    keymaster_padding_t padding;
    if (!GetAndValidatePadding(begin_params, key, &padding, error))
        return nullptr;

    bool require_digest = (purpose() == KM_PURPOSE_SIGN || purpose() == KM_PURPOSE_VERIFY ||
                           padding == KM_PAD_RSA_OAEP);

    keymaster_digest_t digest = KM_DIGEST_NONE;
    if (require_digest && !GetAndValidateDigest(begin_params, key, &digest, error))
        return nullptr;

    UniquePtr<EVP_PKEY, EVP_PKEY_Delete> rsa(GetRsaKey(key, error));
    if (!rsa.get())
        return nullptr;

    RsaOperation* op = InstantiateOperation(digest, padding, rsa.release());
    if (!op)
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return op;
}

static const keymaster_padding_t supported_sig_padding[] = {KM_PAD_NONE, KM_PAD_RSA_PKCS1_1_5_SIGN,
                                                            KM_PAD_RSA_PSS};
const keymaster_padding_t*
RsaDigestingOperationFactory::SupportedPaddingModes(size_t* padding_mode_count) const {
    *padding_mode_count = array_length(supported_sig_padding);
    return supported_sig_padding;
}

RsaOperation* RsaCryptingOperationFactory::CreateRsaOperation(const Key& key,
                                                              const AuthorizationSet& begin_params,
                                                              keymaster_error_t* error) {
    UniquePtr<RsaOperation> op(RsaOperationFactory::CreateRsaOperation(key, begin_params, error));
    if (op.get()) {
        switch (op->padding()) {
        case KM_PAD_NONE:
        case KM_PAD_RSA_PKCS1_1_5_ENCRYPT:
            if (op->digest() != KM_DIGEST_NONE) {
                *error = KM_ERROR_INCOMPATIBLE_DIGEST;
                return nullptr;
            }
            break;

        case KM_PAD_RSA_OAEP:
            if (op->digest() == KM_DIGEST_NONE) {
                *error = KM_ERROR_INCOMPATIBLE_DIGEST;
                return nullptr;
            }
            break;

        default:
            *error = KM_ERROR_UNSUPPORTED_PADDING_MODE;
            return nullptr;
        }
    }
    return op.release();
}

static const keymaster_padding_t supported_crypt_padding[] = {KM_PAD_NONE, KM_PAD_RSA_OAEP,
                                                              KM_PAD_RSA_PKCS1_1_5_ENCRYPT};
const keymaster_padding_t*
RsaCryptingOperationFactory::SupportedPaddingModes(size_t* padding_mode_count) const {
    *padding_mode_count = array_length(supported_crypt_padding);
    return supported_crypt_padding;
}

RsaOperation::~RsaOperation() {
    if (rsa_key_ != NULL)
        EVP_PKEY_free(rsa_key_);
}

keymaster_error_t RsaOperation::Begin(const AuthorizationSet& /* input_params */,
                                      AuthorizationSet* /* output_params */) {
    return InitDigest();
}

keymaster_error_t RsaOperation::Update(const AuthorizationSet& /* additional_params */,
                                       const Buffer& input, AuthorizationSet* /* output_params */,
                                       Buffer* /* output */, size_t* input_consumed) {
    assert(input_consumed);
    switch (purpose()) {
    default:
        return KM_ERROR_UNIMPLEMENTED;
    case KM_PURPOSE_SIGN:
    case KM_PURPOSE_VERIFY:
    case KM_PURPOSE_ENCRYPT:
    case KM_PURPOSE_DECRYPT:
        return StoreData(input, input_consumed);
    }
}

keymaster_error_t RsaOperation::StoreData(const Buffer& input, size_t* input_consumed) {
    assert(input_consumed);

    if (!data_.reserve(EVP_PKEY_size(rsa_key_)))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    // If the write fails, it's because input length exceeds key size.
    if (!data_.write(input.peek_read(), input.available_read())) {
        LOG_E("Input too long: cannot operate on %u bytes of data with %u-byte RSA key",
              input.available_read() + data_.available_read(), EVP_PKEY_size(rsa_key_));
        return KM_ERROR_INVALID_INPUT_LENGTH;
    }

    *input_consumed = input.available_read();
    return KM_ERROR_OK;
}

keymaster_error_t RsaOperation::SetRsaPaddingInEvpContext(EVP_PKEY_CTX* pkey_ctx, bool signing) {
    keymaster_error_t error;
    int openssl_padding = GetOpensslPadding(&error);
    if (error != KM_ERROR_OK)
        return error;

    if (EVP_PKEY_CTX_set_rsa_padding(pkey_ctx, openssl_padding) <= 0)
        return TranslateLastOpenSslError();

    if (signing && openssl_padding == RSA_PKCS1_PSS_PADDING) {
        // Also need to set the length of the salt used in the padding generation.  We set it equal
        // to the length of the selected digest.
        assert(digest_algorithm_);
        if (EVP_PKEY_CTX_set_rsa_pss_saltlen(pkey_ctx, EVP_MD_size(digest_algorithm_)) <= 0)
            return TranslateLastOpenSslError();
    }

    return KM_ERROR_OK;
}

keymaster_error_t RsaOperation::InitDigest() {
    if (digest_ == KM_DIGEST_NONE) {
        if (require_digest())
            return KM_ERROR_INCOMPATIBLE_DIGEST;
        return KM_ERROR_OK;
    }

    switch (digest_) {
    case KM_DIGEST_NONE:
        return KM_ERROR_OK;
    case KM_DIGEST_MD5:
        digest_algorithm_ = EVP_md5();
        return KM_ERROR_OK;
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

RsaDigestingOperation::RsaDigestingOperation(keymaster_purpose_t purpose, keymaster_digest_t digest,
                                             keymaster_padding_t padding, EVP_PKEY* key)
    : RsaOperation(purpose, digest, padding, key) {
    EVP_MD_CTX_init(&digest_ctx_);
}
RsaDigestingOperation::~RsaDigestingOperation() {
    EVP_MD_CTX_cleanup(&digest_ctx_);
}

int RsaDigestingOperation::GetOpensslPadding(keymaster_error_t* error) {
    *error = KM_ERROR_OK;
    switch (padding_) {
    case KM_PAD_NONE:
        return RSA_NO_PADDING;
    case KM_PAD_RSA_PKCS1_1_5_SIGN:
        return RSA_PKCS1_PADDING;
    case KM_PAD_RSA_PSS:
        if (digest_ == KM_DIGEST_NONE) {
            *error = KM_ERROR_INCOMPATIBLE_PADDING_MODE;
            return -1;
        }
        if (EVP_MD_size(digest_algorithm_) * 2 + kPssOverhead > (size_t)EVP_PKEY_size(rsa_key_)) {
            LOG_E("Input too long: %d-byte digest cannot be used with %d-byte RSA key in PSS "
                  "padding mode",
                  EVP_MD_size(digest_algorithm_), EVP_PKEY_size(rsa_key_));
            *error = KM_ERROR_INCOMPATIBLE_DIGEST;
            return -1;
        }
        return RSA_PKCS1_PSS_PADDING;
    default:
        return -1;
    }
}

keymaster_error_t RsaSignOperation::Begin(const AuthorizationSet& input_params,
                                          AuthorizationSet* output_params) {
    keymaster_error_t error = RsaDigestingOperation::Begin(input_params, output_params);
    if (error != KM_ERROR_OK)
        return error;

    if (digest_ == KM_DIGEST_NONE)
        return KM_ERROR_OK;

    EVP_PKEY_CTX* pkey_ctx;
    if (EVP_DigestSignInit(&digest_ctx_, &pkey_ctx, digest_algorithm_, nullptr /* engine */,
                           rsa_key_) != 1)
        return TranslateLastOpenSslError();
    return SetRsaPaddingInEvpContext(pkey_ctx, true /* signing */);
}

keymaster_error_t RsaSignOperation::Update(const AuthorizationSet& additional_params,
                                           const Buffer& input, AuthorizationSet* output_params,
                                           Buffer* output, size_t* input_consumed) {
    if (digest_ == KM_DIGEST_NONE)
        // Just buffer the data.
        return RsaOperation::Update(additional_params, input, output_params, output,
                                    input_consumed);

    if (EVP_DigestSignUpdate(&digest_ctx_, input.peek_read(), input.available_read()) != 1)
        return TranslateLastOpenSslError();
    *input_consumed = input.available_read();
    return KM_ERROR_OK;
}

keymaster_error_t RsaSignOperation::Finish(const AuthorizationSet& additional_params,
                                           const Buffer& input, const Buffer& /* signature */,
                                           AuthorizationSet* /* output_params */, Buffer* output) {
    assert(output);

    keymaster_error_t error = UpdateForFinish(additional_params, input);
    if (error != KM_ERROR_OK)
        return error;

    if (digest_ == KM_DIGEST_NONE)
        return SignUndigested(output);
    else
        return SignDigested(output);
}

static keymaster_error_t zero_pad_left(UniquePtr<uint8_t[]>* dest, size_t padded_len, Buffer& src) {
    assert(padded_len > src.available_read());

    dest->reset(new uint8_t[padded_len]);
    if (!dest->get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    size_t padding_len = padded_len - src.available_read();
    memset(dest->get(), 0, padding_len);
    if (!src.read(dest->get() + padding_len, src.available_read()))
        return KM_ERROR_UNKNOWN_ERROR;

    return KM_ERROR_OK;
}

keymaster_error_t RsaSignOperation::SignUndigested(Buffer* output) {
    UniquePtr<RSA, RSA_Delete> rsa(EVP_PKEY_get1_RSA(const_cast<EVP_PKEY*>(rsa_key_)));
    if (!rsa.get())
        return TranslateLastOpenSslError();

    if (!output->Reinitialize(RSA_size(rsa.get())))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    size_t key_len = EVP_PKEY_size(rsa_key_);
    int bytes_encrypted;
    switch (padding_) {
    case KM_PAD_NONE: {
        const uint8_t* to_encrypt = data_.peek_read();
        UniquePtr<uint8_t[]> zero_padded;
        if (data_.available_read() > key_len) {
            return KM_ERROR_INVALID_INPUT_LENGTH;
        } else if (data_.available_read() < key_len) {
            keymaster_error_t error = zero_pad_left(&zero_padded, key_len, data_);
            if (error != KM_ERROR_OK)
                return error;
            to_encrypt = zero_padded.get();
        }
        bytes_encrypted = RSA_private_encrypt(key_len, to_encrypt, output->peek_write(), rsa.get(),
                                              RSA_NO_PADDING);
        break;
    }
    case KM_PAD_RSA_PKCS1_1_5_SIGN:
        // Does PKCS1 padding without digesting even make sense?  Dunno.  We'll support it.
        if (data_.available_read() + kPkcs1UndigestedSignaturePaddingOverhead > key_len) {
            LOG_E("Input too long: cannot sign %u-byte message with PKCS1 padding with %u-bit key",
                  data_.available_read(), EVP_PKEY_size(rsa_key_) * 8);
            return KM_ERROR_INVALID_INPUT_LENGTH;
        }
        bytes_encrypted = RSA_private_encrypt(data_.available_read(), data_.peek_read(),
                                              output->peek_write(), rsa.get(), RSA_PKCS1_PADDING);
        break;

    default:
        return KM_ERROR_UNSUPPORTED_PADDING_MODE;
    }

    if (bytes_encrypted <= 0)
        return TranslateLastOpenSslError();
    if (!output->advance_write(bytes_encrypted))
        return KM_ERROR_UNKNOWN_ERROR;
    return KM_ERROR_OK;
}

keymaster_error_t RsaSignOperation::SignDigested(Buffer* output) {
    size_t siglen;
    if (EVP_DigestSignFinal(&digest_ctx_, nullptr /* signature */, &siglen) != 1)
        return TranslateLastOpenSslError();

    if (!output->Reinitialize(siglen))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    if (EVP_DigestSignFinal(&digest_ctx_, output->peek_write(), &siglen) <= 0)
        return TranslateLastOpenSslError();
    if (!output->advance_write(siglen))
        return KM_ERROR_UNKNOWN_ERROR;

    return KM_ERROR_OK;
}

keymaster_error_t RsaVerifyOperation::Begin(const AuthorizationSet& input_params,
                                            AuthorizationSet* output_params) {
    keymaster_error_t error = RsaDigestingOperation::Begin(input_params, output_params);
    if (error != KM_ERROR_OK)
        return error;

    if (digest_ == KM_DIGEST_NONE)
        return KM_ERROR_OK;

    EVP_PKEY_CTX* pkey_ctx;
    if (EVP_DigestVerifyInit(&digest_ctx_, &pkey_ctx, digest_algorithm_, NULL, rsa_key_) != 1)
        return TranslateLastOpenSslError();
    return SetRsaPaddingInEvpContext(pkey_ctx, false /* signing */);
}

keymaster_error_t RsaVerifyOperation::Update(const AuthorizationSet& additional_params,
                                             const Buffer& input, AuthorizationSet* output_params,
                                             Buffer* output, size_t* input_consumed) {
    if (digest_ == KM_DIGEST_NONE)
        // Just buffer the data.
        return RsaOperation::Update(additional_params, input, output_params, output,
                                    input_consumed);

    if (EVP_DigestVerifyUpdate(&digest_ctx_, input.peek_read(), input.available_read()) != 1)
        return TranslateLastOpenSslError();
    *input_consumed = input.available_read();
    return KM_ERROR_OK;
}

keymaster_error_t RsaVerifyOperation::Finish(const AuthorizationSet& additional_params,
                                             const Buffer& input, const Buffer& signature,
                                             AuthorizationSet* /* output_params */,
                                             Buffer* /* output */) {
    keymaster_error_t error = UpdateForFinish(additional_params, input);
    if (error != KM_ERROR_OK)
        return error;

    if (digest_ == KM_DIGEST_NONE)
        return VerifyUndigested(signature);
    else
        return VerifyDigested(signature);
}

keymaster_error_t RsaVerifyOperation::VerifyUndigested(const Buffer& signature) {
    UniquePtr<RSA, RSA_Delete> rsa(EVP_PKEY_get1_RSA(const_cast<EVP_PKEY*>(rsa_key_)));
    if (!rsa.get())
        return KM_ERROR_UNKNOWN_ERROR;

    size_t key_len = RSA_size(rsa.get());
    int openssl_padding;
    switch (padding_) {
    case KM_PAD_NONE:
        if (data_.available_read() > key_len)
            return KM_ERROR_INVALID_INPUT_LENGTH;
        if (key_len != signature.available_read())
            return KM_ERROR_VERIFICATION_FAILED;
        openssl_padding = RSA_NO_PADDING;
        break;
    case KM_PAD_RSA_PKCS1_1_5_SIGN:
        if (data_.available_read() + kPkcs1UndigestedSignaturePaddingOverhead > key_len) {
            LOG_E("Input too long: cannot verify %u-byte message with PKCS1 padding && %u-bit key",
                  data_.available_read(), key_len * 8);
            return KM_ERROR_INVALID_INPUT_LENGTH;
        }
        openssl_padding = RSA_PKCS1_PADDING;
        break;
    default:
        return KM_ERROR_UNSUPPORTED_PADDING_MODE;
    }

    UniquePtr<uint8_t[]> decrypted_data(new (std::nothrow) uint8_t[key_len]);
    if (!decrypted_data.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    int bytes_decrypted = RSA_public_decrypt(signature.available_read(), signature.peek_read(),
                                             decrypted_data.get(), rsa.get(), openssl_padding);
    if (bytes_decrypted < 0)
        return KM_ERROR_VERIFICATION_FAILED;

    const uint8_t* compare_pos = decrypted_data.get();
    size_t bytes_to_compare = bytes_decrypted;
    uint8_t zero_check_result = 0;
    if (padding_ == KM_PAD_NONE && data_.available_read() < bytes_to_compare) {
        // If the data is short, for "unpadded" signing we zero-pad to the left.  So during
        // verification we should have zeros on the left of the decrypted data.  Do a constant-time
        // check.
        const uint8_t* zero_end = compare_pos + bytes_to_compare - data_.available_read();
        while (compare_pos < zero_end)
            zero_check_result |= *compare_pos++;
        bytes_to_compare = data_.available_read();
    }
    if (memcmp_s(compare_pos, data_.peek_read(), bytes_to_compare) != 0 || zero_check_result != 0)
        return KM_ERROR_VERIFICATION_FAILED;
    return KM_ERROR_OK;
}

keymaster_error_t RsaVerifyOperation::VerifyDigested(const Buffer& signature) {
    if (!EVP_DigestVerifyFinal(&digest_ctx_, signature.peek_read(), signature.available_read()))
        return KM_ERROR_VERIFICATION_FAILED;
    return KM_ERROR_OK;
}

keymaster_error_t RsaCryptOperation::SetOaepDigestIfRequired(EVP_PKEY_CTX* pkey_ctx) {
    if (padding() != KM_PAD_RSA_OAEP)
        return KM_ERROR_OK;

    assert(digest_algorithm_ != nullptr);
    if (!EVP_PKEY_CTX_set_rsa_oaep_md(pkey_ctx, digest_algorithm_))
        return TranslateLastOpenSslError();

    // MGF1 MD is always SHA1.
    if (!EVP_PKEY_CTX_set_rsa_mgf1_md(pkey_ctx, EVP_sha1()))
        return TranslateLastOpenSslError();

    return KM_ERROR_OK;
}

int RsaCryptOperation::GetOpensslPadding(keymaster_error_t* error) {
    *error = KM_ERROR_OK;
    switch (padding_) {
    case KM_PAD_NONE:
        return RSA_NO_PADDING;
    case KM_PAD_RSA_PKCS1_1_5_ENCRYPT:
        return RSA_PKCS1_PADDING;
    case KM_PAD_RSA_OAEP:
        return RSA_PKCS1_OAEP_PADDING;
    default:
        return -1;
    }
}

struct EVP_PKEY_CTX_Delete {
    void operator()(EVP_PKEY_CTX* p) { EVP_PKEY_CTX_free(p); }
};

keymaster_error_t RsaEncryptOperation::Finish(const AuthorizationSet& additional_params,
                                              const Buffer& input, const Buffer& /* signature */,
                                              AuthorizationSet* /* output_params */,
                                              Buffer* output) {
    if (!output)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    keymaster_error_t error = UpdateForFinish(additional_params, input);
    if (error != KM_ERROR_OK)
        return error;

    UniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_Delete> ctx(
        EVP_PKEY_CTX_new(rsa_key_, nullptr /* engine */));
    if (!ctx.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    if (EVP_PKEY_encrypt_init(ctx.get()) <= 0)
        return TranslateLastOpenSslError();

    error = SetRsaPaddingInEvpContext(ctx.get(), false /* signing */);
    if (error != KM_ERROR_OK)
        return error;
    error = SetOaepDigestIfRequired(ctx.get());
    if (error != KM_ERROR_OK)
        return error;

    size_t outlen;
    if (EVP_PKEY_encrypt(ctx.get(), nullptr /* out */, &outlen, data_.peek_read(),
                         data_.available_read()) <= 0)
        return TranslateLastOpenSslError();

    if (!output->Reinitialize(outlen))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    const uint8_t* to_encrypt = data_.peek_read();
    size_t to_encrypt_len = data_.available_read();
    UniquePtr<uint8_t[]> zero_padded;
    if (padding_ == KM_PAD_NONE && to_encrypt_len < outlen) {
        keymaster_error_t error = zero_pad_left(&zero_padded, outlen, data_);
        if (error != KM_ERROR_OK)
            return error;
        to_encrypt = zero_padded.get();
        to_encrypt_len = outlen;
    }

    if (EVP_PKEY_encrypt(ctx.get(), output->peek_write(), &outlen, to_encrypt, to_encrypt_len) <= 0)
        return TranslateLastOpenSslError();
    if (!output->advance_write(outlen))
        return KM_ERROR_UNKNOWN_ERROR;

    return KM_ERROR_OK;
}

keymaster_error_t RsaDecryptOperation::Finish(const AuthorizationSet& additional_params,
                                              const Buffer& input, const Buffer& /* signature */,
                                              AuthorizationSet* /* output_params */,
                                              Buffer* output) {
    if (!output)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    keymaster_error_t error = UpdateForFinish(additional_params, input);
    if (error != KM_ERROR_OK)
        return error;

    UniquePtr<EVP_PKEY_CTX, EVP_PKEY_CTX_Delete> ctx(
        EVP_PKEY_CTX_new(rsa_key_, nullptr /* engine */));
    if (!ctx.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    if (EVP_PKEY_decrypt_init(ctx.get()) <= 0)
        return TranslateLastOpenSslError();

    error = SetRsaPaddingInEvpContext(ctx.get(), false /* signing */);
    if (error != KM_ERROR_OK)
        return error;
    error = SetOaepDigestIfRequired(ctx.get());
    if (error != KM_ERROR_OK)
        return error;

    size_t outlen;
    if (EVP_PKEY_decrypt(ctx.get(), nullptr /* out */, &outlen, data_.peek_read(),
                         data_.available_read()) <= 0)
        return TranslateLastOpenSslError();

    if (!output->Reinitialize(outlen))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    const uint8_t* to_decrypt = data_.peek_read();
    size_t to_decrypt_len = data_.available_read();
    UniquePtr<uint8_t[]> zero_padded;
    if (padding_ == KM_PAD_NONE && to_decrypt_len < outlen) {
        keymaster_error_t error = zero_pad_left(&zero_padded, outlen, data_);
        if (error != KM_ERROR_OK)
            return error;
        to_decrypt = zero_padded.get();
        to_decrypt_len = outlen;
    }

    if (EVP_PKEY_decrypt(ctx.get(), output->peek_write(), &outlen, to_decrypt, to_decrypt_len) <= 0)
        return TranslateLastOpenSslError();
    if (!output->advance_write(outlen))
        return KM_ERROR_UNKNOWN_ERROR;

    return KM_ERROR_OK;
}

}  // namespace keymaster

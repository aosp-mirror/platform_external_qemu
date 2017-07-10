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

#include "aes_operation.h"

#include <stdio.h>

#include <new>

#include <UniquePtr.h>

#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <keymaster/logger.h>

#include "aes_key.h"
#include "openssl_err.h"

namespace keymaster {

static const size_t GCM_NONCE_SIZE = 12;

inline bool allows_padding(keymaster_block_mode_t block_mode) {
    switch (block_mode) {
    case KM_MODE_CTR:
    case KM_MODE_GCM:
        return false;
    case KM_MODE_ECB:
    case KM_MODE_CBC:
        return true;
    }
    assert(false /* Can't get here */);
    return false;
}

static keymaster_error_t GetAndValidateGcmTagLength(const AuthorizationSet& begin_params,
                                                    const AuthorizationSet& key_params,
                                                    size_t* tag_length) {
    uint32_t tag_length_bits;
    if (!begin_params.GetTagValue(TAG_MAC_LENGTH, &tag_length_bits)) {
        return KM_ERROR_MISSING_MAC_LENGTH;
    }

    uint32_t min_tag_length_bits;
    if (!key_params.GetTagValue(TAG_MIN_MAC_LENGTH, &min_tag_length_bits)) {
        LOG_E("AES GCM key must have KM_TAG_MIN_MAC_LENGTH", 0);
        return KM_ERROR_INVALID_KEY_BLOB;
    }

    if (tag_length_bits % 8 != 0 || tag_length_bits > kMaxGcmTagLength ||
        tag_length_bits < kMinGcmTagLength) {
        return KM_ERROR_UNSUPPORTED_MAC_LENGTH;
    }

    if (tag_length_bits < min_tag_length_bits) {
        return KM_ERROR_INVALID_MAC_LENGTH;
    }

    *tag_length = tag_length_bits / 8;
    return KM_ERROR_OK;
}

Operation* AesOperationFactory::CreateOperation(const Key& key,
                                                const AuthorizationSet& begin_params,
                                                keymaster_error_t* error) {
    *error = KM_ERROR_OK;
    const SymmetricKey* symmetric_key = static_cast<const SymmetricKey*>(&key);

    switch (symmetric_key->key_data_size()) {
    case 16:
    case 24:
    case 32:
        break;
    default:
        *error = KM_ERROR_UNSUPPORTED_KEY_SIZE;
        return nullptr;
    }

    keymaster_block_mode_t block_mode;
    if (!begin_params.GetTagValue(TAG_BLOCK_MODE, &block_mode)) {
        LOG_E("%d block modes specified in begin params", begin_params.GetTagCount(TAG_BLOCK_MODE));
        *error = KM_ERROR_UNSUPPORTED_BLOCK_MODE;
        return nullptr;
    } else if (!supported(block_mode)) {
        LOG_E("Block mode %d not supported", block_mode);
        *error = KM_ERROR_UNSUPPORTED_BLOCK_MODE;
        return nullptr;
    } else if (!key.authorizations().Contains(TAG_BLOCK_MODE, block_mode)) {
        LOG_E("Block mode %d was specified, but not authorized by key", block_mode);
        *error = KM_ERROR_INCOMPATIBLE_BLOCK_MODE;
        return nullptr;
    }

    size_t tag_length = 0;
    if (block_mode == KM_MODE_GCM) {
        *error = GetAndValidateGcmTagLength(begin_params, key.authorizations(), &tag_length);
        if (*error != KM_ERROR_OK) {
            return nullptr;
        }
    }

    keymaster_padding_t padding;
    if (!GetAndValidatePadding(begin_params, key, &padding, error)) {
        return nullptr;
    }
    if (!allows_padding(block_mode) && padding != KM_PAD_NONE) {
        LOG_E("Mode does not support padding", 0);
        *error = KM_ERROR_INCOMPATIBLE_PADDING_MODE;
        return nullptr;
    }

    bool caller_nonce = key.authorizations().GetTagValue(TAG_CALLER_NONCE);

    Operation* op = nullptr;
    switch (purpose()) {
    case KM_PURPOSE_ENCRYPT:
        op = new (std::nothrow)
            AesEvpEncryptOperation(block_mode, padding, caller_nonce, tag_length,
                                   symmetric_key->key_data(), symmetric_key->key_data_size());
        break;
    case KM_PURPOSE_DECRYPT:
        op = new (std::nothrow)
            AesEvpDecryptOperation(block_mode, padding, tag_length, symmetric_key->key_data(),
                                   symmetric_key->key_data_size());
        break;
    default:
        *error = KM_ERROR_UNSUPPORTED_PURPOSE;
        return nullptr;
    }

    if (!op)
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
    return op;
}

static const keymaster_block_mode_t supported_block_modes[] = {KM_MODE_ECB, KM_MODE_CBC,
                                                               KM_MODE_CTR, KM_MODE_GCM};

const keymaster_block_mode_t*
AesOperationFactory::SupportedBlockModes(size_t* block_mode_count) const {
    *block_mode_count = array_length(supported_block_modes);
    return supported_block_modes;
}

static const keymaster_padding_t supported_padding_modes[] = {KM_PAD_NONE, KM_PAD_PKCS7};
const keymaster_padding_t*
AesOperationFactory::SupportedPaddingModes(size_t* padding_mode_count) const {
    *padding_mode_count = array_length(supported_padding_modes);
    return supported_padding_modes;
}

AesEvpOperation::AesEvpOperation(keymaster_purpose_t purpose, keymaster_block_mode_t block_mode,
                                 keymaster_padding_t padding, bool caller_iv, size_t tag_length,
                                 const uint8_t* key, size_t key_size)
    : Operation(purpose), block_mode_(block_mode), caller_iv_(caller_iv), tag_length_(tag_length),
      data_started_(false), key_size_(key_size), padding_(padding) {
    memcpy(key_, key, key_size_);
    EVP_CIPHER_CTX_init(&ctx_);
}

AesEvpOperation::~AesEvpOperation() {
    EVP_CIPHER_CTX_cleanup(&ctx_);
    memset_s(aad_block_buf_.get(), AES_BLOCK_SIZE, 0);
}

keymaster_error_t AesEvpOperation::Begin(const AuthorizationSet& /* input_params */,
                                         AuthorizationSet* /* output_params */) {
    if (block_mode_ == KM_MODE_GCM) {
        aad_block_buf_length_ = 0;
        aad_block_buf_.reset(new (std::nothrow) uint8_t[AES_BLOCK_SIZE]);
        if (!aad_block_buf_.get())
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    return InitializeCipher();
}

keymaster_error_t AesEvpOperation::Update(const AuthorizationSet& additional_params,
                                          const Buffer& input,
                                          AuthorizationSet* /* output_params */, Buffer* output,
                                          size_t* input_consumed) {
    keymaster_error_t error;
    if (block_mode_ == KM_MODE_GCM)
        if (!HandleAad(additional_params, input, &error))
            return error;

    if (!InternalUpdate(input.peek_read(), input.available_read(), output, &error))
        return error;
    *input_consumed = input.available_read();

    return KM_ERROR_OK;
}

inline bool is_bad_decrypt(unsigned long error) {
    return (ERR_GET_LIB(error) == ERR_LIB_CIPHER &&  //
            ERR_GET_REASON(error) == CIPHER_R_BAD_DECRYPT);
}

keymaster_error_t AesEvpOperation::Finish(const AuthorizationSet& additional_params,
                                          const Buffer& input, const Buffer& /* signature */,
                                          AuthorizationSet* output_params, Buffer* output) {
    keymaster_error_t error;
    if (!UpdateForFinish(additional_params, input, output_params, output, &error))
        return error;

    if (!output->reserve(AES_BLOCK_SIZE))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    if (block_mode_ == KM_MODE_GCM && aad_block_buf_length_ > 0 && !ProcessBufferedAadBlock(&error))
        return error;

    int output_written = -1;
    if (!EVP_CipherFinal_ex(&ctx_, output->peek_write(), &output_written)) {
        if (tag_length_ > 0)
            return KM_ERROR_VERIFICATION_FAILED;
        LOG_E("Error encrypting final block: %s", ERR_error_string(ERR_peek_last_error(), NULL));
        return TranslateLastOpenSslError();
    }

    assert(output_written <= AES_BLOCK_SIZE);
    if (!output->advance_write(output_written))
        return KM_ERROR_UNKNOWN_ERROR;
    return KM_ERROR_OK;
}

bool AesEvpOperation::need_iv() const {
    switch (block_mode_) {
    case KM_MODE_CBC:
    case KM_MODE_CTR:
    case KM_MODE_GCM:
        return true;
    case KM_MODE_ECB:
        return false;
    default:
        // Shouldn't get here.
        assert(false);
        return false;
    }
}

keymaster_error_t AesEvpOperation::InitializeCipher() {
    const EVP_CIPHER* cipher;
    switch (block_mode_) {
    case KM_MODE_ECB:
        switch (key_size_) {
        case 16:
            cipher = EVP_aes_128_ecb();
            break;
        case 24:
            cipher = EVP_aes_192_ecb();
            break;
        case 32:
            cipher = EVP_aes_256_ecb();
            break;
        default:
            return KM_ERROR_UNSUPPORTED_KEY_SIZE;
        }
        break;
    case KM_MODE_CBC:
        switch (key_size_) {
        case 16:
            cipher = EVP_aes_128_cbc();
            break;
        case 24:
            cipher = EVP_aes_192_cbc();
            break;
        case 32:
            cipher = EVP_aes_256_cbc();
            break;
        default:
            return KM_ERROR_UNSUPPORTED_KEY_SIZE;
        }
        break;
    case KM_MODE_CTR:
        switch (key_size_) {
        case 16:
            cipher = EVP_aes_128_ctr();
            break;
        case 24:
            cipher = EVP_aes_192_ctr();
            break;
        case 32:
            cipher = EVP_aes_256_ctr();
            break;
        default:
            return KM_ERROR_UNSUPPORTED_KEY_SIZE;
        }
        break;
    case KM_MODE_GCM:
        switch (key_size_) {
        case 16:
            cipher = EVP_aes_128_gcm();
            break;
        case 24:
            cipher = EVP_aes_192_gcm();
            break;
        case 32:
            cipher = EVP_aes_256_gcm();
            break;
        default:
            return KM_ERROR_UNSUPPORTED_KEY_SIZE;
        }
        break;
    default:
        return KM_ERROR_UNSUPPORTED_BLOCK_MODE;
    }

    if (!EVP_CipherInit_ex(&ctx_, cipher, NULL /* engine */, key_, iv_.get(), evp_encrypt_mode()))
        return TranslateLastOpenSslError();

    switch (padding_) {
    case KM_PAD_NONE:
        EVP_CIPHER_CTX_set_padding(&ctx_, 0 /* disable padding */);
        break;
    case KM_PAD_PKCS7:
        // This is the default for OpenSSL EVP cipher operations.
        break;
    default:
        return KM_ERROR_UNSUPPORTED_PADDING_MODE;
    }

    if (block_mode_ == KM_MODE_GCM) {
        aad_block_buf_length_ = 0;
        aad_block_buf_.reset(new (std::nothrow) uint8_t[AES_BLOCK_SIZE]);
        if (!aad_block_buf_.get())
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AesEvpOperation::GetIv(const AuthorizationSet& input_params) {
    keymaster_blob_t iv_blob;
    if (!input_params.GetTagValue(TAG_NONCE, &iv_blob)) {
        LOG_E("No IV provided", 0);
        return KM_ERROR_INVALID_ARGUMENT;
    }
    if (block_mode_ != KM_MODE_GCM && iv_blob.data_length != AES_BLOCK_SIZE) {
        LOG_E("Expected %d-byte IV for AES operation, but got %d bytes", AES_BLOCK_SIZE,
              iv_blob.data_length);
        return KM_ERROR_INVALID_NONCE;
    }
    if (block_mode_ == KM_MODE_GCM && iv_blob.data_length != GCM_NONCE_SIZE) {
        LOG_E("Expected %d-byte nonce for AES-GCM operation, but got %d bytes", GCM_NONCE_SIZE,
              iv_blob.data_length);
        return KM_ERROR_INVALID_NONCE;
    }
    iv_.reset(dup_array(iv_blob.data, iv_blob.data_length));
    if (!iv_.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    iv_length_ = iv_blob.data_length;
    return KM_ERROR_OK;
}

/*
 * Process Incoming Associated Authentication Data.
 *
 * This method is more complex than might be expected, because the underlying library silently does
 * the wrong thing when given partial AAD blocks, so we have to take care to process AAD in
 * AES_BLOCK_SIZE increments, buffering (in aad_block_buf_) when given smaller amounts of data.
 */
bool AesEvpOperation::HandleAad(const AuthorizationSet& input_params, const Buffer& input,
                                keymaster_error_t* error) {
    assert(tag_length_ > 0);
    assert(error);

    keymaster_blob_t aad;
    if (input_params.GetTagValue(TAG_ASSOCIATED_DATA, &aad)) {
        if (data_started_) {
            *error = KM_ERROR_INVALID_TAG;
            return false;
        }

        if (aad_block_buf_length_ > 0) {
            FillBufferedAadBlock(&aad);
            if (aad_block_buf_length_ == AES_BLOCK_SIZE && !ProcessBufferedAadBlock(error))
                return false;
        }

        size_t blocks_to_process = aad.data_length / AES_BLOCK_SIZE;
        if (blocks_to_process && !ProcessAadBlocks(aad.data, blocks_to_process, error))
            return false;
        aad.data += blocks_to_process * AES_BLOCK_SIZE;
        aad.data_length -= blocks_to_process * AES_BLOCK_SIZE;

        FillBufferedAadBlock(&aad);
        assert(aad.data_length == 0);
    }

    if (input.available_read()) {
        data_started_ = true;
        // Data has begun, no more AAD is allowed.  Process any buffered AAD.
        if (aad_block_buf_length_ > 0 && !ProcessBufferedAadBlock(error))
            return false;
    }

    return true;
}

bool AesEvpOperation::ProcessBufferedAadBlock(keymaster_error_t* error) {
    int output_written;
    if (EVP_CipherUpdate(&ctx_, nullptr /* out */, &output_written, aad_block_buf_.get(),
                         aad_block_buf_length_)) {
        aad_block_buf_length_ = 0;
        return true;
    }
    *error = TranslateLastOpenSslError();
    return false;
}

bool AesEvpOperation::ProcessAadBlocks(const uint8_t* data, size_t blocks,
                                       keymaster_error_t* error) {
    int output_written;
    if (EVP_CipherUpdate(&ctx_, nullptr /* out */, &output_written, data, blocks * AES_BLOCK_SIZE))
        return true;
    *error = TranslateLastOpenSslError();
    return false;
}

inline size_t min(size_t a, size_t b) {
    return (a < b) ? a : b;
}

void AesEvpOperation::FillBufferedAadBlock(keymaster_blob_t* aad) {
    size_t to_buffer = min(AES_BLOCK_SIZE - aad_block_buf_length_, aad->data_length);
    memcpy(aad_block_buf_.get() + aad_block_buf_length_, aad->data, to_buffer);
    aad->data += to_buffer;
    aad->data_length -= to_buffer;
    aad_block_buf_length_ += to_buffer;
}

bool AesEvpOperation::InternalUpdate(const uint8_t* input, size_t input_length, Buffer* output,
                                     keymaster_error_t* error) {
    assert(output);
    assert(error);

    if (!input_length)
        return true;

    if (!output->reserve(input_length + AES_BLOCK_SIZE)) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return false;
    }

    int output_written = -1;
    if (!EVP_CipherUpdate(&ctx_, output->peek_write(), &output_written, input, input_length)) {
        *error = TranslateLastOpenSslError();
        return false;
    }
    return output->advance_write(output_written);
}

bool AesEvpOperation::UpdateForFinish(const AuthorizationSet& additional_params,
                                      const Buffer& input, AuthorizationSet* output_params,
                                      Buffer* output, keymaster_error_t* error) {
    if (input.available_read() || !additional_params.empty()) {
        size_t input_consumed;
        *error = Update(additional_params, input, output_params, output, &input_consumed);
        if (*error != KM_ERROR_OK)
            return false;
        if (input_consumed != input.available_read()) {
            *error = KM_ERROR_INVALID_INPUT_LENGTH;
            return false;
        }
    }

    return true;
}

keymaster_error_t AesEvpEncryptOperation::Begin(const AuthorizationSet& input_params,
                                                AuthorizationSet* output_params) {
    if (!output_params)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    if (need_iv()) {
        keymaster_error_t error = KM_ERROR_OK;
        if (input_params.find(TAG_NONCE) == -1)
            error = GenerateIv();
        else if (caller_iv_)
            error = GetIv(input_params);
        else
            error = KM_ERROR_CALLER_NONCE_PROHIBITED;

        if (error == KM_ERROR_OK)
            output_params->push_back(TAG_NONCE, iv_.get(), iv_length_);
        else
            return error;
    }

    return AesEvpOperation::Begin(input_params, output_params);
}

keymaster_error_t AesEvpEncryptOperation::Finish(const AuthorizationSet& additional_params,
                                                 const Buffer& input, const Buffer& signature,
                                                 AuthorizationSet* output_params, Buffer* output) {
    if (!output->reserve(input.available_read() + AES_BLOCK_SIZE + tag_length_))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    keymaster_error_t error =
        AesEvpOperation::Finish(additional_params, input, signature, output_params, output);
    if (error != KM_ERROR_OK)
        return error;

    if (tag_length_ > 0) {
        if (!output->reserve(tag_length_))
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;

        if (!EVP_CIPHER_CTX_ctrl(&ctx_, EVP_CTRL_GCM_GET_TAG, tag_length_, output->peek_write()))
            return TranslateLastOpenSslError();
        if (!output->advance_write(tag_length_))
            return KM_ERROR_UNKNOWN_ERROR;
    }

    return KM_ERROR_OK;
}

keymaster_error_t AesEvpEncryptOperation::GenerateIv() {
    iv_length_ = (block_mode_ == KM_MODE_GCM) ? GCM_NONCE_SIZE : AES_BLOCK_SIZE;
    iv_.reset(new (std::nothrow) uint8_t[iv_length_]);
    if (!iv_.get())
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    if (RAND_bytes(iv_.get(), iv_length_) != 1)
        return TranslateLastOpenSslError();
    return KM_ERROR_OK;
}

keymaster_error_t AesEvpDecryptOperation::Begin(const AuthorizationSet& input_params,
                                                AuthorizationSet* output_params) {
    if (need_iv()) {
        keymaster_error_t error = GetIv(input_params);
        if (error != KM_ERROR_OK)
            return error;
    }

    if (tag_length_ > 0) {
        tag_buf_length_ = 0;
        tag_buf_.reset(new (std::nothrow) uint8_t[tag_length_]);
        if (!tag_buf_.get())
            return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    return AesEvpOperation::Begin(input_params, output_params);
}

keymaster_error_t AesEvpDecryptOperation::Update(const AuthorizationSet& additional_params,
                                                 const Buffer& input,
                                                 AuthorizationSet* /* output_params */,
                                                 Buffer* output, size_t* input_consumed) {
    if (!output || !input_consumed)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    // Barring error, we'll consume it all.
    *input_consumed = input.available_read();

    keymaster_error_t error;
    if (block_mode_ == KM_MODE_GCM) {
        if (!HandleAad(additional_params, input, &error))
            return error;
        return ProcessAllButTagLengthBytes(input, output);
    }

    if (!InternalUpdate(input.peek_read(), input.available_read(), output, &error))
        return error;
    return KM_ERROR_OK;
}

keymaster_error_t AesEvpDecryptOperation::ProcessAllButTagLengthBytes(const Buffer& input,
                                                                      Buffer* output) {
    if (input.available_read() <= tag_buf_unused()) {
        BufferCandidateTagData(input.peek_read(), input.available_read());
        return KM_ERROR_OK;
    }

    const size_t data_available = tag_buf_length_ + input.available_read();

    const size_t to_process = data_available - tag_length_;
    const size_t to_process_from_tag_buf = min(to_process, tag_buf_length_);
    const size_t to_process_from_input = to_process - to_process_from_tag_buf;

    if (!output->reserve(to_process + AES_BLOCK_SIZE))
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    keymaster_error_t error;
    if (!ProcessTagBufContentsAsData(to_process_from_tag_buf, output, &error))
        return error;

    if (!InternalUpdate(input.peek_read(), to_process_from_input, output, &error))
        return error;

    BufferCandidateTagData(input.peek_read() + to_process_from_input,
                           input.available_read() - to_process_from_input);
    assert(tag_buf_unused() == 0);

    return KM_ERROR_OK;
}

bool AesEvpDecryptOperation::ProcessTagBufContentsAsData(size_t to_process, Buffer* output,
                                                         keymaster_error_t* error) {
    assert(to_process <= tag_buf_length_);
    if (!InternalUpdate(tag_buf_.get(), to_process, output, error))
        return false;
    if (to_process < tag_buf_length_)
        memmove(tag_buf_.get(), tag_buf_.get() + to_process, tag_buf_length_ - to_process);
    tag_buf_length_ -= to_process;
    return true;
}

void AesEvpDecryptOperation::BufferCandidateTagData(const uint8_t* data, size_t data_length) {
    assert(data_length <= tag_length_ - tag_buf_length_);
    memcpy(tag_buf_.get() + tag_buf_length_, data, data_length);
    tag_buf_length_ += data_length;
}

keymaster_error_t AesEvpDecryptOperation::Finish(const AuthorizationSet& additional_params,
                                                 const Buffer& input, const Buffer& signature,
                                                 AuthorizationSet* output_params, Buffer* output) {
    keymaster_error_t error;
    if (!UpdateForFinish(additional_params, input, output_params, output, &error))
        return error;

    if (tag_buf_length_ < tag_length_)
        return KM_ERROR_INVALID_INPUT_LENGTH;
    else if (tag_length_ > 0 &&
             !EVP_CIPHER_CTX_ctrl(&ctx_, EVP_CTRL_GCM_SET_TAG, tag_length_, tag_buf_.get()))
        return TranslateLastOpenSslError();

    AuthorizationSet empty_params;
    Buffer empty_input;
    return AesEvpOperation::Finish(empty_params, empty_input, signature, output_params, output);
}

keymaster_error_t AesEvpOperation::Abort() {
    return KM_ERROR_OK;
}

}  // namespace keymaster

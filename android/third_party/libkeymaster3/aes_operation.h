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

#ifndef SYSTEM_KEYMASTER_AES_OPERATION_H_
#define SYSTEM_KEYMASTER_AES_OPERATION_H_

#include <openssl/evp.h>

#include "ocb_utils.h"
#include "operation.h"

namespace keymaster {

/**
 * Abstract base for AES operation factories.  This class does all of the work to create
 * AES operations.
 */
class AesOperationFactory : public OperationFactory {
  public:
    KeyType registry_key() const override { return KeyType(KM_ALGORITHM_AES, purpose()); }

    Operation* CreateOperation(const Key& key, const AuthorizationSet& begin_params,
                               keymaster_error_t* error) override;
    const keymaster_block_mode_t* SupportedBlockModes(size_t* block_mode_count) const override;
    const keymaster_padding_t* SupportedPaddingModes(size_t* padding_count) const override;

    virtual keymaster_purpose_t purpose() const = 0;
};

/**
 * Concrete factory for AES encryption operations.
 */
class AesEncryptionOperationFactory : public AesOperationFactory {
    keymaster_purpose_t purpose() const override { return KM_PURPOSE_ENCRYPT; }
};

/**
 * Concrete factory for AES decryption operations.
 */
class AesDecryptionOperationFactory : public AesOperationFactory {
    keymaster_purpose_t purpose() const override { return KM_PURPOSE_DECRYPT; }
};

static const size_t MAX_EVP_KEY_SIZE = 32;

class AesEvpOperation : public Operation {
  public:
    AesEvpOperation(keymaster_purpose_t purpose, keymaster_block_mode_t block_mode,
                    keymaster_padding_t padding, bool caller_iv, size_t tag_length,
                    const uint8_t* key, size_t key_size);
    ~AesEvpOperation();

    keymaster_error_t Begin(const AuthorizationSet& input_params,
                            AuthorizationSet* output_params) override;
    keymaster_error_t Update(const AuthorizationSet& additional_params, const Buffer& input,
                             AuthorizationSet* output_params, Buffer* output,
                             size_t* input_consumed) override;
    keymaster_error_t Finish(const AuthorizationSet& additional_params, const Buffer& input,
                             const Buffer& signature, AuthorizationSet* output_params,
                             Buffer* output) override;
    keymaster_error_t Abort() override;

    virtual int evp_encrypt_mode() = 0;

  protected:
    bool need_iv() const;
    keymaster_error_t InitializeCipher();
    keymaster_error_t GetIv(const AuthorizationSet& input_params);
    bool HandleAad(const AuthorizationSet& input_params, const Buffer& input,
                   keymaster_error_t* error);
    bool ProcessAadBlocks(const uint8_t* data, size_t blocks, keymaster_error_t* error);
    void FillBufferedAadBlock(keymaster_blob_t* aad);
    bool ProcessBufferedAadBlock(keymaster_error_t* error);
    bool InternalUpdate(const uint8_t* input, size_t input_length, Buffer* output,
                        keymaster_error_t* error);
    bool UpdateForFinish(const AuthorizationSet& additional_params, const Buffer& input,
                         AuthorizationSet* output_params, Buffer* output, keymaster_error_t* error);

    const keymaster_block_mode_t block_mode_;
    EVP_CIPHER_CTX ctx_;
    UniquePtr<uint8_t[]> iv_;
    size_t iv_length_;
    const bool caller_iv_;
    size_t tag_length_;
    UniquePtr<uint8_t[]> aad_block_buf_;
    size_t aad_block_buf_length_;

  private:
    bool data_started_;
    const size_t key_size_;
    const keymaster_padding_t padding_;
    uint8_t key_[MAX_EVP_KEY_SIZE];
};

class AesEvpEncryptOperation : public AesEvpOperation {
  public:
    AesEvpEncryptOperation(keymaster_block_mode_t block_mode, keymaster_padding_t padding,
                           bool caller_iv, size_t tag_length, const uint8_t* key, size_t key_size)
        : AesEvpOperation(KM_PURPOSE_ENCRYPT, block_mode, padding, caller_iv, tag_length, key,
                          key_size) {}

    keymaster_error_t Begin(const AuthorizationSet& input_params,
                            AuthorizationSet* output_params) override;
    keymaster_error_t Finish(const AuthorizationSet& additional_params, const Buffer& input,
                             const Buffer& signature, AuthorizationSet* output_params,
                             Buffer* output) override;

    int evp_encrypt_mode() override { return 1; }

  private:
    keymaster_error_t GenerateIv();
};

class AesEvpDecryptOperation : public AesEvpOperation {
  public:
    AesEvpDecryptOperation(keymaster_block_mode_t block_mode, keymaster_padding_t padding,
                           size_t tag_length, const uint8_t* key, size_t key_size)
        : AesEvpOperation(KM_PURPOSE_DECRYPT, block_mode, padding,
                          false /* caller_iv -- don't care */, tag_length, key, key_size) {}

    keymaster_error_t Begin(const AuthorizationSet& input_params,
                            AuthorizationSet* output_params) override;
    keymaster_error_t Update(const AuthorizationSet& additional_params, const Buffer& input,
                             AuthorizationSet* output_params, Buffer* output,
                             size_t* input_consumed) override;
    keymaster_error_t Finish(const AuthorizationSet& additional_params, const Buffer& input,
                             const Buffer& signature, AuthorizationSet* output_params,
                             Buffer* output) override;

    int evp_encrypt_mode() override { return 0; }

  private:
    size_t tag_buf_unused() { return tag_length_ - tag_buf_length_; }

    keymaster_error_t ProcessAllButTagLengthBytes(const Buffer& input, Buffer* output);
    bool ProcessTagBufContentsAsData(size_t to_process, Buffer* output, keymaster_error_t* error);
    void BufferCandidateTagData(const uint8_t* data, size_t data_length);

    UniquePtr<uint8_t[]> tag_buf_;
    size_t tag_buf_length_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_AES_OPERATION_H_

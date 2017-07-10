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

#ifndef SYSTEM_KEYMASTER_ECDSA_OPERATION_H_
#define SYSTEM_KEYMASTER_ECDSA_OPERATION_H_

#include <openssl/ec.h>
#include <openssl/evp.h>

#include <UniquePtr.h>

#include "operation.h"

namespace keymaster {

class EcdsaOperation : public Operation {
  public:
    EcdsaOperation(keymaster_purpose_t purpose, keymaster_digest_t digest, EVP_PKEY* key)
        : Operation(purpose), digest_(digest), ecdsa_key_(key) {
        EVP_MD_CTX_init(&digest_ctx_);
    }
    ~EcdsaOperation();

    keymaster_error_t Abort() override { return KM_ERROR_OK; }

  protected:
    keymaster_error_t StoreData(const Buffer& input, size_t* input_consumed);
    keymaster_error_t InitDigest();

    keymaster_digest_t digest_;
    const EVP_MD* digest_algorithm_;
    EVP_PKEY* ecdsa_key_;
    EVP_MD_CTX digest_ctx_;
    Buffer data_;
};

class EcdsaSignOperation : public EcdsaOperation {
  public:
    EcdsaSignOperation(keymaster_digest_t digest, EVP_PKEY* key)
        : EcdsaOperation(KM_PURPOSE_SIGN, digest, key) {}
    keymaster_error_t Begin(const AuthorizationSet& input_params,
                            AuthorizationSet* output_params) override;
    keymaster_error_t Update(const AuthorizationSet& additional_params, const Buffer& input,
                             AuthorizationSet* output_params, Buffer* output,
                             size_t* input_consumed) override;
    keymaster_error_t Finish(const AuthorizationSet& additional_params, const Buffer& input,
                             const Buffer& signature, AuthorizationSet* output_params,
                             Buffer* output) override;
};

class EcdsaVerifyOperation : public EcdsaOperation {
  public:
    EcdsaVerifyOperation(keymaster_digest_t digest, EVP_PKEY* key)
        : EcdsaOperation(KM_PURPOSE_VERIFY, digest, key) {}
    keymaster_error_t Begin(const AuthorizationSet& input_params,
                            AuthorizationSet* output_params) override;
    keymaster_error_t Update(const AuthorizationSet& additional_params, const Buffer& input,
                             AuthorizationSet* output_params, Buffer* output,
                             size_t* input_consumed) override;
    keymaster_error_t Finish(const AuthorizationSet& additional_params, const Buffer& input,
                             const Buffer& signature, AuthorizationSet* output_params,
                             Buffer* output) override;
};

class EcdsaOperationFactory : public OperationFactory {
  private:
    KeyType registry_key() const override { return KeyType(KM_ALGORITHM_EC, purpose()); }
    Operation* CreateOperation(const Key& key, const AuthorizationSet& begin_params,
                               keymaster_error_t* error) override;
    const keymaster_digest_t* SupportedDigests(size_t* digest_count) const override;

    virtual keymaster_purpose_t purpose() const = 0;
    virtual Operation* InstantiateOperation(keymaster_digest_t digest, EVP_PKEY* key) = 0;
};

class EcdsaSignOperationFactory : public EcdsaOperationFactory {
  private:
    keymaster_purpose_t purpose() const override { return KM_PURPOSE_SIGN; }
    Operation* InstantiateOperation(keymaster_digest_t digest, EVP_PKEY* key) override {
        return new (std::nothrow) EcdsaSignOperation(digest, key);
    }
};

class EcdsaVerifyOperationFactory : public EcdsaOperationFactory {
  public:
    keymaster_purpose_t purpose() const override { return KM_PURPOSE_VERIFY; }
    Operation* InstantiateOperation(keymaster_digest_t digest, EVP_PKEY* key) override {
        return new (std::nothrow) EcdsaVerifyOperation(digest, key);
    }
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ECDSA_OPERATION_H_

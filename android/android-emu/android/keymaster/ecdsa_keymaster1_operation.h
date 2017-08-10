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

#ifndef SYSTEM_KEYMASTER_ECDSA_KEYMASTER1_OPERATION_H_
#define SYSTEM_KEYMASTER_ECDSA_KEYMASTER1_OPERATION_H_

#include <openssl/evp.h>

#include <hardware/keymaster1.h>
#include <keymaster/android_keymaster_utils.h>

#include "ecdsa_operation.h"
#include "keymaster1_engine.h"

namespace keymaster {

class EcdsaKeymaster1WrappedOperation {
  public:
    EcdsaKeymaster1WrappedOperation(keymaster_purpose_t purpose, const Keymaster1Engine* engine)
        : purpose_(purpose), operation_handle_(0), engine_(engine) {}
    ~EcdsaKeymaster1WrappedOperation() {
        if (operation_handle_)
            Abort();
    }

    keymaster_error_t Begin(EVP_PKEY* ecdsa_key, const AuthorizationSet& input_params);
    keymaster_error_t PrepareFinish(EVP_PKEY* ecdsa_key, const AuthorizationSet& input_params);
    void Finish() { operation_handle_ = 0; }
    keymaster_error_t Abort();

    keymaster_error_t GetError(EVP_PKEY* ecdsa_key);

  protected:
    keymaster_purpose_t purpose_;
    keymaster_operation_handle_t operation_handle_;
    const Keymaster1Engine* engine_;
};

template <typename BaseOperation> class EcdsaKeymaster1Operation : public BaseOperation {
    typedef BaseOperation super;

  public:
    EcdsaKeymaster1Operation(keymaster_digest_t digest, EVP_PKEY* key,
                             const Keymaster1Engine* engine)
        : BaseOperation(digest, key), wrapped_operation_(super::purpose(), engine) {
        // Shouldn't be instantiated for public key operations.
        assert(super::purpose() != KM_PURPOSE_VERIFY);
        assert(super::purpose() != KM_PURPOSE_ENCRYPT);
    }

    keymaster_error_t Begin(const AuthorizationSet& input_params,
                            AuthorizationSet* output_params) override {
        keymaster_error_t error = wrapped_operation_.Begin(super::ecdsa_key_, input_params);
        if (error != KM_ERROR_OK)
            return error;
        return super::Begin(input_params, output_params);
    }

    keymaster_error_t Finish(const AuthorizationSet& input_params, const Buffer& input,
                             const Buffer& signature, AuthorizationSet* output_params,
                             Buffer* output) override {
        keymaster_error_t error = wrapped_operation_.PrepareFinish(super::ecdsa_key_, input_params);
        if (error != KM_ERROR_OK)
            return error;
        error = super::Finish(input_params, input, signature, output_params, output);
        if (wrapped_operation_.GetError(super::ecdsa_key_) != KM_ERROR_OK)
            error = wrapped_operation_.GetError(super::ecdsa_key_);
        if (error == KM_ERROR_OK)
            wrapped_operation_.Finish();
        return error;
    }

    keymaster_error_t Abort() override {
        keymaster_error_t error = wrapped_operation_.Abort();
        if (error != KM_ERROR_OK)
            return error;
        return super::Abort();
    }

  private:
    EcdsaKeymaster1WrappedOperation wrapped_operation_;
};

/**
 * Factory that produces EcdsaKeymaster1Operations.  This is instantiated and
 * provided by EcdsaKeymaster1KeyFactory.
 */
class EcdsaKeymaster1OperationFactory : public OperationFactory {
  public:
    EcdsaKeymaster1OperationFactory(keymaster_purpose_t purpose, const Keymaster1Engine* engine)
        : purpose_(purpose), engine_(engine) {}
    KeyType registry_key() const override { return KeyType(KM_ALGORITHM_EC, purpose_); }

    Operation* CreateOperation(const Key& key, const AuthorizationSet& begin_params,
                               keymaster_error_t* error) override;

    const keymaster_digest_t* SupportedDigests(size_t* digest_count) const override;
    const keymaster_padding_t* SupportedPaddingModes(size_t* padding_mode_count) const override;

  private:
    keymaster_purpose_t purpose_;
    const Keymaster1Engine* engine_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_ECDSA_KEYMASTER1_OPERATION_H_

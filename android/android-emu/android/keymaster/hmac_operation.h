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

#ifndef SYSTEM_KEYMASTER_HMAC_OPERATION_H_
#define SYSTEM_KEYMASTER_HMAC_OPERATION_H_

#include "operation.h"

#include <openssl/hmac.h>

namespace keymaster {

class HmacOperation : public Operation {
  public:
    HmacOperation(keymaster_purpose_t purpose, const uint8_t* key_data, size_t key_data_size,
                  keymaster_digest_t digest, size_t mac_length, size_t min_mac_length);
    ~HmacOperation();

    virtual keymaster_error_t Begin(const AuthorizationSet& input_params,
                                    AuthorizationSet* output_params);
    virtual keymaster_error_t Update(const AuthorizationSet& additional_params, const Buffer& input,
                                     AuthorizationSet* output_params, Buffer* output,
                                     size_t* input_consumed);
    virtual keymaster_error_t Abort();
    virtual keymaster_error_t Finish(const AuthorizationSet& additional_params, const Buffer& input,
                                     const Buffer& signature, AuthorizationSet* output_params,
                                     Buffer* output);

    keymaster_error_t error() { return error_; }

  private:
    HMAC_CTX ctx_;
    keymaster_error_t error_;
    const size_t mac_length_;
    const size_t min_mac_length_;
};

/**
 * Abstract base for HMAC operation factories.  This class does all of the work to create
 * HMAC operations.
 */
class HmacOperationFactory : public OperationFactory {
  public:
    virtual KeyType registry_key() const { return KeyType(KM_ALGORITHM_HMAC, purpose()); }

    virtual Operation* CreateOperation(const Key& key, const AuthorizationSet& begin_params,
                                       keymaster_error_t* error);

    virtual const keymaster_digest_t* SupportedDigests(size_t* digest_count) const;

    virtual keymaster_purpose_t purpose() const = 0;
};

class HmacSignOperationFactory : public HmacOperationFactory {
    keymaster_purpose_t purpose() const { return KM_PURPOSE_SIGN; }
};

class HmacVerifyOperationFactory : public HmacOperationFactory {
    keymaster_purpose_t purpose() const { return KM_PURPOSE_VERIFY; }
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_HMAC_OPERATION_H_

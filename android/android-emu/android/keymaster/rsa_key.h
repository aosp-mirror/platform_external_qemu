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

#ifndef SYSTEM_KEYMASTER_RSA_KEY_H_
#define SYSTEM_KEYMASTER_RSA_KEY_H_

#include <openssl/rsa.h>

#include "asymmetric_key.h"

namespace keymaster {

class RsaKey : public AsymmetricKey {
  public:
    RsaKey(const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
           keymaster_error_t* error)
        : AsymmetricKey(hw_enforced, sw_enforced, error) {}

    bool InternalToEvp(EVP_PKEY* pkey) const override;
    bool EvpToInternal(const EVP_PKEY* pkey) override;

    bool SupportedMode(keymaster_purpose_t purpose, keymaster_padding_t padding);
    bool SupportedMode(keymaster_purpose_t purpose, keymaster_digest_t digest);

    struct RSA_Delete {
        void operator()(RSA* p) { RSA_free(p); }
    };

    RSA* key() const { return rsa_key_.get(); }

  protected:
    RsaKey(RSA* rsa, const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
           keymaster_error_t* error)
        : AsymmetricKey(hw_enforced, sw_enforced, error), rsa_key_(rsa) {}

  private:
    UniquePtr<RSA, RSA_Delete> rsa_key_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_RSA_KEY_H_

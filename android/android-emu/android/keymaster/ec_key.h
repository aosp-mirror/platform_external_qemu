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

#ifndef SYSTEM_KEYMASTER_EC_KEY_H_
#define SYSTEM_KEYMASTER_EC_KEY_H_

#include <openssl/ec.h>

#include "asymmetric_key.h"
#include "openssl_utils.h"

namespace keymaster {

class EcdsaOperationFactory;

class EcKey : public AsymmetricKey {
  public:
    EcKey(const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
          keymaster_error_t* error)
        : AsymmetricKey(hw_enforced, sw_enforced, error) {}

    bool InternalToEvp(EVP_PKEY* pkey) const override;
    bool EvpToInternal(const EVP_PKEY* pkey) override;

    EC_KEY* key() const { return ec_key_.get(); }

  protected:
    EcKey(EC_KEY* ec_key, const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
          keymaster_error_t* error)
        : AsymmetricKey(hw_enforced, sw_enforced, error), ec_key_(ec_key) {}

  private:
    UniquePtr<EC_KEY, EC_KEY_Delete> ec_key_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_EC_KEY_H_

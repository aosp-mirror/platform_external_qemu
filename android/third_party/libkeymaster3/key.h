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

#ifndef SYSTEM_KEYMASTER_KEY_H_
#define SYSTEM_KEYMASTER_KEY_H_

#include <UniquePtr.h>

#include <hardware/keymaster_defs.h>

#include <keymaster/android_keymaster_utils.h>
#include <keymaster/authorization_set.h>
#include <keymaster/keymaster_context.h>

namespace keymaster {

class Key {
  public:
    virtual ~Key() {}

    /**
     * Return a copy of raw key material, in the specified format.
     */
    virtual keymaster_error_t formatted_key_material(keymaster_key_format_t format,
                                                     UniquePtr<uint8_t[]>* material,
                                                     size_t* size) const = 0;

    /**
     * Generate an attestation certificate chain.
     */
    virtual keymaster_error_t GenerateAttestation(
        const KeymasterContext& /* context */, const AuthorizationSet& /* attest_params */,
        const AuthorizationSet& /* tee_enforced */, const AuthorizationSet& /* sw_enforced */,
        keymaster_cert_chain_t* /* certificate_chain */) const {
        return KM_ERROR_INCOMPATIBLE_ALGORITHM;
    }

    const AuthorizationSet& authorizations() const { return authorizations_; }

  protected:
    Key(const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
        keymaster_error_t* error);

  private:
    AuthorizationSet authorizations_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEY_H_

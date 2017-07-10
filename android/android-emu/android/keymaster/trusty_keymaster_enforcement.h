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

#ifndef TRUSTY_APP_KEYMASTER_TRUSTY_KEYMASTER_ENFORCEMENT_H_
#define TRUSTY_APP_KEYMASTER_TRUSTY_KEYMASTER_ENFORCEMENT_H_

#include <keymaster/keymaster_enforcement.h>

namespace keymaster {

class TrustyKeymasterContext;

const int kAccessMapTableSize = 32;
const int kAccessCountTableSize = 32;

class TrustyKeymasterEnforcement : public KeymasterEnforcement {
  public:
    TrustyKeymasterEnforcement(TrustyKeymasterContext* context)
        : KeymasterEnforcement(kAccessMapTableSize, kAccessCountTableSize), context_(context) {}
    ~TrustyKeymasterEnforcement() {}

    bool activation_date_valid(uint64_t activation_date) const override {
        // Have no wall clock, can't check activations.
        return true;
    }

    bool expiration_date_passed(uint64_t expiration_date) const override {
        // Have no wall clock, can't check expirations.
        return false;
    }

    bool auth_token_timed_out(const hw_auth_token_t& token, uint32_t timeout) const override;
    uint32_t get_current_time() const override;
    bool ValidateTokenSignature(const hw_auth_token_t& token) const override;

  private:
    uint64_t milliseconds_since_boot() const;

    TrustyKeymasterContext* context_;
};

}  // namespace keymaster

#endif  // TRUSTY_APP_KEYMASTER_TRUSTY_KEYMASTER_ENFORCEMENT_H_

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

#include <assert.h>

#include "key.h"

#include <openssl/x509.h>

#include "openssl_utils.h"

namespace keymaster {

Key::Key(const AuthorizationSet& hw_enforced, const AuthorizationSet& sw_enforced,
         keymaster_error_t* error) {
    assert(error);
    authorizations_.push_back(hw_enforced);
    authorizations_.push_back(sw_enforced);
    *error = KM_ERROR_OK;
    if (authorizations_.is_valid() != AuthorizationSet::OK)
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
}

}  // namespace keymaster

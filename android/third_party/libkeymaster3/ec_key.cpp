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

#include "ec_key.h"

#if defined(OPENSSL_IS_BORINGSSL)
typedef size_t openssl_size_t;
#else
typedef int openssl_size_t;
#endif

namespace keymaster {

bool EcKey::EvpToInternal(const EVP_PKEY* pkey) {
    ec_key_.reset(EVP_PKEY_get1_EC_KEY(const_cast<EVP_PKEY*>(pkey)));
    return ec_key_.get() != NULL;
}

bool EcKey::InternalToEvp(EVP_PKEY* pkey) const {
    return EVP_PKEY_set1_EC_KEY(pkey, ec_key_.get()) == 1;
}

}  // namespace keymaster

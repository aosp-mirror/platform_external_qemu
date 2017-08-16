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

#ifndef SYSTEM_KEYMASTER_OCB_UTILS_H_
#define SYSTEM_KEYMASTER_OCB_UTILS_H_

#include "ae.h"

#include <hardware/keymaster_defs.h>

#include <keymaster/serializable.h>

namespace keymaster {

class AuthorizationSet;
struct KeymasterKeyBlob;

static const int OCB_NONCE_LENGTH = 12;
static const int OCB_TAG_LENGTH = 16;

keymaster_error_t OcbEncryptKey(const AuthorizationSet& hw_enforced,
                                const AuthorizationSet& sw_enforced, const AuthorizationSet& hidden,
                                const KeymasterKeyBlob& master_key,
                                const KeymasterKeyBlob& plaintext, const Buffer& nonce,
                                KeymasterKeyBlob* ciphertext, Buffer* tag);

keymaster_error_t OcbDecryptKey(const AuthorizationSet& hw_enforced,
                                const AuthorizationSet& sw_enforced, const AuthorizationSet& hidden,
                                const KeymasterKeyBlob& master_key,
                                const KeymasterKeyBlob& ciphertext, const Buffer& nonce,
                                const Buffer& tag, KeymasterKeyBlob* plaintext);

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_OCB_UTILS_H_

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

#ifndef SYSTEM_KEYMASTER_INTEGRITY_ASSURED_KEY_BLOB_
#define SYSTEM_KEYMASTER_INTEGRITY_ASSURED_KEY_BLOB_

#include <hardware/keymaster_defs.h>

namespace keymaster {

class AuthorizationSet;
class Buffer;
struct KeymasterKeyBlob;

keymaster_error_t SerializeIntegrityAssuredBlob(const KeymasterKeyBlob& key_material,
                                                const AuthorizationSet& hidden,
                                                const AuthorizationSet& hw_enforced,
                                                const AuthorizationSet& sw_enforced,
                                                KeymasterKeyBlob* key_blob);

keymaster_error_t DeserializeIntegrityAssuredBlob(const KeymasterKeyBlob& key_blob,
                                                  const AuthorizationSet& hidden,
                                                  KeymasterKeyBlob* key_material,
                                                  AuthorizationSet* hw_enforced,
                                                  AuthorizationSet* sw_enforced);

keymaster_error_t DeserializeIntegrityAssuredBlob_NoHmacCheck(const KeymasterKeyBlob& key_blob,
                                                              KeymasterKeyBlob* key_material,
                                                              AuthorizationSet* hw_enforced,
                                                              AuthorizationSet* sw_enforced);

}  // namespace keymaster;

#endif  // SYSTEM_KEYMASTER_INTEGRITY_ASSURED_KEY_BLOB_

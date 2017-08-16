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

#ifndef SYSTEM_KEYMASTER_KEM_H_
#define SYSTEM_KEYMASTER_KEM_H_

#include <keymaster/serializable.h>

#include "openssl_utils.h"

namespace keymaster {

// Kem is an abstract class that provides an interface to a key encapsulation
// mechansim primitive. A key encapsulation mechanism works just like a
// public-key encryption scheme, except that the encryption algorithm takes no
// input other than the recipient’s public key. Instead, the encryption algorithm
// generates a pair (K, C0), where K is a bit string of some specified length,
// and C0 is an encryption of K, that is, the decryption algorithm applied to C0
// yields K.
class Kem {
  public:
    virtual ~Kem(){};

    // For a key encapsulation mechanism, the goal of encryption is to take the recipient's public
    // key, and to generate a pair (K, C0), where K is a bit string of some specified length,
    // and C0 is an encryption of K.
    virtual bool Encrypt(const Buffer& peer_public_value, Buffer* output_clear_key,
                         Buffer* output_encrypted_key) = 0;
    virtual bool Encrypt(const uint8_t* peer_public_value, size_t peer_public_value_len,
                         Buffer* output_clear_key, Buffer* output_encrypted_key) = 0;

    // Decrypt takes an encrypted key, and outputs its clear text.
    // Decrypt takes ownership of \p private_key.
    virtual bool Decrypt(EC_KEY* private_key, const Buffer& encrypted_key, Buffer* output_key) = 0;
    virtual bool Decrypt(EC_KEY* private_key, const uint8_t* encrypted_key,
                         size_t encrypted_key_len, Buffer* output_key) = 0;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEM_H_

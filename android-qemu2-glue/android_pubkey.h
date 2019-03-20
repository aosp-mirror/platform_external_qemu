/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <openssl/rsa.h>

// Size of an RSA modulus such as an encrypted block or a signature.
#define ANDROID_PUBKEY_MODULUS_SIZE (2048 / 8)

// Size of an encoded RSA key.
#define ANDROID_PUBKEY_ENCODED_SIZE \
    (3 * sizeof(uint32_t) + 2 * ANDROID_PUBKEY_MODULUS_SIZE)

/* Encodes |key| in the Android RSA public key binary format and stores the
 * bytes in |key_buffer|. |key_buffer| should be of size at least
 * |ANDROID_PUBKEY_ENCODED_SIZE|.
 *
 * Returns true if successful, false on error.
 */
bool android_pubkey_encode(const RSA* key, uint8_t* key_buffer, size_t size);

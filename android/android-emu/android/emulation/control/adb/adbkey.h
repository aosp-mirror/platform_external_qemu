// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include <stdint.h>  // for uint8_t, uint32_t
#include <string>    // for string
#include <openssl/rsa.h>


// Size of an RSA modulus such as an encrypted block or a signature.
constexpr const int ANDROID_PUBKEY_MODULUS_SIZE = 2048 / 8;
// Adb authentication
constexpr const int TOKEN_SIZE = 20;

// Size of an encoded RSA key.
constexpr const int ANDROID_PUBKEY_ENCODED_SIZE =
        (3 * sizeof(uint32_t) + 2 * ANDROID_PUBKEY_MODULUS_SIZE);
constexpr const char* kPrivateKeyFileName = "adbkey";
constexpr const char* kPublicKeyFileName = "adbkey.pub";


// Tries to find |adbKeyFileName|, returning "" if not found.
// Will search the default key directories.
std::string getAdbKeyPath(const char* adbKeyFileName);

// Tries to find the "adbkey.pub" file, returning "" if not found
std::string getPublicAdbKeyPath();

// Tries to find the "adbkey" file, returning "" if not found
std::string getPrivateAdbKeyPath();

bool adb_auth_keygen(const char* filename);

// Creates a public key given the private key.
// |path| Path to the adb private key.
// |out| string receiving the public key.
bool pubkey_from_privkey(const std::string& path, std::string* out);


/* Encodes |key| in the Android RSA public key binary format and stores the
 * bytes in |key_buffer|. |key_buffer| should be of size at least
 * |ANDROID_PUBKEY_ENCODED_SIZE|.
 *
 * Returns true if successful, false on error.
 */
bool android_pubkey_encode(const RSA* key, uint8_t* key_buffer, size_t size);
bool android_pubkey_decode(const uint8_t* key_buffer, size_t size, RSA** key);
bool sign_auth_token(const uint8_t* token, int token_size, uint8_t* sig, int& siglen);
bool calculate_public_key(std::string* out, RSA* private_key);

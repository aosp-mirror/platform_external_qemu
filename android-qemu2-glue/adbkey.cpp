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

#include "android-qemu2-glue/android_pubkey.h"

#include <openssl/base64.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <string>
#include <vector>

// Size of an RSA modulus such as an encrypted block or a signature.
#define ANDROID_PUBKEY_MODULUS_SIZE (2048 / 8)

// Size of an encoded RSA key.
#define ANDROID_PUBKEY_ENCODED_SIZE \
    (3 * sizeof(uint32_t) + 2 * ANDROID_PUBKEY_MODULUS_SIZE)

// From ${AOSP}/system/core/adb/client/auth.cpp
static bool calculate_public_key(std::string* out, RSA* private_key) {
    uint8_t binary_key_data[ANDROID_PUBKEY_ENCODED_SIZE];
    if (!android_pubkey_encode(private_key, binary_key_data,
                               sizeof(binary_key_data))) {
        fprintf(stderr, "Failed to convert to public key\n");
        return false;
    }

    size_t expected_length;
    if (!EVP_EncodedLength(&expected_length, sizeof(binary_key_data))) {
        fprintf(stderr, "Public key too large to base64 encode\n");
        return false;
    }

    std::vector<char> buffer(expected_length);
    size_t actual_length =
            EVP_EncodeBlock(reinterpret_cast<uint8_t*>(buffer.data()),
                            binary_key_data, sizeof(binary_key_data));
    out->assign(buffer.data(), actual_length);
    return true;
}

static std::shared_ptr<RSA> read_key_file(const std::string& file) {
    std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(file.c_str(), "r"),
                                                fclose);
    if (!fp) {
        fprintf(stderr, "Failed to open '%s'\n", file.c_str());
        return nullptr;
    }

    RSA* key = RSA_new();
    if (!PEM_read_RSAPrivateKey(fp.get(), &key, nullptr, nullptr)) {
        fprintf(stderr, "Failed to read key\n");
        RSA_free(key);
        return nullptr;
    }

    return std::shared_ptr<RSA>(key, RSA_free);
}

bool pubkey_from_privkey(const std::string& path, std::string* out) {
    std::shared_ptr<RSA> privkey = read_key_file(path);
    if (!privkey) {
        return false;
    }
    return calculate_public_key(out, privkey.get());
}

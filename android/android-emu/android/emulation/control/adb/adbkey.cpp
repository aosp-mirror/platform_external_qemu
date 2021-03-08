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

#include "android/emulation/control/adb/adbkey.h"

#include <openssl/base.h>                  // for RSA, BIGNUM, BN_CTX, EVP_PKEY
#include <openssl/base64.h>                // for EVP_EncodeBlock, EVP_Encod...
#include <openssl/bn.h>                    // for BN_new, BN_free, BN_get_word
#include <openssl/evp.h>                   // for EVP_PKEY_free, EVP_PKEY_new
#include <openssl/nid.h>                   // for NID_sha1
#include <openssl/pem.h>                   // for PEM_read_RSAPrivateKey
#include <openssl/rsa.h>                   // for RSA_free, RSA_new, rsa_st

#include <string.h>                        // for memcpy
#include <sys/types.h>                     // for mode_t
#include <cstdio>                          // for fclose, size_t, fopen, FILE
#include <memory>                          // for shared_ptr, unique_ptr
#include <string>                          // for string, basic_string
#include <vector>                          // for vector

#include "android/base/Log.h"              // for LOG, LogMessage, LogStream
#include "android/base/files/PathUtils.h"  // for PathUtils
#include "android/base/system/System.h"    // for System
#include "android/emulation/ConfigDirs.h"  // for ConfigDirs
#include "android/utils/debug.h"           // for dwarning, VERBOSE_PRINT
#include "android/utils/file_io.h"         // for android_chmod
#include "android/utils/path.h"            // for path_can_read, path_is_reg...

/* set >0 for very verbose debugging */
#define DEBUG 0

#define D(...) (void)0
#define DD(...) (void)0
#if DEBUG >= 1
#undef D
#define D(fmt, ...)                                                 \
    fprintf(stderr, "adbkey: %s:%d| " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__)
#endif
#if DEBUG >= 2
#undef DD
#define DD(fmt, ...)                                                \
    fprintf(stderr, "adbkey: %s:%d| " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__)
#endif


using android::base::PathUtils;
using android::base::System;

// Better safe than sorry.
static_assert(ANDROID_PUBKEY_MODULUS_SIZE % 4 == 0,
              "RSA modulus size must be multiple of the word size!");

// Size of the RSA modulus in words.
constexpr const int ANDROID_PUBKEY_MODULUS_SIZE_WORDS =
        ANDROID_PUBKEY_MODULUS_SIZE / 4;


namespace {
std::string get_user_info() {
    std::string hostname = System::get()->getEnvironmentVariable("HOSTNAME");
    if (hostname.empty()) hostname = "unknown";

    std::string username;
    if (getenv("LOGNAME")) username = getenv("LOGNAME");
    if (username.empty()) hostname = "unknown";
    return " " + username + "@" + hostname;
}

}

// From ${AOSP}/system/core/adb/client/auth.cpp
bool calculate_public_key(std::string* out, RSA* private_key) {
    uint8_t binary_key_data[ANDROID_PUBKEY_ENCODED_SIZE];
    if (!android_pubkey_encode(private_key, binary_key_data,
                               sizeof(binary_key_data))) {
        LOG(ERROR) << "Failed to convert to public key";
        return false;
    }

    size_t expected_length;
    if (!EVP_EncodedLength(&expected_length, sizeof(binary_key_data))) {
        LOG(ERROR) << "Public key too large to base64 encode";
        return false;
    }

    out->resize(expected_length);
    size_t actual_length =
            EVP_EncodeBlock((uint8_t*) out->data(),
                            binary_key_data, sizeof(binary_key_data));
    out->resize(actual_length);
    out->append(get_user_info());
    return true;
}

static std::shared_ptr<RSA> read_key_file(const std::string& file) {
    std::unique_ptr<FILE, decltype(&fclose)> fp(fopen(file.c_str(), "r"),
                                                fclose);
    if (!fp) {
        LOG(ERROR) << "Failed to open rsa file: " << file;
        return nullptr;
    }

    RSA* key = RSA_new();
    if (!PEM_read_RSAPrivateKey(fp.get(), &key, nullptr, nullptr)) {
        LOG(ERROR) << "Failed to read rsa key";
        RSA_free(key);
        return nullptr;
    }

    return std::shared_ptr<RSA>(key, RSA_free);
}

static bool generate_key(const std::string& file) {
    VERBOSE_PRINT(init, "generate_key(%s)...", file.c_str());

    mode_t old_mask;
    FILE* f = nullptr;
    bool ret = 0;

    EVP_PKEY* pkey = EVP_PKEY_new();
    BIGNUM* exponent = BN_new();
    RSA* rsa = RSA_new();
    if (!pkey || !exponent || !rsa) {
        dwarning("Failed to allocate key");
        goto out;
    }

    BN_set_word(exponent, RSA_F4);
    RSA_generate_key_ex(rsa, 2048, exponent, nullptr);
    EVP_PKEY_set1_RSA(pkey, rsa);

    f = fopen(file.c_str(), "w");
    if (!f) {
        dwarning("Failed to open %s", file.c_str());
        goto out;
    }

    if (!PEM_write_PrivateKey(f, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
        dwarning("Failed to write key");
        goto out;
    }

    fclose(f);
    f = NULL;
    android_chmod(file.c_str(), 0777);

    ret = true;

out:
    if (f) {
        fclose(f);
    }
    EVP_PKEY_free(pkey);
    RSA_free(rsa);
    BN_free(exponent);
    return ret;
}

bool adb_auth_keygen(const char* filename) {
    return generate_key(filename);
}

bool pubkey_from_privkey(const std::string& path, std::string* out) {
    std::shared_ptr<RSA> privkey = read_key_file(path);
    if (!privkey) {
        return false;
    }
    return calculate_public_key(out, privkey.get());
}

// Get adbkey path, return "" if failed
// adbKeyFileName could be "adbkey" or "adbkey.pub"
std::string getAdbKeyPath(const char* adbKeyFileName) {
    std::string adbKeyPath = PathUtils::join(
            android::ConfigDirs::getUserDirectory(), adbKeyFileName);
    if (path_is_regular(adbKeyPath.c_str()) &&
        path_can_read(adbKeyPath.c_str())) {
        return adbKeyPath;
    }
    D("cannot read adb key file: %s", adbKeyPath.c_str());
    D("trying again by copying from home dir");

    auto home = System::get()->getHomeDirectory();
    if (home.empty()) {
        home = System::get()->getTempDir();
        if (home.empty()) {
            home = "/tmp";
        }
    }
    D("Looking in %s", home.c_str());

    auto guessedSrcAdbKeyPub =
            PathUtils::join(home, ".android", adbKeyFileName);
    path_copy_file(adbKeyPath.c_str(), guessedSrcAdbKeyPub.c_str());

    if (path_is_regular(adbKeyPath.c_str()) &&
        path_can_read(adbKeyPath.c_str())) {
        return adbKeyPath;
    }
    D("cannot read adb key file (failed): %s", adbKeyPath.c_str());
    return "";
}

std::string getPublicAdbKeyPath() {
    return getAdbKeyPath(kPublicKeyFileName);
}

std::string getPrivateAdbKeyPath() {
    return getAdbKeyPath(kPrivateKeyFileName);
}

// This file implements encoding and decoding logic for Android's custom RSA
// public key binary format. Public keys are stored as a sequence of
// little-endian 32 bit words. Note that Android only supports little-endian
// processors, so we don't do any byte order conversions when parsing the binary
// struct.
typedef struct RSAPublicKey {
    // Modulus length. This must be ANDROID_PUBKEY_MODULUS_SIZE.
    uint32_t modulus_size_words;

    // Precomputed montgomery parameter: -1 / n[0] mod 2^32
    uint32_t n0inv;

    // RSA modulus as a little-endian array.
    uint8_t modulus[ANDROID_PUBKEY_MODULUS_SIZE];

    // Montgomery parameter R^2 as a little-endian array of little-endian words.
    uint8_t rr[ANDROID_PUBKEY_MODULUS_SIZE];

    // RSA modulus: 3 or 65537
    uint32_t exponent;
} RSAPublicKey;

// Reverses byte order in |buffer|.
static void reverse_bytes(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < (size + 1) / 2; ++i) {
        uint8_t tmp = buffer[i];
        buffer[i] = buffer[size - i - 1];
        buffer[size - i - 1] = tmp;
    }
}

bool android_pubkey_decode(const uint8_t* key_buffer, size_t size, RSA** key) {
    const RSAPublicKey* key_struct = (RSAPublicKey*)key_buffer;
    bool ret = false;
    uint8_t modulus_buffer[ANDROID_PUBKEY_MODULUS_SIZE];
    RSA* new_key = RSA_new();
    if (!new_key) {
        goto cleanup;
    }

    // Check |size| is large enough and the modulus size is correct.
    if (size < sizeof(RSAPublicKey)) {
        goto cleanup;
    }
    if (key_struct->modulus_size_words != ANDROID_PUBKEY_MODULUS_SIZE_WORDS) {
        goto cleanup;
    }

    // Convert the modulus to big-endian byte order as expected by BN_bin2bn.
    memcpy(modulus_buffer, key_struct->modulus, sizeof(modulus_buffer));
    reverse_bytes(modulus_buffer, sizeof(modulus_buffer));
    new_key->n = BN_bin2bn(modulus_buffer, sizeof(modulus_buffer), NULL);
    if (!new_key->n) {
        goto cleanup;
    }

    // Read the exponent.
    new_key->e = BN_new();
    if (!new_key->e || !BN_set_word(new_key->e, key_struct->exponent)) {
        goto cleanup;
    }

    // Note that we don't extract the montgomery parameters n0inv and rr from
    // the RSAPublicKey structure. They assume a word size of 32 bits, but
    // BoringSSL may use a word size of 64 bits internally, so we're lacking the
    // top 32 bits of n0inv in general. For now, we just ignore the parameters
    // and have BoringSSL recompute them internally. More sophisticated logic
    // can be added here if/when we want the additional speedup from using the
    // pre-computed montgomery parameters.

    *key = new_key;
    ret = true;

cleanup:
    if (!ret) {
        LOG(ERROR) << "WARNING: adbkey decode failed.";
        if (new_key) {
            RSA_free(new_key);
        }
    }
    return ret;
}

static bool android_pubkey_encode_bignum(const BIGNUM* num, uint8_t* buffer) {
    if (!BN_bn2bin_padded(buffer, ANDROID_PUBKEY_MODULUS_SIZE, num)) {
        return false;
    }

    reverse_bytes(buffer, ANDROID_PUBKEY_MODULUS_SIZE);
    return true;
}

bool android_pubkey_encode(const RSA* key, uint8_t* key_buffer, size_t size) {
    RSAPublicKey* key_struct = (RSAPublicKey*)key_buffer;
    bool ret = false;
    BN_CTX* ctx = BN_CTX_new();
    BIGNUM* r32 = BN_new();
    BIGNUM* n0inv = BN_new();
    BIGNUM* rr = BN_new();

    if (sizeof(RSAPublicKey) > size ||
        RSA_size(key) != ANDROID_PUBKEY_MODULUS_SIZE) {
        goto cleanup;
    }

    // Store the modulus size.
    key_struct->modulus_size_words = ANDROID_PUBKEY_MODULUS_SIZE_WORDS;

    // Compute and store n0inv = -1 / N[0] mod 2^32.
    if (!ctx || !r32 || !n0inv || !BN_set_bit(r32, 32) ||
        !BN_mod(n0inv, key->n, r32, ctx) ||
        !BN_mod_inverse(n0inv, n0inv, r32, ctx) || !BN_sub(n0inv, r32, n0inv)) {
        goto cleanup;
    }
    key_struct->n0inv = (uint32_t)BN_get_word(n0inv);

    // Store the modulus.
    if (!android_pubkey_encode_bignum(key->n, key_struct->modulus)) {
        goto cleanup;
    }

    // Compute and store rr = (2^(rsa_size)) ^ 2 mod N.
    if (!ctx || !rr || !BN_set_bit(rr, ANDROID_PUBKEY_MODULUS_SIZE * 8) ||
        !BN_mod_sqr(rr, rr, key->n, ctx) ||
        !android_pubkey_encode_bignum(rr, key_struct->rr)) {
        goto cleanup;
    }

    // Store the exponent.
    key_struct->exponent = (uint32_t)BN_get_word(key->e);

    ret = true;

cleanup:
    BN_free(rr);
    BN_free(n0inv);
    BN_free(r32);
    BN_CTX_free(ctx);
    return ret;
}

static bool sign_token(RSA* key_rsa,
                       const uint8_t* token,
                       int token_size,
                       uint8_t* sig,
                       int& len) {
    if (token_size != TOKEN_SIZE) {
        DD("Unexpected token size %d\n", token_size);
    }

    if (!RSA_sign(NID_sha1, token, (size_t)token_size, sig, (unsigned int*)&len,
                  key_rsa)) {
        return false;
    }

    DD("successfully signed with siglen %d\n", (int)len);
    return true;
}

bool sign_auth_token(const uint8_t* token,
                     int token_size,
                     uint8_t* sig,
                     int& siglen) {
    const std::string key_path = getAdbKeyPath(kPrivateKeyFileName);
    if (key_path.empty()) {
        LOG(ERROR) << "No private key found, unable to sign token";
    }
    auto rsa = read_key_file(key_path);
    if (!rsa) {
        LOG(ERROR) << "No RSA key available.";
        return false;
    }
    return sign_token(rsa.get(), token, token_size, sig, siglen);
}

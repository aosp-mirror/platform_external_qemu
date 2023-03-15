// Copyright (C) 2020 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/control/test/CertificateFactory.h"

#include <openssl/evp.h>                   // for EVP_PKEY_assign_RSA, EVP_P...
#include <openssl/pem.h>                   // for PEM_write_X509, PEM_write_...
#include <openssl/rsa.h>                   // for RSA_free, RSA_generate_key_ex
#include <stdio.h>                         // for NULL, fclose, FILE
#include <chrono>                          // for seconds
#include <utility>                         // for __unwrap_reference<>::type

#include "aemu/base/files/PathUtils.h"  // for pj, PathUtils (ptr only)
#include "aemu/base/logging/Log.h"
#include "android/utils/file_io.h"         // for android_fopen
#include "openssl/asn1.h"                  // for ASN1_INTEGER_set, MBSTRING...
#include "openssl/base.h"                  // for EVP_PKEY, BIO, BIGNUM, RSA
#include "openssl/bio.h"                   // for BIO_free_all, BIO_new_file
#include "openssl/bn.h"                    // for BN_free, BN_new, BN_set_word
#include "openssl/digest.h"                // for EVP_sha1
#include "openssl/x509.h"                  // for X509_NAME_add_entry_by_txt

static bool generate_rsa_key(int bits,
                             const char* public_pem,
                             const char* private_pem,
                             EVP_PKEY** ppKey) {
    int success = 0;
    EVP_PKEY* pkey = EVP_PKEY_new();
    RSA* r = RSA_new();
    BIGNUM* bne = BN_new();
    BIO *bp_public = NULL, *bp_private = NULL;
    unsigned long e = RSA_F4;

    success = BN_set_word(bne, e);
    if (!success) {
        goto exit;
    }

    success = RSA_generate_key_ex(r, bits, bne, NULL);
    if (!success) {
        goto exit;
    }

    bp_public = BIO_new_file(public_pem, "w+");
    success = PEM_write_bio_RSAPublicKey(bp_public, r);
    if (!success) {
        goto exit;
    }

    bp_private = BIO_new_file(private_pem, "w+");
    success = PEM_write_bio_RSAPrivateKey(bp_private, r, NULL, NULL, 0, NULL,
                                          NULL);

    if (!success) {
        goto exit;
    }

    EVP_PKEY_assign_RSA(pkey, r);
    r = nullptr;
    *ppKey = pkey;
    pkey = nullptr;

exit:
    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    RSA_free(r);
    BN_free(bne);
    return (success == 1);
}

static bool mkcert(EVP_PKEY* pk,
                   std::chrono::seconds validFor,
                   const char* pemFile) {
    bool success = false;
    X509* x = X509_new();
    X509_NAME* name = nullptr;
    FILE* f = nullptr;

    X509_set_version(x, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(x), 1);
    X509_gmtime_adj(X509_get_notBefore(x), 0);
    X509_gmtime_adj(X509_get_notAfter(x), static_cast<long>(validFor.count()));
    X509_set_pubkey(x, pk);

    name = X509_get_subject_name(x);

    X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"CA",
                               -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               (unsigned char*)"localhost", -1, -1, 0);

    // Self signed so set the issuer name to be the same as the
    // subject.
    X509_set_issuer_name(x, name);
    if (!X509_sign(x, pk, EVP_sha1()))
        goto exit;

    f = android_fopen(pemFile, "wb");
    if (!f) {
        goto exit;
    }

    if (!PEM_write_X509(f,x)) {
        goto exit;
    }

    success = true;

exit:
    if (f) {
        fclose(f);
    };
    X509_free(x);
    return success;
}

using android::base::PathUtils;
using android::emulation::control::CertificateFactory;

std::tuple<std::string, std::string> CertificateFactory::generateCertKeyPair(
        std::string dir,
        std::string prefix) {
    std::string pub = prefix + "_public.key";
    std::string pem = prefix + "_public.pem";
    std::string key = prefix + "_private.key";
    EVP_PKEY* pKey = nullptr;

    auto pubFile = base::pj(dir, pub);
    auto keyFile = base::pj(dir, key);
    auto cert = base::pj(dir, pem);
    if (!generate_rsa_key(2048, pubFile.c_str(), keyFile.c_str(), &pKey)) {
        return std::make_tuple("", "");
    }

    if (!mkcert(pKey, std::chrono::hours(24 * 360), cert.c_str())) {
        return std::make_tuple("", "");
    }

    LOG(INFO) << "Created key " << keyFile << ", cer: " << cert;
    EVP_PKEY_free(pKey);
    return std::make_tuple(keyFile, cert);
}

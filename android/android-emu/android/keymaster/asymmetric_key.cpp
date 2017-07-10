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

#include "asymmetric_key.h"

#include <new>

#include <openssl/asn1.h>
#include <openssl/stack.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "attestation_record.h"
#include "openssl_err.h"
#include "openssl_utils.h"

namespace keymaster {

namespace {

constexpr int kDigitalSignatureKeyUsageBit = 0;
constexpr int kKeyEnciphermentKeyUsageBit = 2;
constexpr int kDataEnciphermentKeyUsageBit = 3;
constexpr int kMaxKeyUsageBit = 8;

template <typename T> T min(T a, T b) {
    return (a < b) ? a : b;
}

static keymaster_error_t add_key_usage_extension(const AuthorizationSet& tee_enforced,
                                                 const AuthorizationSet& sw_enforced,
                                                 X509* certificate) {
    // Build BIT_STRING with correct contents.
    ASN1_BIT_STRING_Ptr key_usage(ASN1_BIT_STRING_new());

    for (size_t i = 0; i <= kMaxKeyUsageBit; ++i) {
        if (!ASN1_BIT_STRING_set_bit(key_usage.get(), i, 0)) {
            return TranslateLastOpenSslError();
        }
    }

    if (tee_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_SIGN) ||
        tee_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_VERIFY) ||
        sw_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_SIGN) ||
        sw_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_VERIFY)) {
        if (!ASN1_BIT_STRING_set_bit(key_usage.get(), kDigitalSignatureKeyUsageBit, 1)) {
            return TranslateLastOpenSslError();
        }
    }

    if (tee_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_ENCRYPT) ||
        tee_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_DECRYPT) ||
        sw_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_ENCRYPT) ||
        sw_enforced.Contains(TAG_PURPOSE, KM_PURPOSE_DECRYPT)) {
        if (!ASN1_BIT_STRING_set_bit(key_usage.get(), kKeyEnciphermentKeyUsageBit, 1) ||
            !ASN1_BIT_STRING_set_bit(key_usage.get(), kDataEnciphermentKeyUsageBit, 1)) {
            return TranslateLastOpenSslError();
        }
    }

    // Convert to octets
    int len = i2d_ASN1_BIT_STRING(key_usage.get(), nullptr);
    if (len < 0) {
        return TranslateLastOpenSslError();
    }
    UniquePtr<uint8_t[]> asn1_key_usage(new uint8_t[len]);
    if (!asn1_key_usage.get()) {
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;
    }
    uint8_t* p = asn1_key_usage.get();
    len = i2d_ASN1_BIT_STRING(key_usage.get(), &p);
    if (len < 0) {
        return TranslateLastOpenSslError();
    }

    // Build OCTET_STRING
    ASN1_OCTET_STRING_Ptr key_usage_str(ASN1_OCTET_STRING_new());
    if (!key_usage_str.get() ||
        !ASN1_OCTET_STRING_set(key_usage_str.get(), asn1_key_usage.get(), len)) {
        return TranslateLastOpenSslError();
    }

    X509_EXTENSION_Ptr key_usage_extension(X509_EXTENSION_create_by_NID(nullptr,        //
                                                                        NID_key_usage,  //
                                                                        false /* critical */,
                                                                        key_usage_str.get()));
    if (!key_usage_extension.get()) {
        return TranslateLastOpenSslError();
    }

    if (!X509_add_ext(certificate, key_usage_extension.get() /* Don't release; copied */,
                      -1 /* insert at end */)) {
        return TranslateLastOpenSslError();
    }

    return KM_ERROR_OK;
}

}  // anonymous namespace

keymaster_error_t AsymmetricKey::formatted_key_material(keymaster_key_format_t format,
                                                        UniquePtr<uint8_t[]>* material,
                                                        size_t* size) const {
    if (format != KM_KEY_FORMAT_X509)
        return KM_ERROR_UNSUPPORTED_KEY_FORMAT;

    if (material == NULL || size == NULL)
        return KM_ERROR_OUTPUT_PARAMETER_NULL;

    EVP_PKEY_Ptr pkey(EVP_PKEY_new());
    if (!InternalToEvp(pkey.get()))
        return TranslateLastOpenSslError();

    int key_data_length = i2d_PUBKEY(pkey.get(), NULL);
    if (key_data_length <= 0)
        return TranslateLastOpenSslError();

    material->reset(new (std::nothrow) uint8_t[key_data_length]);
    if (material->get() == NULL)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    uint8_t* tmp = material->get();
    if (i2d_PUBKEY(pkey.get(), &tmp) != key_data_length) {
        material->reset();
        return TranslateLastOpenSslError();
    }

    *size = key_data_length;
    return KM_ERROR_OK;
}

static keymaster_error_t build_attestation_extension(const AuthorizationSet& attest_params,
                                                     const AuthorizationSet& tee_enforced,
                                                     const AuthorizationSet& sw_enforced,
                                                     const KeymasterContext& context,
                                                     X509_EXTENSION_Ptr* extension) {
    ASN1_OBJECT_Ptr oid(
        OBJ_txt2obj(kAttestionRecordOid, 1 /* accept numerical dotted string form only */));
    if (!oid.get())
        return TranslateLastOpenSslError();

    UniquePtr<uint8_t[]> attest_bytes;
    size_t attest_bytes_len;
    keymaster_error_t error = build_attestation_record(attest_params, sw_enforced, tee_enforced,
                                                       context, &attest_bytes, &attest_bytes_len);
    if (error != KM_ERROR_OK)
        return error;

    ASN1_OCTET_STRING_Ptr attest_str(ASN1_OCTET_STRING_new());
    if (!attest_str.get() ||
        !ASN1_OCTET_STRING_set(attest_str.get(), attest_bytes.get(), attest_bytes_len))
        return TranslateLastOpenSslError();

    extension->reset(
        X509_EXTENSION_create_by_OBJ(nullptr, oid.get(), 0 /* not critical */, attest_str.get()));
    if (!extension->get())
        return TranslateLastOpenSslError();

    return KM_ERROR_OK;
}

static bool add_public_key(EVP_PKEY* key, X509* certificate, keymaster_error_t* error) {
    if (!X509_set_pubkey(certificate, key)) {
        *error = TranslateLastOpenSslError();
        return false;
    }
    return true;
}

static bool add_attestation_extension(const AuthorizationSet& attest_params,
                                      const AuthorizationSet& tee_enforced,
                                      const AuthorizationSet& sw_enforced,
                                      const KeymasterContext& context, X509* certificate,
                                      keymaster_error_t* error) {
    X509_EXTENSION_Ptr attest_extension;
    *error = build_attestation_extension(attest_params, tee_enforced, sw_enforced, context,
                                         &attest_extension);
    if (*error != KM_ERROR_OK)
        return false;

    if (!X509_add_ext(certificate, attest_extension.get() /* Don't release; copied */,
                      -1 /* insert at end */)) {
        *error = TranslateLastOpenSslError();
        return false;
    }

    return true;
}

static keymaster_error_t get_certificate_blob(X509* certificate, keymaster_blob_t* blob) {
    int len = i2d_X509(certificate, nullptr);
    if (len < 0)
        return TranslateLastOpenSslError();

    uint8_t* data = new uint8_t[len];
    if (!data)
        return KM_ERROR_MEMORY_ALLOCATION_FAILED;

    uint8_t* p = data;
    i2d_X509(certificate, &p);

    blob->data_length = len;
    blob->data = data;

    return KM_ERROR_OK;
}

static bool allocate_cert_chain(size_t entry_count, keymaster_cert_chain_t* chain,
                                keymaster_error_t* error) {
    if (chain->entries) {
        for (size_t i = 0; i < chain->entry_count; ++i)
            delete[] chain->entries[i].data;
        delete[] chain->entries;
    }

    chain->entry_count = entry_count;
    chain->entries = new keymaster_blob_t[entry_count];
    if (!chain->entries) {
        *error = KM_ERROR_MEMORY_ALLOCATION_FAILED;
        return false;
    }
    return true;
}

// Copies the intermediate and root certificates into chain, leaving the first slot for the leaf
// certificate.
static bool copy_attestation_chain(const KeymasterContext& context,
                                   keymaster_algorithm_t sign_algorithm,
                                   keymaster_cert_chain_t* chain, keymaster_error_t* error) {

    UniquePtr<keymaster_cert_chain_t, CertificateChainDelete> attest_key_chain(
        context.AttestationChain(sign_algorithm, error));
    if (!attest_key_chain.get())
        return false;

    if (!allocate_cert_chain(attest_key_chain->entry_count + 1, chain, error))
        return false;

    chain->entries[0].data = nullptr;  // Leave empty for the leaf certificate.
    chain->entries[1].data_length = 0;

    for (size_t i = 0; i < attest_key_chain->entry_count; ++i) {
        chain->entries[i + 1] = attest_key_chain->entries[i];
        attest_key_chain->entries[i].data = nullptr;
    }

    return true;
}

keymaster_error_t AsymmetricKey::GenerateAttestation(const KeymasterContext& context,
                                                     const AuthorizationSet& attest_params,
                                                     const AuthorizationSet& tee_enforced,
                                                     const AuthorizationSet& sw_enforced,
                                                     keymaster_cert_chain_t* cert_chain) const {

    keymaster_algorithm_t sign_algorithm;
    if ((!sw_enforced.GetTagValue(TAG_ALGORITHM, &sign_algorithm) &&
         !tee_enforced.GetTagValue(TAG_ALGORITHM, &sign_algorithm)))
        return KM_ERROR_UNKNOWN_ERROR;

    if ((sign_algorithm != KM_ALGORITHM_RSA && sign_algorithm != KM_ALGORITHM_EC))
        return KM_ERROR_INCOMPATIBLE_ALGORITHM;

    EVP_PKEY_Ptr pkey(EVP_PKEY_new());
    if (!InternalToEvp(pkey.get()))
        return TranslateLastOpenSslError();

    X509_Ptr certificate(X509_new());
    if (!certificate.get())
        return TranslateLastOpenSslError();

    if (!X509_set_version(certificate.get(), 2 /* version 3, but zero-based */))
        return TranslateLastOpenSslError();

    ASN1_INTEGER_Ptr serialNumber(ASN1_INTEGER_new());
    if (!serialNumber.get() || !ASN1_INTEGER_set(serialNumber.get(), 1) ||
        !X509_set_serialNumber(certificate.get(), serialNumber.get() /* Don't release; copied */))
        return TranslateLastOpenSslError();

    // TODO(swillden): Find useful values (if possible) for issuerName and subjectName.
    X509_NAME_Ptr issuerName(X509_NAME_new());
    if (!issuerName.get() ||
        !X509_NAME_add_entry_by_txt(issuerName.get(), "CN", MBSTRING_ASC,
                                    reinterpret_cast<const uint8_t*>("Android Keymaster"),
                                    -1 /* len */, -1 /* loc */, 0 /* set */) ||
        !X509_set_issuer_name(certificate.get(), issuerName.get() /* Don't release; copied  */))
        return TranslateLastOpenSslError();

    X509_NAME_Ptr subjectName(X509_NAME_new());
    if (!subjectName.get() ||
        !X509_NAME_add_entry_by_txt(subjectName.get(), "CN", MBSTRING_ASC,
                                    reinterpret_cast<const uint8_t*>("A Keymaster Key"),
                                    -1 /* len */, -1 /* loc */, 0 /* set */) ||
        !X509_set_subject_name(certificate.get(), subjectName.get() /* Don't release; copied */))
        return TranslateLastOpenSslError();

    ASN1_TIME_Ptr notBefore(ASN1_TIME_new());
    uint64_t activeDateTime = 0;
    authorizations().GetTagValue(TAG_ACTIVE_DATETIME, &activeDateTime);
    if (!notBefore.get() || !ASN1_TIME_set(notBefore.get(), activeDateTime / 1000) ||
        !X509_set_notBefore(certificate.get(), notBefore.get() /* Don't release; copied */))
        return TranslateLastOpenSslError();

    ASN1_TIME_Ptr notAfter(ASN1_TIME_new());
    uint64_t usageExpireDateTime = UINT64_MAX;
    authorizations().GetTagValue(TAG_USAGE_EXPIRE_DATETIME, &usageExpireDateTime);
    // TODO(swillden): When trusty can use the C++ standard library change the calculation of
    // notAfterTime to use std::numeric_limits<time_t>::max(), rather than assuming that time_t is
    // 32 bits.
    time_t notAfterTime = min(static_cast<uint64_t>(UINT32_MAX), usageExpireDateTime / 1000);
    if (!notAfter.get() || !ASN1_TIME_set(notAfter.get(), notAfterTime) ||
        !X509_set_notAfter(certificate.get(), notAfter.get() /* Don't release; copied */))
        return TranslateLastOpenSslError();

    keymaster_error_t error = add_key_usage_extension(tee_enforced, sw_enforced, certificate.get());
    if (error != KM_ERROR_OK) {
        return error;
    }

    EVP_PKEY_Ptr sign_key(context.AttestationKey(sign_algorithm, &error));

    if (!sign_key.get() ||  //
        !add_public_key(pkey.get(), certificate.get(), &error) ||
        !add_attestation_extension(attest_params, tee_enforced, sw_enforced, context,
                                   certificate.get(), &error))
        return error;

    if (!copy_attestation_chain(context, sign_algorithm, cert_chain, &error))
        return error;

    // Copy subject key identifier from cert_chain->entries[1] as authority key_id.
    if (cert_chain->entry_count < 2) {
        // cert_chain must have at least two entries, one for the cert we're trying to create and
        // one for the cert for the key that signs the new cert.
        return KM_ERROR_UNKNOWN_ERROR;
    }

    const uint8_t* p = cert_chain->entries[1].data;
    X509_Ptr signing_cert(d2i_X509(nullptr, &p, cert_chain->entries[1].data_length));
    if (!signing_cert.get()) {
        return TranslateLastOpenSslError();
    }

    UniquePtr<X509V3_CTX> x509v3_ctx(new X509V3_CTX);
    *x509v3_ctx = {};
    X509V3_set_ctx(x509v3_ctx.get(), signing_cert.get(), certificate.get(), nullptr /* req */,
                   nullptr /* crl */, 0 /* flags */);

    X509_EXTENSION_Ptr auth_key_id(X509V3_EXT_nconf_nid(nullptr /* conf */, x509v3_ctx.get(),
                                                        NID_authority_key_identifier,
                                                        const_cast<char*>("keyid:always")));
    if (!auth_key_id.get() ||
        !X509_add_ext(certificate.get(), auth_key_id.get() /* Don't release; copied */,
                      -1 /* insert at end */)) {
        return TranslateLastOpenSslError();
    }

    if (!X509_sign(certificate.get(), sign_key.get(), EVP_sha256()))
        return TranslateLastOpenSslError();

    return get_certificate_blob(certificate.get(), &cert_chain->entries[0]);
}

}  // namespace keymaster

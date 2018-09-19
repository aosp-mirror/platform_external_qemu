/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI types
 *
 * Copyright IBM, Corp. 2011
 * Copyright (c) 2013-2018 Red Hat Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qapi/dealloc-visitor.h"
#include "qapi-types-crypto.h"
#include "qapi-visit-crypto.h"

const QEnumLookup QCryptoTLSCredsEndpoint_lookup = {
    .array = (const char *const[]) {
        [QCRYPTO_TLS_CREDS_ENDPOINT_CLIENT] = "client",
        [QCRYPTO_TLS_CREDS_ENDPOINT_SERVER] = "server",
    },
    .size = QCRYPTO_TLS_CREDS_ENDPOINT__MAX
};

const QEnumLookup QCryptoSecretFormat_lookup = {
    .array = (const char *const[]) {
        [QCRYPTO_SECRET_FORMAT_RAW] = "raw",
        [QCRYPTO_SECRET_FORMAT_BASE64] = "base64",
    },
    .size = QCRYPTO_SECRET_FORMAT__MAX
};

const QEnumLookup QCryptoHashAlgorithm_lookup = {
    .array = (const char *const[]) {
        [QCRYPTO_HASH_ALG_MD5] = "md5",
        [QCRYPTO_HASH_ALG_SHA1] = "sha1",
        [QCRYPTO_HASH_ALG_SHA224] = "sha224",
        [QCRYPTO_HASH_ALG_SHA256] = "sha256",
        [QCRYPTO_HASH_ALG_SHA384] = "sha384",
        [QCRYPTO_HASH_ALG_SHA512] = "sha512",
        [QCRYPTO_HASH_ALG_RIPEMD160] = "ripemd160",
    },
    .size = QCRYPTO_HASH_ALG__MAX
};

const QEnumLookup QCryptoCipherAlgorithm_lookup = {
    .array = (const char *const[]) {
        [QCRYPTO_CIPHER_ALG_AES_128] = "aes-128",
        [QCRYPTO_CIPHER_ALG_AES_192] = "aes-192",
        [QCRYPTO_CIPHER_ALG_AES_256] = "aes-256",
        [QCRYPTO_CIPHER_ALG_DES_RFB] = "des-rfb",
        [QCRYPTO_CIPHER_ALG_3DES] = "3des",
        [QCRYPTO_CIPHER_ALG_CAST5_128] = "cast5-128",
        [QCRYPTO_CIPHER_ALG_SERPENT_128] = "serpent-128",
        [QCRYPTO_CIPHER_ALG_SERPENT_192] = "serpent-192",
        [QCRYPTO_CIPHER_ALG_SERPENT_256] = "serpent-256",
        [QCRYPTO_CIPHER_ALG_TWOFISH_128] = "twofish-128",
        [QCRYPTO_CIPHER_ALG_TWOFISH_192] = "twofish-192",
        [QCRYPTO_CIPHER_ALG_TWOFISH_256] = "twofish-256",
    },
    .size = QCRYPTO_CIPHER_ALG__MAX
};

const QEnumLookup QCryptoCipherMode_lookup = {
    .array = (const char *const[]) {
        [QCRYPTO_CIPHER_MODE_ECB] = "ecb",
        [QCRYPTO_CIPHER_MODE_CBC] = "cbc",
        [QCRYPTO_CIPHER_MODE_XTS] = "xts",
        [QCRYPTO_CIPHER_MODE_CTR] = "ctr",
    },
    .size = QCRYPTO_CIPHER_MODE__MAX
};

const QEnumLookup QCryptoIVGenAlgorithm_lookup = {
    .array = (const char *const[]) {
        [QCRYPTO_IVGEN_ALG_PLAIN] = "plain",
        [QCRYPTO_IVGEN_ALG_PLAIN64] = "plain64",
        [QCRYPTO_IVGEN_ALG_ESSIV] = "essiv",
    },
    .size = QCRYPTO_IVGEN_ALG__MAX
};

const QEnumLookup QCryptoBlockFormat_lookup = {
    .array = (const char *const[]) {
        [Q_CRYPTO_BLOCK_FORMAT_QCOW] = "qcow",
        [Q_CRYPTO_BLOCK_FORMAT_LUKS] = "luks",
    },
    .size = Q_CRYPTO_BLOCK_FORMAT__MAX
};

void qapi_free_QCryptoBlockOptionsBase(QCryptoBlockOptionsBase *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockOptionsBase(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockOptionsQCow(QCryptoBlockOptionsQCow *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockOptionsQCow(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockOptionsLUKS(QCryptoBlockOptionsLUKS *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockOptionsLUKS(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockCreateOptionsLUKS(QCryptoBlockCreateOptionsLUKS *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockCreateOptionsLUKS(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockOpenOptions(QCryptoBlockOpenOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockOpenOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockCreateOptions(QCryptoBlockCreateOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockCreateOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockInfoBase(QCryptoBlockInfoBase *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockInfoBase(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockInfoLUKSSlot(QCryptoBlockInfoLUKSSlot *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockInfoLUKSSlot(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockInfoLUKSSlotList(QCryptoBlockInfoLUKSSlotList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockInfoLUKSSlotList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockInfoLUKS(QCryptoBlockInfoLUKS *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockInfoLUKS(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockInfoQCow(QCryptoBlockInfoQCow *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockInfoQCow(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_QCryptoBlockInfo(QCryptoBlockInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_QCryptoBlockInfo(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_crypto_c;

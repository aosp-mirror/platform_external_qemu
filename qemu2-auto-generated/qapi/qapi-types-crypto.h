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

#ifndef QAPI_TYPES_CRYPTO_H
#define QAPI_TYPES_CRYPTO_H

#include "qapi/qapi-builtin-types.h"

typedef enum QCryptoTLSCredsEndpoint {
    QCRYPTO_TLS_CREDS_ENDPOINT_CLIENT = 0,
    QCRYPTO_TLS_CREDS_ENDPOINT_SERVER = 1,
    QCRYPTO_TLS_CREDS_ENDPOINT__MAX = 2,
} QCryptoTLSCredsEndpoint;

#define QCryptoTLSCredsEndpoint_str(val) \
    qapi_enum_lookup(&QCryptoTLSCredsEndpoint_lookup, (val))

extern const QEnumLookup QCryptoTLSCredsEndpoint_lookup;

typedef enum QCryptoSecretFormat {
    QCRYPTO_SECRET_FORMAT_RAW = 0,
    QCRYPTO_SECRET_FORMAT_BASE64 = 1,
    QCRYPTO_SECRET_FORMAT__MAX = 2,
} QCryptoSecretFormat;

#define QCryptoSecretFormat_str(val) \
    qapi_enum_lookup(&QCryptoSecretFormat_lookup, (val))

extern const QEnumLookup QCryptoSecretFormat_lookup;

typedef enum QCryptoHashAlgorithm {
    QCRYPTO_HASH_ALG_MD5 = 0,
    QCRYPTO_HASH_ALG_SHA1 = 1,
    QCRYPTO_HASH_ALG_SHA224 = 2,
    QCRYPTO_HASH_ALG_SHA256 = 3,
    QCRYPTO_HASH_ALG_SHA384 = 4,
    QCRYPTO_HASH_ALG_SHA512 = 5,
    QCRYPTO_HASH_ALG_RIPEMD160 = 6,
    QCRYPTO_HASH_ALG__MAX = 7,
} QCryptoHashAlgorithm;

#define QCryptoHashAlgorithm_str(val) \
    qapi_enum_lookup(&QCryptoHashAlgorithm_lookup, (val))

extern const QEnumLookup QCryptoHashAlgorithm_lookup;

typedef enum QCryptoCipherAlgorithm {
    QCRYPTO_CIPHER_ALG_AES_128 = 0,
    QCRYPTO_CIPHER_ALG_AES_192 = 1,
    QCRYPTO_CIPHER_ALG_AES_256 = 2,
    QCRYPTO_CIPHER_ALG_DES_RFB = 3,
    QCRYPTO_CIPHER_ALG_3DES = 4,
    QCRYPTO_CIPHER_ALG_CAST5_128 = 5,
    QCRYPTO_CIPHER_ALG_SERPENT_128 = 6,
    QCRYPTO_CIPHER_ALG_SERPENT_192 = 7,
    QCRYPTO_CIPHER_ALG_SERPENT_256 = 8,
    QCRYPTO_CIPHER_ALG_TWOFISH_128 = 9,
    QCRYPTO_CIPHER_ALG_TWOFISH_192 = 10,
    QCRYPTO_CIPHER_ALG_TWOFISH_256 = 11,
    QCRYPTO_CIPHER_ALG__MAX = 12,
} QCryptoCipherAlgorithm;

#define QCryptoCipherAlgorithm_str(val) \
    qapi_enum_lookup(&QCryptoCipherAlgorithm_lookup, (val))

extern const QEnumLookup QCryptoCipherAlgorithm_lookup;

typedef enum QCryptoCipherMode {
    QCRYPTO_CIPHER_MODE_ECB = 0,
    QCRYPTO_CIPHER_MODE_CBC = 1,
    QCRYPTO_CIPHER_MODE_XTS = 2,
    QCRYPTO_CIPHER_MODE_CTR = 3,
    QCRYPTO_CIPHER_MODE__MAX = 4,
} QCryptoCipherMode;

#define QCryptoCipherMode_str(val) \
    qapi_enum_lookup(&QCryptoCipherMode_lookup, (val))

extern const QEnumLookup QCryptoCipherMode_lookup;

typedef enum QCryptoIVGenAlgorithm {
    QCRYPTO_IVGEN_ALG_PLAIN = 0,
    QCRYPTO_IVGEN_ALG_PLAIN64 = 1,
    QCRYPTO_IVGEN_ALG_ESSIV = 2,
    QCRYPTO_IVGEN_ALG__MAX = 3,
} QCryptoIVGenAlgorithm;

#define QCryptoIVGenAlgorithm_str(val) \
    qapi_enum_lookup(&QCryptoIVGenAlgorithm_lookup, (val))

extern const QEnumLookup QCryptoIVGenAlgorithm_lookup;

typedef enum QCryptoBlockFormat {
    Q_CRYPTO_BLOCK_FORMAT_QCOW = 0,
    Q_CRYPTO_BLOCK_FORMAT_LUKS = 1,
    Q_CRYPTO_BLOCK_FORMAT__MAX = 2,
} QCryptoBlockFormat;

#define QCryptoBlockFormat_str(val) \
    qapi_enum_lookup(&QCryptoBlockFormat_lookup, (val))

extern const QEnumLookup QCryptoBlockFormat_lookup;

typedef struct QCryptoBlockOptionsBase QCryptoBlockOptionsBase;

typedef struct QCryptoBlockOptionsQCow QCryptoBlockOptionsQCow;

typedef struct QCryptoBlockOptionsLUKS QCryptoBlockOptionsLUKS;

typedef struct QCryptoBlockCreateOptionsLUKS QCryptoBlockCreateOptionsLUKS;

typedef struct QCryptoBlockOpenOptions QCryptoBlockOpenOptions;

typedef struct QCryptoBlockCreateOptions QCryptoBlockCreateOptions;

typedef struct QCryptoBlockInfoBase QCryptoBlockInfoBase;

typedef struct QCryptoBlockInfoLUKSSlot QCryptoBlockInfoLUKSSlot;

typedef struct QCryptoBlockInfoLUKSSlotList QCryptoBlockInfoLUKSSlotList;

typedef struct QCryptoBlockInfoLUKS QCryptoBlockInfoLUKS;

typedef struct QCryptoBlockInfoQCow QCryptoBlockInfoQCow;

typedef struct QCryptoBlockInfo QCryptoBlockInfo;

struct QCryptoBlockOptionsBase {
    QCryptoBlockFormat format;
};

void qapi_free_QCryptoBlockOptionsBase(QCryptoBlockOptionsBase *obj);

struct QCryptoBlockOptionsQCow {
    bool has_key_secret;
    char *key_secret;
};

void qapi_free_QCryptoBlockOptionsQCow(QCryptoBlockOptionsQCow *obj);

struct QCryptoBlockOptionsLUKS {
    bool has_key_secret;
    char *key_secret;
};

void qapi_free_QCryptoBlockOptionsLUKS(QCryptoBlockOptionsLUKS *obj);

struct QCryptoBlockCreateOptionsLUKS {
    /* Members inherited from QCryptoBlockOptionsLUKS: */
    bool has_key_secret;
    char *key_secret;
    /* Own members: */
    bool has_cipher_alg;
    QCryptoCipherAlgorithm cipher_alg;
    bool has_cipher_mode;
    QCryptoCipherMode cipher_mode;
    bool has_ivgen_alg;
    QCryptoIVGenAlgorithm ivgen_alg;
    bool has_ivgen_hash_alg;
    QCryptoHashAlgorithm ivgen_hash_alg;
    bool has_hash_alg;
    QCryptoHashAlgorithm hash_alg;
    bool has_iter_time;
    int64_t iter_time;
};

static inline QCryptoBlockOptionsLUKS *qapi_QCryptoBlockCreateOptionsLUKS_base(const QCryptoBlockCreateOptionsLUKS *obj)
{
    return (QCryptoBlockOptionsLUKS *)obj;
}

void qapi_free_QCryptoBlockCreateOptionsLUKS(QCryptoBlockCreateOptionsLUKS *obj);

struct QCryptoBlockOpenOptions {
    /* Members inherited from QCryptoBlockOptionsBase: */
    QCryptoBlockFormat format;
    /* Own members: */
    union { /* union tag is @format */
        QCryptoBlockOptionsQCow qcow;
        QCryptoBlockOptionsLUKS luks;
    } u;
};

static inline QCryptoBlockOptionsBase *qapi_QCryptoBlockOpenOptions_base(const QCryptoBlockOpenOptions *obj)
{
    return (QCryptoBlockOptionsBase *)obj;
}

void qapi_free_QCryptoBlockOpenOptions(QCryptoBlockOpenOptions *obj);

struct QCryptoBlockCreateOptions {
    /* Members inherited from QCryptoBlockOptionsBase: */
    QCryptoBlockFormat format;
    /* Own members: */
    union { /* union tag is @format */
        QCryptoBlockOptionsQCow qcow;
        QCryptoBlockCreateOptionsLUKS luks;
    } u;
};

static inline QCryptoBlockOptionsBase *qapi_QCryptoBlockCreateOptions_base(const QCryptoBlockCreateOptions *obj)
{
    return (QCryptoBlockOptionsBase *)obj;
}

void qapi_free_QCryptoBlockCreateOptions(QCryptoBlockCreateOptions *obj);

struct QCryptoBlockInfoBase {
    QCryptoBlockFormat format;
};

void qapi_free_QCryptoBlockInfoBase(QCryptoBlockInfoBase *obj);

struct QCryptoBlockInfoLUKSSlot {
    bool active;
    bool has_iters;
    int64_t iters;
    bool has_stripes;
    int64_t stripes;
    int64_t key_offset;
};

void qapi_free_QCryptoBlockInfoLUKSSlot(QCryptoBlockInfoLUKSSlot *obj);

struct QCryptoBlockInfoLUKSSlotList {
    QCryptoBlockInfoLUKSSlotList *next;
    QCryptoBlockInfoLUKSSlot *value;
};

void qapi_free_QCryptoBlockInfoLUKSSlotList(QCryptoBlockInfoLUKSSlotList *obj);

struct QCryptoBlockInfoLUKS {
    QCryptoCipherAlgorithm cipher_alg;
    QCryptoCipherMode cipher_mode;
    QCryptoIVGenAlgorithm ivgen_alg;
    bool has_ivgen_hash_alg;
    QCryptoHashAlgorithm ivgen_hash_alg;
    QCryptoHashAlgorithm hash_alg;
    int64_t payload_offset;
    int64_t master_key_iters;
    char *uuid;
    QCryptoBlockInfoLUKSSlotList *slots;
};

void qapi_free_QCryptoBlockInfoLUKS(QCryptoBlockInfoLUKS *obj);

struct QCryptoBlockInfoQCow {
    char qapi_dummy_for_empty_struct;
};

void qapi_free_QCryptoBlockInfoQCow(QCryptoBlockInfoQCow *obj);

struct QCryptoBlockInfo {
    /* Members inherited from QCryptoBlockInfoBase: */
    QCryptoBlockFormat format;
    /* Own members: */
    union { /* union tag is @format */
        QCryptoBlockInfoQCow qcow;
        QCryptoBlockInfoLUKS luks;
    } u;
};

static inline QCryptoBlockInfoBase *qapi_QCryptoBlockInfo_base(const QCryptoBlockInfo *obj)
{
    return (QCryptoBlockInfoBase *)obj;
}

void qapi_free_QCryptoBlockInfo(QCryptoBlockInfo *obj);

#endif /* QAPI_TYPES_CRYPTO_H */

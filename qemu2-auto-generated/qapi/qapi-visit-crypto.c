/* AUTOMATICALLY GENERATED, DO NOT MODIFY */

/*
 * Schema-defined QAPI visitors
 *
 * Copyright IBM, Corp. 2011
 * Copyright (C) 2014-2018 Red Hat, Inc.
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.1 or later.
 * See the COPYING.LIB file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qapi-visit-crypto.h"

void visit_type_QCryptoTLSCredsEndpoint(Visitor *v, const char *name, QCryptoTLSCredsEndpoint *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QCryptoTLSCredsEndpoint_lookup, errp);
    *obj = value;
}

void visit_type_QCryptoSecretFormat(Visitor *v, const char *name, QCryptoSecretFormat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QCryptoSecretFormat_lookup, errp);
    *obj = value;
}

void visit_type_QCryptoHashAlgorithm(Visitor *v, const char *name, QCryptoHashAlgorithm *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QCryptoHashAlgorithm_lookup, errp);
    *obj = value;
}

void visit_type_QCryptoCipherAlgorithm(Visitor *v, const char *name, QCryptoCipherAlgorithm *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QCryptoCipherAlgorithm_lookup, errp);
    *obj = value;
}

void visit_type_QCryptoCipherMode(Visitor *v, const char *name, QCryptoCipherMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QCryptoCipherMode_lookup, errp);
    *obj = value;
}

void visit_type_QCryptoIVGenAlgorithm(Visitor *v, const char *name, QCryptoIVGenAlgorithm *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QCryptoIVGenAlgorithm_lookup, errp);
    *obj = value;
}

void visit_type_QCryptoBlockFormat(Visitor *v, const char *name, QCryptoBlockFormat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QCryptoBlockFormat_lookup, errp);
    *obj = value;
}

void visit_type_QCryptoBlockOptionsBase_members(Visitor *v, QCryptoBlockOptionsBase *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockFormat(v, "format", &obj->format, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockOptionsBase(Visitor *v, const char *name, QCryptoBlockOptionsBase **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockOptionsBase), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockOptionsBase_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockOptionsBase(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockOptionsQCow_members(Visitor *v, QCryptoBlockOptionsQCow *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "key-secret", &obj->has_key_secret)) {
        visit_type_str(v, "key-secret", &obj->key_secret, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockOptionsQCow(Visitor *v, const char *name, QCryptoBlockOptionsQCow **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockOptionsQCow), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockOptionsQCow_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockOptionsQCow(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockOptionsLUKS_members(Visitor *v, QCryptoBlockOptionsLUKS *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "key-secret", &obj->has_key_secret)) {
        visit_type_str(v, "key-secret", &obj->key_secret, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockOptionsLUKS(Visitor *v, const char *name, QCryptoBlockOptionsLUKS **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockOptionsLUKS), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockOptionsLUKS_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockOptionsLUKS(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockCreateOptionsLUKS_members(Visitor *v, QCryptoBlockCreateOptionsLUKS *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockOptionsLUKS_members(v, (QCryptoBlockOptionsLUKS *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "cipher-alg", &obj->has_cipher_alg)) {
        visit_type_QCryptoCipherAlgorithm(v, "cipher-alg", &obj->cipher_alg, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cipher-mode", &obj->has_cipher_mode)) {
        visit_type_QCryptoCipherMode(v, "cipher-mode", &obj->cipher_mode, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ivgen-alg", &obj->has_ivgen_alg)) {
        visit_type_QCryptoIVGenAlgorithm(v, "ivgen-alg", &obj->ivgen_alg, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ivgen-hash-alg", &obj->has_ivgen_hash_alg)) {
        visit_type_QCryptoHashAlgorithm(v, "ivgen-hash-alg", &obj->ivgen_hash_alg, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "hash-alg", &obj->has_hash_alg)) {
        visit_type_QCryptoHashAlgorithm(v, "hash-alg", &obj->hash_alg, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "iter-time", &obj->has_iter_time)) {
        visit_type_int(v, "iter-time", &obj->iter_time, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockCreateOptionsLUKS(Visitor *v, const char *name, QCryptoBlockCreateOptionsLUKS **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockCreateOptionsLUKS), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockCreateOptionsLUKS_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockCreateOptionsLUKS(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockOpenOptions_members(Visitor *v, QCryptoBlockOpenOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockOptionsBase_members(v, (QCryptoBlockOptionsBase *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->format) {
    case Q_CRYPTO_BLOCK_FORMAT_QCOW:
        visit_type_QCryptoBlockOptionsQCow_members(v, &obj->u.qcow, &err);
        break;
    case Q_CRYPTO_BLOCK_FORMAT_LUKS:
        visit_type_QCryptoBlockOptionsLUKS_members(v, &obj->u.luks, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockOpenOptions(Visitor *v, const char *name, QCryptoBlockOpenOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockOpenOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockOpenOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockOpenOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockCreateOptions_members(Visitor *v, QCryptoBlockCreateOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockOptionsBase_members(v, (QCryptoBlockOptionsBase *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->format) {
    case Q_CRYPTO_BLOCK_FORMAT_QCOW:
        visit_type_QCryptoBlockOptionsQCow_members(v, &obj->u.qcow, &err);
        break;
    case Q_CRYPTO_BLOCK_FORMAT_LUKS:
        visit_type_QCryptoBlockCreateOptionsLUKS_members(v, &obj->u.luks, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockCreateOptions(Visitor *v, const char *name, QCryptoBlockCreateOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockCreateOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockCreateOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockCreateOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoBase_members(Visitor *v, QCryptoBlockInfoBase *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockFormat(v, "format", &obj->format, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoBase(Visitor *v, const char *name, QCryptoBlockInfoBase **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockInfoBase), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockInfoBase_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockInfoBase(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoLUKSSlot_members(Visitor *v, QCryptoBlockInfoLUKSSlot *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "active", &obj->active, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "iters", &obj->has_iters)) {
        visit_type_int(v, "iters", &obj->iters, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "stripes", &obj->has_stripes)) {
        visit_type_int(v, "stripes", &obj->stripes, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_int(v, "key-offset", &obj->key_offset, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoLUKSSlot(Visitor *v, const char *name, QCryptoBlockInfoLUKSSlot **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockInfoLUKSSlot), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockInfoLUKSSlot_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockInfoLUKSSlot(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoLUKSSlotList(Visitor *v, const char *name, QCryptoBlockInfoLUKSSlotList **obj, Error **errp)
{
    Error *err = NULL;
    QCryptoBlockInfoLUKSSlotList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (QCryptoBlockInfoLUKSSlotList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_QCryptoBlockInfoLUKSSlot(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockInfoLUKSSlotList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoLUKS_members(Visitor *v, QCryptoBlockInfoLUKS *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoCipherAlgorithm(v, "cipher-alg", &obj->cipher_alg, &err);
    if (err) {
        goto out;
    }
    visit_type_QCryptoCipherMode(v, "cipher-mode", &obj->cipher_mode, &err);
    if (err) {
        goto out;
    }
    visit_type_QCryptoIVGenAlgorithm(v, "ivgen-alg", &obj->ivgen_alg, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "ivgen-hash-alg", &obj->has_ivgen_hash_alg)) {
        visit_type_QCryptoHashAlgorithm(v, "ivgen-hash-alg", &obj->ivgen_hash_alg, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_QCryptoHashAlgorithm(v, "hash-alg", &obj->hash_alg, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "payload-offset", &obj->payload_offset, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "master-key-iters", &obj->master_key_iters, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "uuid", &obj->uuid, &err);
    if (err) {
        goto out;
    }
    visit_type_QCryptoBlockInfoLUKSSlotList(v, "slots", &obj->slots, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoLUKS(Visitor *v, const char *name, QCryptoBlockInfoLUKS **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockInfoLUKS), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockInfoLUKS_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockInfoLUKS(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoQCow_members(Visitor *v, QCryptoBlockInfoQCow *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfoQCow(Visitor *v, const char *name, QCryptoBlockInfoQCow **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockInfoQCow), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockInfoQCow_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockInfoQCow(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfo_members(Visitor *v, QCryptoBlockInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QCryptoBlockInfoBase_members(v, (QCryptoBlockInfoBase *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->format) {
    case Q_CRYPTO_BLOCK_FORMAT_QCOW:
        visit_type_QCryptoBlockInfoQCow_members(v, &obj->u.qcow, &err);
        break;
    case Q_CRYPTO_BLOCK_FORMAT_LUKS:
        visit_type_QCryptoBlockInfoLUKS_members(v, &obj->u.luks, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_QCryptoBlockInfo(Visitor *v, const char *name, QCryptoBlockInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(QCryptoBlockInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_QCryptoBlockInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_QCryptoBlockInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_crypto_c;

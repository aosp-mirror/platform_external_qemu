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

#ifndef QAPI_VISIT_CRYPTO_H
#define QAPI_VISIT_CRYPTO_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-crypto.h"

void visit_type_QCryptoTLSCredsEndpoint(Visitor *v, const char *name, QCryptoTLSCredsEndpoint *obj, Error **errp);
void visit_type_QCryptoSecretFormat(Visitor *v, const char *name, QCryptoSecretFormat *obj, Error **errp);
void visit_type_QCryptoHashAlgorithm(Visitor *v, const char *name, QCryptoHashAlgorithm *obj, Error **errp);
void visit_type_QCryptoCipherAlgorithm(Visitor *v, const char *name, QCryptoCipherAlgorithm *obj, Error **errp);
void visit_type_QCryptoCipherMode(Visitor *v, const char *name, QCryptoCipherMode *obj, Error **errp);
void visit_type_QCryptoIVGenAlgorithm(Visitor *v, const char *name, QCryptoIVGenAlgorithm *obj, Error **errp);
void visit_type_QCryptoBlockFormat(Visitor *v, const char *name, QCryptoBlockFormat *obj, Error **errp);

void visit_type_QCryptoBlockOptionsBase_members(Visitor *v, QCryptoBlockOptionsBase *obj, Error **errp);
void visit_type_QCryptoBlockOptionsBase(Visitor *v, const char *name, QCryptoBlockOptionsBase **obj, Error **errp);

void visit_type_QCryptoBlockOptionsQCow_members(Visitor *v, QCryptoBlockOptionsQCow *obj, Error **errp);
void visit_type_QCryptoBlockOptionsQCow(Visitor *v, const char *name, QCryptoBlockOptionsQCow **obj, Error **errp);

void visit_type_QCryptoBlockOptionsLUKS_members(Visitor *v, QCryptoBlockOptionsLUKS *obj, Error **errp);
void visit_type_QCryptoBlockOptionsLUKS(Visitor *v, const char *name, QCryptoBlockOptionsLUKS **obj, Error **errp);

void visit_type_QCryptoBlockCreateOptionsLUKS_members(Visitor *v, QCryptoBlockCreateOptionsLUKS *obj, Error **errp);
void visit_type_QCryptoBlockCreateOptionsLUKS(Visitor *v, const char *name, QCryptoBlockCreateOptionsLUKS **obj, Error **errp);

void visit_type_QCryptoBlockOpenOptions_members(Visitor *v, QCryptoBlockOpenOptions *obj, Error **errp);
void visit_type_QCryptoBlockOpenOptions(Visitor *v, const char *name, QCryptoBlockOpenOptions **obj, Error **errp);

void visit_type_QCryptoBlockCreateOptions_members(Visitor *v, QCryptoBlockCreateOptions *obj, Error **errp);
void visit_type_QCryptoBlockCreateOptions(Visitor *v, const char *name, QCryptoBlockCreateOptions **obj, Error **errp);

void visit_type_QCryptoBlockInfoBase_members(Visitor *v, QCryptoBlockInfoBase *obj, Error **errp);
void visit_type_QCryptoBlockInfoBase(Visitor *v, const char *name, QCryptoBlockInfoBase **obj, Error **errp);

void visit_type_QCryptoBlockInfoLUKSSlot_members(Visitor *v, QCryptoBlockInfoLUKSSlot *obj, Error **errp);
void visit_type_QCryptoBlockInfoLUKSSlot(Visitor *v, const char *name, QCryptoBlockInfoLUKSSlot **obj, Error **errp);
void visit_type_QCryptoBlockInfoLUKSSlotList(Visitor *v, const char *name, QCryptoBlockInfoLUKSSlotList **obj, Error **errp);

void visit_type_QCryptoBlockInfoLUKS_members(Visitor *v, QCryptoBlockInfoLUKS *obj, Error **errp);
void visit_type_QCryptoBlockInfoLUKS(Visitor *v, const char *name, QCryptoBlockInfoLUKS **obj, Error **errp);

void visit_type_QCryptoBlockInfoQCow_members(Visitor *v, QCryptoBlockInfoQCow *obj, Error **errp);
void visit_type_QCryptoBlockInfoQCow(Visitor *v, const char *name, QCryptoBlockInfoQCow **obj, Error **errp);

void visit_type_QCryptoBlockInfo_members(Visitor *v, QCryptoBlockInfo *obj, Error **errp);
void visit_type_QCryptoBlockInfo(Visitor *v, const char *name, QCryptoBlockInfo **obj, Error **errp);

#endif /* QAPI_VISIT_CRYPTO_H */

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

#ifndef QAPI_VISIT_TPM_H
#define QAPI_VISIT_TPM_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-tpm.h"

void visit_type_TpmModel(Visitor *v, const char *name, TpmModel *obj, Error **errp);
void visit_type_TpmModelList(Visitor *v, const char *name, TpmModelList **obj, Error **errp);
void visit_type_TpmType(Visitor *v, const char *name, TpmType *obj, Error **errp);
void visit_type_TpmTypeList(Visitor *v, const char *name, TpmTypeList **obj, Error **errp);

void visit_type_TPMPassthroughOptions_members(Visitor *v, TPMPassthroughOptions *obj, Error **errp);
void visit_type_TPMPassthroughOptions(Visitor *v, const char *name, TPMPassthroughOptions **obj, Error **errp);

void visit_type_TPMEmulatorOptions_members(Visitor *v, TPMEmulatorOptions *obj, Error **errp);
void visit_type_TPMEmulatorOptions(Visitor *v, const char *name, TPMEmulatorOptions **obj, Error **errp);

void visit_type_q_obj_TPMPassthroughOptions_wrapper_members(Visitor *v, q_obj_TPMPassthroughOptions_wrapper *obj, Error **errp);

void visit_type_q_obj_TPMEmulatorOptions_wrapper_members(Visitor *v, q_obj_TPMEmulatorOptions_wrapper *obj, Error **errp);
void visit_type_TpmTypeOptionsKind(Visitor *v, const char *name, TpmTypeOptionsKind *obj, Error **errp);

void visit_type_TpmTypeOptions_members(Visitor *v, TpmTypeOptions *obj, Error **errp);
void visit_type_TpmTypeOptions(Visitor *v, const char *name, TpmTypeOptions **obj, Error **errp);

void visit_type_TPMInfo_members(Visitor *v, TPMInfo *obj, Error **errp);
void visit_type_TPMInfo(Visitor *v, const char *name, TPMInfo **obj, Error **errp);
void visit_type_TPMInfoList(Visitor *v, const char *name, TPMInfoList **obj, Error **errp);

#endif /* QAPI_VISIT_TPM_H */

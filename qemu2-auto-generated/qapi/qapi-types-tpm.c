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
#include "qapi-types-tpm.h"
#include "qapi-visit-tpm.h"

const QEnumLookup TpmModel_lookup = {
    .array = (const char *const[]) {
        [TPM_MODEL_TPM_TIS] = "tpm-tis",
        [TPM_MODEL_TPM_CRB] = "tpm-crb",
    },
    .size = TPM_MODEL__MAX
};

void qapi_free_TpmModelList(TpmModelList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TpmModelList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup TpmType_lookup = {
    .array = (const char *const[]) {
        [TPM_TYPE_PASSTHROUGH] = "passthrough",
        [TPM_TYPE_EMULATOR] = "emulator",
    },
    .size = TPM_TYPE__MAX
};

void qapi_free_TpmTypeList(TpmTypeList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TpmTypeList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_TPMPassthroughOptions(TPMPassthroughOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TPMPassthroughOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_TPMEmulatorOptions(TPMEmulatorOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TPMEmulatorOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup TpmTypeOptionsKind_lookup = {
    .array = (const char *const[]) {
        [TPM_TYPE_OPTIONS_KIND_PASSTHROUGH] = "passthrough",
        [TPM_TYPE_OPTIONS_KIND_EMULATOR] = "emulator",
    },
    .size = TPM_TYPE_OPTIONS_KIND__MAX
};

void qapi_free_TpmTypeOptions(TpmTypeOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TpmTypeOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_TPMInfo(TPMInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TPMInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_TPMInfoList(TPMInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TPMInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_tpm_c;

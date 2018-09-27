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
#include "qapi-visit-tpm.h"

void visit_type_TpmModel(Visitor *v, const char *name, TpmModel *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &TpmModel_lookup, errp);
    *obj = value;
}

void visit_type_TpmModelList(Visitor *v, const char *name, TpmModelList **obj, Error **errp)
{
    Error *err = NULL;
    TpmModelList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (TpmModelList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_TpmModel(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TpmModelList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TpmType(Visitor *v, const char *name, TpmType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &TpmType_lookup, errp);
    *obj = value;
}

void visit_type_TpmTypeList(Visitor *v, const char *name, TpmTypeList **obj, Error **errp)
{
    Error *err = NULL;
    TpmTypeList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (TpmTypeList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_TpmType(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TpmTypeList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TPMPassthroughOptions_members(Visitor *v, TPMPassthroughOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "path", &obj->has_path)) {
        visit_type_str(v, "path", &obj->path, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cancel-path", &obj->has_cancel_path)) {
        visit_type_str(v, "cancel-path", &obj->cancel_path, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_TPMPassthroughOptions(Visitor *v, const char *name, TPMPassthroughOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TPMPassthroughOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TPMPassthroughOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TPMPassthroughOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TPMEmulatorOptions_members(Visitor *v, TPMEmulatorOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "chardev", &obj->chardev, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_TPMEmulatorOptions(Visitor *v, const char *name, TPMEmulatorOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TPMEmulatorOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TPMEmulatorOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TPMEmulatorOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_TPMPassthroughOptions_wrapper_members(Visitor *v, q_obj_TPMPassthroughOptions_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_TPMPassthroughOptions(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_TPMEmulatorOptions_wrapper_members(Visitor *v, q_obj_TPMEmulatorOptions_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_TPMEmulatorOptions(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_TpmTypeOptionsKind(Visitor *v, const char *name, TpmTypeOptionsKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &TpmTypeOptionsKind_lookup, errp);
    *obj = value;
}

void visit_type_TpmTypeOptions_members(Visitor *v, TpmTypeOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_TpmTypeOptionsKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case TPM_TYPE_OPTIONS_KIND_PASSTHROUGH:
        visit_type_q_obj_TPMPassthroughOptions_wrapper_members(v, &obj->u.passthrough, &err);
        break;
    case TPM_TYPE_OPTIONS_KIND_EMULATOR:
        visit_type_q_obj_TPMEmulatorOptions_wrapper_members(v, &obj->u.emulator, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_TpmTypeOptions(Visitor *v, const char *name, TpmTypeOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TpmTypeOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TpmTypeOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TpmTypeOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TPMInfo_members(Visitor *v, TPMInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_TpmModel(v, "model", &obj->model, &err);
    if (err) {
        goto out;
    }
    visit_type_TpmTypeOptions(v, "options", &obj->options, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_TPMInfo(Visitor *v, const char *name, TPMInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TPMInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TPMInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TPMInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TPMInfoList(Visitor *v, const char *name, TPMInfoList **obj, Error **errp)
{
    Error *err = NULL;
    TPMInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (TPMInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_TPMInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TPMInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_tpm_c;

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
#include "qapi-visit-common.h"

void visit_type_QapiErrorClass(Visitor *v, const char *name, QapiErrorClass *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QapiErrorClass_lookup, errp);
    *obj = value;
}

void visit_type_IoOperationType(Visitor *v, const char *name, IoOperationType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &IoOperationType_lookup, errp);
    *obj = value;
}

void visit_type_OnOffAuto(Visitor *v, const char *name, OnOffAuto *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &OnOffAuto_lookup, errp);
    *obj = value;
}

void visit_type_OnOffSplit(Visitor *v, const char *name, OnOffSplit *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &OnOffSplit_lookup, errp);
    *obj = value;
}

void visit_type_String_members(Visitor *v, String *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "str", &obj->str, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_String(Visitor *v, const char *name, String **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(String), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_String_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_String(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_StrOrNull(Visitor *v, const char *name, StrOrNull **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_alternate(v, name, (GenericAlternate **)obj, sizeof(**obj),
                          &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    switch ((*obj)->type) {
    case QTYPE_QSTRING:
        visit_type_str(v, name, &(*obj)->u.s, &err);
        break;
    case QTYPE_QNULL:
        visit_type_null(v, name, &(*obj)->u.n, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "StrOrNull");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_StrOrNull(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_OffAutoPCIBAR(Visitor *v, const char *name, OffAutoPCIBAR *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &OffAutoPCIBAR_lookup, errp);
    *obj = value;
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_common_c;

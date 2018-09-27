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
#include "qapi-types-introspect.h"
#include "qapi-visit-introspect.h"

void qapi_free_SchemaInfoList(SchemaInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup SchemaMetaType_lookup = {
    .array = (const char *const[]) {
        [SCHEMA_META_TYPE_BUILTIN] = "builtin",
        [SCHEMA_META_TYPE_ENUM] = "enum",
        [SCHEMA_META_TYPE_ARRAY] = "array",
        [SCHEMA_META_TYPE_OBJECT] = "object",
        [SCHEMA_META_TYPE_ALTERNATE] = "alternate",
        [SCHEMA_META_TYPE_COMMAND] = "command",
        [SCHEMA_META_TYPE_EVENT] = "event",
    },
    .size = SCHEMA_META_TYPE__MAX
};

void qapi_free_SchemaInfo(SchemaInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoBuiltin(SchemaInfoBuiltin *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoBuiltin(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup JSONType_lookup = {
    .array = (const char *const[]) {
        [JSON_TYPE_STRING] = "string",
        [JSON_TYPE_NUMBER] = "number",
        [JSON_TYPE_INT] = "int",
        [JSON_TYPE_BOOLEAN] = "boolean",
        [JSON_TYPE_NULL] = "null",
        [JSON_TYPE_OBJECT] = "object",
        [JSON_TYPE_ARRAY] = "array",
        [JSON_TYPE_VALUE] = "value",
    },
    .size = JSON_TYPE__MAX
};

void qapi_free_SchemaInfoEnum(SchemaInfoEnum *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoEnum(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoArray(SchemaInfoArray *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoArray(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoObjectMemberList(SchemaInfoObjectMemberList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoObjectMemberList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoObjectVariantList(SchemaInfoObjectVariantList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoObjectVariantList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoObject(SchemaInfoObject *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoObject(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoObjectMember(SchemaInfoObjectMember *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoObjectMember(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoObjectVariant(SchemaInfoObjectVariant *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoObjectVariant(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoAlternateMemberList(SchemaInfoAlternateMemberList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoAlternateMemberList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoAlternate(SchemaInfoAlternate *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoAlternate(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoAlternateMember(SchemaInfoAlternateMember *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoAlternateMember(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoCommand(SchemaInfoCommand *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoCommand(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_SchemaInfoEvent(SchemaInfoEvent *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SchemaInfoEvent(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_introspect_c;

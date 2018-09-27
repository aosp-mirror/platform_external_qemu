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
#include "qapi-visit-introspect.h"

void visit_type_SchemaInfoList(Visitor *v, const char *name, SchemaInfoList **obj, Error **errp)
{
    Error *err = NULL;
    SchemaInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (SchemaInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_SchemaInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaMetaType(Visitor *v, const char *name, SchemaMetaType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SchemaMetaType_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_SchemaInfo_base_members(Visitor *v, q_obj_SchemaInfo_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_SchemaMetaType(v, "meta-type", &obj->meta_type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfo_members(Visitor *v, SchemaInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_SchemaInfo_base_members(v, (q_obj_SchemaInfo_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->meta_type) {
    case SCHEMA_META_TYPE_BUILTIN:
        visit_type_SchemaInfoBuiltin_members(v, &obj->u.builtin, &err);
        break;
    case SCHEMA_META_TYPE_ENUM:
        visit_type_SchemaInfoEnum_members(v, &obj->u.q_enum, &err);
        break;
    case SCHEMA_META_TYPE_ARRAY:
        visit_type_SchemaInfoArray_members(v, &obj->u.array, &err);
        break;
    case SCHEMA_META_TYPE_OBJECT:
        visit_type_SchemaInfoObject_members(v, &obj->u.object, &err);
        break;
    case SCHEMA_META_TYPE_ALTERNATE:
        visit_type_SchemaInfoAlternate_members(v, &obj->u.alternate, &err);
        break;
    case SCHEMA_META_TYPE_COMMAND:
        visit_type_SchemaInfoCommand_members(v, &obj->u.command, &err);
        break;
    case SCHEMA_META_TYPE_EVENT:
        visit_type_SchemaInfoEvent_members(v, &obj->u.event, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfo(Visitor *v, const char *name, SchemaInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoBuiltin_members(Visitor *v, SchemaInfoBuiltin *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_JSONType(v, "json-type", &obj->json_type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoBuiltin(Visitor *v, const char *name, SchemaInfoBuiltin **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoBuiltin), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoBuiltin_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoBuiltin(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_JSONType(Visitor *v, const char *name, JSONType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &JSONType_lookup, errp);
    *obj = value;
}

void visit_type_SchemaInfoEnum_members(Visitor *v, SchemaInfoEnum *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_strList(v, "values", &obj->values, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoEnum(Visitor *v, const char *name, SchemaInfoEnum **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoEnum), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoEnum_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoEnum(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoArray_members(Visitor *v, SchemaInfoArray *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "element-type", &obj->element_type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoArray(Visitor *v, const char *name, SchemaInfoArray **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoArray), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoArray_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoArray(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObjectMemberList(Visitor *v, const char *name, SchemaInfoObjectMemberList **obj, Error **errp)
{
    Error *err = NULL;
    SchemaInfoObjectMemberList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (SchemaInfoObjectMemberList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_SchemaInfoObjectMember(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoObjectMemberList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObjectVariantList(Visitor *v, const char *name, SchemaInfoObjectVariantList **obj, Error **errp)
{
    Error *err = NULL;
    SchemaInfoObjectVariantList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (SchemaInfoObjectVariantList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_SchemaInfoObjectVariant(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoObjectVariantList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObject_members(Visitor *v, SchemaInfoObject *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SchemaInfoObjectMemberList(v, "members", &obj->members, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "tag", &obj->has_tag)) {
        visit_type_str(v, "tag", &obj->tag, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "variants", &obj->has_variants)) {
        visit_type_SchemaInfoObjectVariantList(v, "variants", &obj->variants, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObject(Visitor *v, const char *name, SchemaInfoObject **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoObject), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoObject_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoObject(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObjectMember_members(Visitor *v, SchemaInfoObjectMember *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "default", &obj->has_q_default)) {
        visit_type_any(v, "default", &obj->q_default, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObjectMember(Visitor *v, const char *name, SchemaInfoObjectMember **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoObjectMember), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoObjectMember_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoObjectMember(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObjectVariant_members(Visitor *v, SchemaInfoObjectVariant *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "case", &obj->q_case, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoObjectVariant(Visitor *v, const char *name, SchemaInfoObjectVariant **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoObjectVariant), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoObjectVariant_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoObjectVariant(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoAlternateMemberList(Visitor *v, const char *name, SchemaInfoAlternateMemberList **obj, Error **errp)
{
    Error *err = NULL;
    SchemaInfoAlternateMemberList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (SchemaInfoAlternateMemberList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_SchemaInfoAlternateMember(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoAlternateMemberList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoAlternate_members(Visitor *v, SchemaInfoAlternate *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SchemaInfoAlternateMemberList(v, "members", &obj->members, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoAlternate(Visitor *v, const char *name, SchemaInfoAlternate **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoAlternate), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoAlternate_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoAlternate(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoAlternateMember_members(Visitor *v, SchemaInfoAlternateMember *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoAlternateMember(Visitor *v, const char *name, SchemaInfoAlternateMember **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoAlternateMember), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoAlternateMember_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoAlternateMember(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoCommand_members(Visitor *v, SchemaInfoCommand *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "arg-type", &obj->arg_type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "ret-type", &obj->ret_type, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "allow-oob", &obj->allow_oob, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoCommand(Visitor *v, const char *name, SchemaInfoCommand **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoCommand), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoCommand_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoCommand(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoEvent_members(Visitor *v, SchemaInfoEvent *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "arg-type", &obj->arg_type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SchemaInfoEvent(Visitor *v, const char *name, SchemaInfoEvent **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SchemaInfoEvent), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SchemaInfoEvent_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SchemaInfoEvent(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_introspect_c;

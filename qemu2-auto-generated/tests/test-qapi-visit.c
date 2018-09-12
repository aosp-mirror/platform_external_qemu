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
#include "test-qapi-visit.h"

void visit_type_TestStruct_members(Visitor *v, TestStruct *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "integer", &obj->integer, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "boolean", &obj->boolean, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "string", &obj->string, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_TestStruct(Visitor *v, const char *name, TestStruct **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(TestStruct), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_TestStruct_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TestStruct(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NestedEnumsOne_members(Visitor *v, NestedEnumsOne *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_EnumOne(v, "enum1", &obj->enum1, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "enum2", &obj->has_enum2)) {
        visit_type_EnumOne(v, "enum2", &obj->enum2, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_EnumOne(v, "enum3", &obj->enum3, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "enum4", &obj->has_enum4)) {
        visit_type_EnumOne(v, "enum4", &obj->enum4, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NestedEnumsOne(Visitor *v, const char *name, NestedEnumsOne **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NestedEnumsOne), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NestedEnumsOne_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NestedEnumsOne(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_MyEnum(Visitor *v, const char *name, MyEnum *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &MyEnum_lookup, errp);
    *obj = value;
}

void visit_type_Empty1_members(Visitor *v, Empty1 *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_Empty1(Visitor *v, const char *name, Empty1 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(Empty1), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_Empty1_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_Empty1(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_Empty2_members(Visitor *v, Empty2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_Empty1_members(v, (Empty1 *)obj, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_Empty2(Visitor *v, const char *name, Empty2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(Empty2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_Empty2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_Empty2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QEnumTwo(Visitor *v, const char *name, QEnumTwo *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QEnumTwo_lookup, errp);
    *obj = value;
}

void visit_type_UserDefOne_members(Visitor *v, UserDefOne *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefZero_members(v, (UserDefZero *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "string", &obj->string, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "enum1", &obj->has_enum1)) {
        visit_type_EnumOne(v, "enum1", &obj->enum1, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefOne(Visitor *v, const char *name, UserDefOne **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefOne), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefOne_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefOne(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_EnumOne(Visitor *v, const char *name, EnumOne *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &EnumOne_lookup, errp);
    *obj = value;
}

void visit_type_UserDefZero_members(Visitor *v, UserDefZero *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "integer", &obj->integer, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefZero(Visitor *v, const char *name, UserDefZero **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefZero), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefZero_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefZero(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefTwoDictDict_members(Visitor *v, UserDefTwoDictDict *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefOne(v, "userdef", &obj->userdef, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "string", &obj->string, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefTwoDictDict(Visitor *v, const char *name, UserDefTwoDictDict **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefTwoDictDict), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefTwoDictDict_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefTwoDictDict(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefTwoDict_members(Visitor *v, UserDefTwoDict *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "string1", &obj->string1, &err);
    if (err) {
        goto out;
    }
    visit_type_UserDefTwoDictDict(v, "dict2", &obj->dict2, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "dict3", &obj->has_dict3)) {
        visit_type_UserDefTwoDictDict(v, "dict3", &obj->dict3, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefTwoDict(Visitor *v, const char *name, UserDefTwoDict **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefTwoDict), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefTwoDict_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefTwoDict(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefTwo_members(Visitor *v, UserDefTwo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "string0", &obj->string0, &err);
    if (err) {
        goto out;
    }
    visit_type_UserDefTwoDict(v, "dict1", &obj->dict1, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefTwo(Visitor *v, const char *name, UserDefTwo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefTwo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefTwo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefTwo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefOneList(Visitor *v, const char *name, UserDefOneList **obj, Error **errp)
{
    Error *err = NULL;
    UserDefOneList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (UserDefOneList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_UserDefOne(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefOneList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefTwoList(Visitor *v, const char *name, UserDefTwoList **obj, Error **errp)
{
    Error *err = NULL;
    UserDefTwoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (UserDefTwoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_UserDefTwo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefTwoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_TestStructList(Visitor *v, const char *name, TestStructList **obj, Error **errp)
{
    Error *err = NULL;
    TestStructList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (TestStructList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_TestStruct(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_TestStructList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ForceArrays_members(Visitor *v, ForceArrays *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefOneList(v, "unused1", &obj->unused1, &err);
    if (err) {
        goto out;
    }
    visit_type_UserDefTwoList(v, "unused2", &obj->unused2, &err);
    if (err) {
        goto out;
    }
    visit_type_TestStructList(v, "unused3", &obj->unused3, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ForceArrays(Visitor *v, const char *name, ForceArrays **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ForceArrays), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ForceArrays_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ForceArrays(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefA_members(Visitor *v, UserDefA *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "boolean", &obj->boolean, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "a_b", &obj->has_a_b)) {
        visit_type_int(v, "a_b", &obj->a_b, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefA(Visitor *v, const char *name, UserDefA **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefA), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefA_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefA(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefB_members(Visitor *v, UserDefB *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "intb", &obj->intb, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "a-b", &obj->has_a_b)) {
        visit_type_bool(v, "a-b", &obj->a_b, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefB(Visitor *v, const char *name, UserDefB **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefB), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefB_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefB(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefFlatUnion_members(Visitor *v, UserDefFlatUnion *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefUnionBase_members(v, (UserDefUnionBase *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->enum1) {
    case ENUM_ONE_VALUE1:
        visit_type_UserDefA_members(v, &obj->u.value1, &err);
        break;
    case ENUM_ONE_VALUE2:
        visit_type_UserDefB_members(v, &obj->u.value2, &err);
        break;
    case ENUM_ONE_VALUE3:
        visit_type_UserDefB_members(v, &obj->u.value3, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefFlatUnion(Visitor *v, const char *name, UserDefFlatUnion **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefFlatUnion), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefFlatUnion_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefFlatUnion(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefUnionBase_members(Visitor *v, UserDefUnionBase *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefZero_members(v, (UserDefZero *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "string", &obj->string, &err);
    if (err) {
        goto out;
    }
    visit_type_EnumOne(v, "enum1", &obj->enum1, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefUnionBase(Visitor *v, const char *name, UserDefUnionBase **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefUnionBase), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefUnionBase_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefUnionBase(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_UserDefFlatUnion2_base_members(Visitor *v, q_obj_UserDefFlatUnion2_base *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "integer", &obj->has_integer)) {
        visit_type_int(v, "integer", &obj->integer, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "string", &obj->string, &err);
    if (err) {
        goto out;
    }
    visit_type_QEnumTwo(v, "enum1", &obj->enum1, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefFlatUnion2_members(Visitor *v, UserDefFlatUnion2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_UserDefFlatUnion2_base_members(v, (q_obj_UserDefFlatUnion2_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->enum1) {
    case QENUM_TWO_VALUE1:
        visit_type_UserDefC_members(v, &obj->u.value1, &err);
        break;
    case QENUM_TWO_VALUE2:
        visit_type_UserDefB_members(v, &obj->u.value2, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefFlatUnion2(Visitor *v, const char *name, UserDefFlatUnion2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefFlatUnion2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefFlatUnion2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefFlatUnion2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_WrapAlternate_members(Visitor *v, WrapAlternate *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefAlternate(v, "alt", &obj->alt, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_WrapAlternate(Visitor *v, const char *name, WrapAlternate **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(WrapAlternate), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_WrapAlternate_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_WrapAlternate(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefAlternate(Visitor *v, const char *name, UserDefAlternate **obj, Error **errp)
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
    case QTYPE_QDICT:
        visit_start_struct(v, name, NULL, 0, &err);
        if (err) {
            break;
        }
        visit_type_UserDefFlatUnion_members(v, &(*obj)->u.udfu, &err);
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        break;
    case QTYPE_QSTRING:
        visit_type_EnumOne(v, name, &(*obj)->u.e, &err);
        break;
    case QTYPE_QNUM:
        visit_type_int(v, name, &(*obj)->u.i, &err);
        break;
    case QTYPE_QNULL:
        visit_type_null(v, name, &(*obj)->u.n, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "UserDefAlternate");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefAlternate(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UserDefC_members(Visitor *v, UserDefC *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "string1", &obj->string1, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "string2", &obj->string2, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefC(Visitor *v, const char *name, UserDefC **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefC), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefC_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefC(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_AltEnumBool(Visitor *v, const char *name, AltEnumBool **obj, Error **errp)
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
        visit_type_EnumOne(v, name, &(*obj)->u.e, &err);
        break;
    case QTYPE_QBOOL:
        visit_type_bool(v, name, &(*obj)->u.b, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "AltEnumBool");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_AltEnumBool(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_AltEnumNum(Visitor *v, const char *name, AltEnumNum **obj, Error **errp)
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
        visit_type_EnumOne(v, name, &(*obj)->u.e, &err);
        break;
    case QTYPE_QNUM:
        visit_type_number(v, name, &(*obj)->u.n, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "AltEnumNum");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_AltEnumNum(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_AltNumEnum(Visitor *v, const char *name, AltNumEnum **obj, Error **errp)
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
    case QTYPE_QNUM:
        visit_type_number(v, name, &(*obj)->u.n, &err);
        break;
    case QTYPE_QSTRING:
        visit_type_EnumOne(v, name, &(*obj)->u.e, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "AltNumEnum");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_AltNumEnum(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_AltEnumInt(Visitor *v, const char *name, AltEnumInt **obj, Error **errp)
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
        visit_type_EnumOne(v, name, &(*obj)->u.e, &err);
        break;
    case QTYPE_QNUM:
        visit_type_int(v, name, &(*obj)->u.i, &err);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "AltEnumInt");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_AltEnumInt(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_AltStrObj(Visitor *v, const char *name, AltStrObj **obj, Error **errp)
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
    case QTYPE_QDICT:
        visit_start_struct(v, name, NULL, 0, &err);
        if (err) {
            break;
        }
        visit_type_TestStruct_members(v, &(*obj)->u.o, &err);
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "AltStrObj");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_AltStrObj(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_intList_wrapper_members(Visitor *v, q_obj_intList_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_intList(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_int8List_wrapper_members(Visitor *v, q_obj_int8List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int8List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_int16List_wrapper_members(Visitor *v, q_obj_int16List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int16List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_int32List_wrapper_members(Visitor *v, q_obj_int32List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int32List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_int64List_wrapper_members(Visitor *v, q_obj_int64List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int64List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_uint8List_wrapper_members(Visitor *v, q_obj_uint8List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint8List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_uint16List_wrapper_members(Visitor *v, q_obj_uint16List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint16List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_uint32List_wrapper_members(Visitor *v, q_obj_uint32List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint32List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_uint64List_wrapper_members(Visitor *v, q_obj_uint64List_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint64List(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_numberList_wrapper_members(Visitor *v, q_obj_numberList_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_numberList(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_boolList_wrapper_members(Visitor *v, q_obj_boolList_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_boolList(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_strList_wrapper_members(Visitor *v, q_obj_strList_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_strList(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_sizeList_wrapper_members(Visitor *v, q_obj_sizeList_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_sizeList(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_anyList_wrapper_members(Visitor *v, q_obj_anyList_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_anyList(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefNativeListUnionKind(Visitor *v, const char *name, UserDefNativeListUnionKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &UserDefNativeListUnionKind_lookup, errp);
    *obj = value;
}

void visit_type_UserDefNativeListUnion_members(Visitor *v, UserDefNativeListUnion *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefNativeListUnionKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case USER_DEF_NATIVE_LIST_UNION_KIND_INTEGER:
        visit_type_q_obj_intList_wrapper_members(v, &obj->u.integer, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_S8:
        visit_type_q_obj_int8List_wrapper_members(v, &obj->u.s8, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_S16:
        visit_type_q_obj_int16List_wrapper_members(v, &obj->u.s16, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_S32:
        visit_type_q_obj_int32List_wrapper_members(v, &obj->u.s32, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_S64:
        visit_type_q_obj_int64List_wrapper_members(v, &obj->u.s64, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_U8:
        visit_type_q_obj_uint8List_wrapper_members(v, &obj->u.u8, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_U16:
        visit_type_q_obj_uint16List_wrapper_members(v, &obj->u.u16, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_U32:
        visit_type_q_obj_uint32List_wrapper_members(v, &obj->u.u32, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_U64:
        visit_type_q_obj_uint64List_wrapper_members(v, &obj->u.u64, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_NUMBER:
        visit_type_q_obj_numberList_wrapper_members(v, &obj->u.number, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_BOOLEAN:
        visit_type_q_obj_boolList_wrapper_members(v, &obj->u.boolean, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_STRING:
        visit_type_q_obj_strList_wrapper_members(v, &obj->u.string, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_SIZES:
        visit_type_q_obj_sizeList_wrapper_members(v, &obj->u.sizes, &err);
        break;
    case USER_DEF_NATIVE_LIST_UNION_KIND_ANY:
        visit_type_q_obj_anyList_wrapper_members(v, &obj->u.any, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefNativeListUnion(Visitor *v, const char *name, UserDefNativeListUnion **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefNativeListUnion), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefNativeListUnion_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefNativeListUnion(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_user_def_cmd1_arg_members(Visitor *v, q_obj_user_def_cmd1_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefOne(v, "ud1a", &obj->ud1a, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_user_def_cmd2_arg_members(Visitor *v, q_obj_user_def_cmd2_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefOne(v, "ud1a", &obj->ud1a, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "ud1b", &obj->has_ud1b)) {
        visit_type_UserDefOne(v, "ud1b", &obj->ud1b, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_guest_get_time_arg_members(Visitor *v, q_obj_guest_get_time_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "a", &obj->a, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "b", &obj->has_b)) {
        visit_type_int(v, "b", &obj->b, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_guest_sync_arg_members(Visitor *v, q_obj_guest_sync_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_any(v, "arg", &obj->arg, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefOptions_members(Visitor *v, UserDefOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "i64", &obj->has_i64)) {
        visit_type_intList(v, "i64", &obj->i64, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "u64", &obj->has_u64)) {
        visit_type_uint64List(v, "u64", &obj->u64, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "u16", &obj->has_u16)) {
        visit_type_uint16List(v, "u16", &obj->u16, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "i64x", &obj->has_i64x)) {
        visit_type_int(v, "i64x", &obj->i64x, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "u64x", &obj->has_u64x)) {
        visit_type_uint64(v, "u64x", &obj->u64x, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_UserDefOptions(Visitor *v, const char *name, UserDefOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UserDefOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UserDefOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UserDefOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_EventStructOne_members(Visitor *v, EventStructOne *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UserDefOne(v, "struct1", &obj->struct1, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "string", &obj->string, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "enum2", &obj->has_enum2)) {
        visit_type_EnumOne(v, "enum2", &obj->enum2, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_EventStructOne(Visitor *v, const char *name, EventStructOne **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(EventStructOne), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_EventStructOne_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_EventStructOne(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_EVENT_C_arg_members(Visitor *v, q_obj_EVENT_C_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "a", &obj->has_a)) {
        visit_type_int(v, "a", &obj->a, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "b", &obj->has_b)) {
        visit_type_UserDefOne(v, "b", &obj->b, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "c", &obj->c, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_EVENT_D_arg_members(Visitor *v, q_obj_EVENT_D_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_EventStructOne(v, "a", &obj->a, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "b", &obj->b, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "c", &obj->has_c)) {
        visit_type_str(v, "c", &obj->c, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "enum3", &obj->has_enum3)) {
        visit_type_EnumOne(v, "enum3", &obj->enum3, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Enum(Visitor *v, const char *name, __org_qemu_x_Enum *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &__org_qemu_x_Enum_lookup, errp);
    *obj = value;
}

void visit_type___org_qemu_x_Base_members(Visitor *v, __org_qemu_x_Base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type___org_qemu_x_Enum(v, "__org.qemu_x-member1", &obj->__org_qemu_x_member1, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Base(Visitor *v, const char *name, __org_qemu_x_Base **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(__org_qemu_x_Base), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type___org_qemu_x_Base_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_Base(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Struct_members(Visitor *v, __org_qemu_x_Struct *obj, Error **errp)
{
    Error *err = NULL;

    visit_type___org_qemu_x_Base_members(v, (__org_qemu_x_Base *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "__org.qemu_x-member2", &obj->__org_qemu_x_member2, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "wchar-t", &obj->has_q_wchar_t)) {
        visit_type_int(v, "wchar-t", &obj->q_wchar_t, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Struct(Visitor *v, const char *name, __org_qemu_x_Struct **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(__org_qemu_x_Struct), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type___org_qemu_x_Struct_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_Struct(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_str_wrapper_members(Visitor *v, q_obj_str_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Union1Kind(Visitor *v, const char *name, __org_qemu_x_Union1Kind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &__org_qemu_x_Union1Kind_lookup, errp);
    *obj = value;
}

void visit_type___org_qemu_x_Union1_members(Visitor *v, __org_qemu_x_Union1 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type___org_qemu_x_Union1Kind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case ORG_QEMU_X_UNION1_KIND___ORG_QEMU_X_BRANCH:
        visit_type_q_obj_str_wrapper_members(v, &obj->u.__org_qemu_x_branch, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Union1(Visitor *v, const char *name, __org_qemu_x_Union1 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(__org_qemu_x_Union1), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type___org_qemu_x_Union1_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_Union1(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Union1List(Visitor *v, const char *name, __org_qemu_x_Union1List **obj, Error **errp)
{
    Error *err = NULL;
    __org_qemu_x_Union1List *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (__org_qemu_x_Union1List *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type___org_qemu_x_Union1(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_Union1List(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Struct2_members(Visitor *v, __org_qemu_x_Struct2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type___org_qemu_x_Union1List(v, "array", &obj->array, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Struct2(Visitor *v, const char *name, __org_qemu_x_Struct2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(__org_qemu_x_Struct2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type___org_qemu_x_Struct2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_Struct2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Union2_members(Visitor *v, __org_qemu_x_Union2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type___org_qemu_x_Base_members(v, (__org_qemu_x_Base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->__org_qemu_x_member1) {
    case ORG_QEMU_X_ENUM___ORG_QEMU_X_VALUE:
        visit_type___org_qemu_x_Struct2_members(v, &obj->u.__org_qemu_x_value, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Union2(Visitor *v, const char *name, __org_qemu_x_Union2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(__org_qemu_x_Union2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type___org_qemu_x_Union2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_Union2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_Alt(Visitor *v, const char *name, __org_qemu_x_Alt **obj, Error **errp)
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
        visit_type_str(v, name, &(*obj)->u.__org_qemu_x_branch, &err);
        break;
    case QTYPE_QDICT:
        visit_start_struct(v, name, NULL, 0, &err);
        if (err) {
            break;
        }
        visit_type___org_qemu_x_Base_members(v, &(*obj)->u.b, &err);
        if (!err) {
            visit_check_struct(v, &err);
        }
        visit_end_struct(v, NULL);
        break;
    case QTYPE_NONE:
        abort();
    default:
        error_setg(&err, QERR_INVALID_PARAMETER_TYPE, name ? name : "null",
                   "__org.qemu_x-Alt");
    }
out_obj:
    visit_end_alternate(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_Alt(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_EnumList(Visitor *v, const char *name, __org_qemu_x_EnumList **obj, Error **errp)
{
    Error *err = NULL;
    __org_qemu_x_EnumList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (__org_qemu_x_EnumList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type___org_qemu_x_Enum(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_EnumList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type___org_qemu_x_StructList(Visitor *v, const char *name, __org_qemu_x_StructList **obj, Error **errp)
{
    Error *err = NULL;
    __org_qemu_x_StructList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (__org_qemu_x_StructList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type___org_qemu_x_Struct(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free___org_qemu_x_StructList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj___org_qemu_x_command_arg_members(Visitor *v, q_obj___org_qemu_x_command_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type___org_qemu_x_EnumList(v, "a", &obj->a, &err);
    if (err) {
        goto out;
    }
    visit_type___org_qemu_x_StructList(v, "b", &obj->b, &err);
    if (err) {
        goto out;
    }
    visit_type___org_qemu_x_Union2(v, "c", &obj->c, &err);
    if (err) {
        goto out;
    }
    visit_type___org_qemu_x_Alt(v, "d", &obj->d, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_test_qapi_visit_c;

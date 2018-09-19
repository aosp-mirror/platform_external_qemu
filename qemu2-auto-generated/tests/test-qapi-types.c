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
#include "test-qapi-types.h"
#include "test-qapi-visit.h"

void qapi_free_TestStruct(TestStruct *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TestStruct(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_NestedEnumsOne(NestedEnumsOne *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_NestedEnumsOne(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup MyEnum_lookup = {
    .array = (const char *const[]) {
    },
    .size = MY_ENUM__MAX
};

void qapi_free_Empty1(Empty1 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_Empty1(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_Empty2(Empty2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_Empty2(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup QEnumTwo_lookup = {
    .array = (const char *const[]) {
        [QENUM_TWO_VALUE1] = "value1",
        [QENUM_TWO_VALUE2] = "value2",
    },
    .size = QENUM_TWO__MAX
};

void qapi_free_UserDefOne(UserDefOne *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefOne(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup EnumOne_lookup = {
    .array = (const char *const[]) {
        [ENUM_ONE_VALUE1] = "value1",
        [ENUM_ONE_VALUE2] = "value2",
        [ENUM_ONE_VALUE3] = "value3",
    },
    .size = ENUM_ONE__MAX
};

void qapi_free_UserDefZero(UserDefZero *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefZero(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefTwoDictDict(UserDefTwoDictDict *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefTwoDictDict(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefTwoDict(UserDefTwoDict *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefTwoDict(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefTwo(UserDefTwo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefTwo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefOneList(UserDefOneList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefOneList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefTwoList(UserDefTwoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefTwoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_TestStructList(TestStructList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_TestStructList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ForceArrays(ForceArrays *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ForceArrays(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefA(UserDefA *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefA(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefB(UserDefB *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefB(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefFlatUnion(UserDefFlatUnion *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefFlatUnion(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefUnionBase(UserDefUnionBase *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefUnionBase(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefFlatUnion2(UserDefFlatUnion2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefFlatUnion2(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_WrapAlternate(WrapAlternate *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_WrapAlternate(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefAlternate(UserDefAlternate *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefAlternate(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefC(UserDefC *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefC(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_AltEnumBool(AltEnumBool *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_AltEnumBool(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_AltEnumNum(AltEnumNum *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_AltEnumNum(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_AltNumEnum(AltNumEnum *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_AltNumEnum(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_AltEnumInt(AltEnumInt *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_AltEnumInt(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_AltStrObj(AltStrObj *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_AltStrObj(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup UserDefNativeListUnionKind_lookup = {
    .array = (const char *const[]) {
        [USER_DEF_NATIVE_LIST_UNION_KIND_INTEGER] = "integer",
        [USER_DEF_NATIVE_LIST_UNION_KIND_S8] = "s8",
        [USER_DEF_NATIVE_LIST_UNION_KIND_S16] = "s16",
        [USER_DEF_NATIVE_LIST_UNION_KIND_S32] = "s32",
        [USER_DEF_NATIVE_LIST_UNION_KIND_S64] = "s64",
        [USER_DEF_NATIVE_LIST_UNION_KIND_U8] = "u8",
        [USER_DEF_NATIVE_LIST_UNION_KIND_U16] = "u16",
        [USER_DEF_NATIVE_LIST_UNION_KIND_U32] = "u32",
        [USER_DEF_NATIVE_LIST_UNION_KIND_U64] = "u64",
        [USER_DEF_NATIVE_LIST_UNION_KIND_NUMBER] = "number",
        [USER_DEF_NATIVE_LIST_UNION_KIND_BOOLEAN] = "boolean",
        [USER_DEF_NATIVE_LIST_UNION_KIND_STRING] = "string",
        [USER_DEF_NATIVE_LIST_UNION_KIND_SIZES] = "sizes",
        [USER_DEF_NATIVE_LIST_UNION_KIND_ANY] = "any",
    },
    .size = USER_DEF_NATIVE_LIST_UNION_KIND__MAX
};

void qapi_free_UserDefNativeListUnion(UserDefNativeListUnion *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefNativeListUnion(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UserDefOptions(UserDefOptions *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UserDefOptions(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_EventStructOne(EventStructOne *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_EventStructOne(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup __org_qemu_x_Enum_lookup = {
    .array = (const char *const[]) {
        [ORG_QEMU_X_ENUM___ORG_QEMU_X_VALUE] = "__org.qemu_x-value",
    },
    .size = ORG_QEMU_X_ENUM__MAX
};

void qapi_free___org_qemu_x_Base(__org_qemu_x_Base *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Base(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free___org_qemu_x_Struct(__org_qemu_x_Struct *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Struct(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup __org_qemu_x_Union1Kind_lookup = {
    .array = (const char *const[]) {
        [ORG_QEMU_X_UNION1_KIND___ORG_QEMU_X_BRANCH] = "__org.qemu_x-branch",
    },
    .size = ORG_QEMU_X_UNION1_KIND__MAX
};

void qapi_free___org_qemu_x_Union1(__org_qemu_x_Union1 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Union1(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free___org_qemu_x_Union1List(__org_qemu_x_Union1List *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Union1List(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free___org_qemu_x_Struct2(__org_qemu_x_Struct2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Struct2(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free___org_qemu_x_Union2(__org_qemu_x_Union2 *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Union2(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free___org_qemu_x_Alt(__org_qemu_x_Alt *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_Alt(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free___org_qemu_x_EnumList(__org_qemu_x_EnumList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_EnumList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free___org_qemu_x_StructList(__org_qemu_x_StructList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type___org_qemu_x_StructList(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_test_qapi_types_c;

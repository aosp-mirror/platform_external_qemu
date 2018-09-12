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

#ifndef TEST_QAPI_TYPES_H
#define TEST_QAPI_TYPES_H

#include "qapi/qapi-builtin-types.h"

typedef struct TestStruct TestStruct;

typedef struct NestedEnumsOne NestedEnumsOne;

typedef enum MyEnum {
    MY_ENUM__MAX = 0,
} MyEnum;

#define MyEnum_str(val) \
    qapi_enum_lookup(&MyEnum_lookup, (val))

extern const QEnumLookup MyEnum_lookup;

typedef struct Empty1 Empty1;

typedef struct Empty2 Empty2;

typedef enum QEnumTwo {
    QENUM_TWO_VALUE1 = 0,
    QENUM_TWO_VALUE2 = 1,
    QENUM_TWO__MAX = 2,
} QEnumTwo;

#define QEnumTwo_str(val) \
    qapi_enum_lookup(&QEnumTwo_lookup, (val))

extern const QEnumLookup QEnumTwo_lookup;

typedef struct UserDefOne UserDefOne;

typedef enum EnumOne {
    ENUM_ONE_VALUE1 = 0,
    ENUM_ONE_VALUE2 = 1,
    ENUM_ONE_VALUE3 = 2,
    ENUM_ONE__MAX = 3,
} EnumOne;

#define EnumOne_str(val) \
    qapi_enum_lookup(&EnumOne_lookup, (val))

extern const QEnumLookup EnumOne_lookup;

typedef struct UserDefZero UserDefZero;

typedef struct UserDefTwoDictDict UserDefTwoDictDict;

typedef struct UserDefTwoDict UserDefTwoDict;

typedef struct UserDefTwo UserDefTwo;

typedef struct UserDefOneList UserDefOneList;

typedef struct UserDefTwoList UserDefTwoList;

typedef struct TestStructList TestStructList;

typedef struct ForceArrays ForceArrays;

typedef struct UserDefA UserDefA;

typedef struct UserDefB UserDefB;

typedef struct UserDefFlatUnion UserDefFlatUnion;

typedef struct UserDefUnionBase UserDefUnionBase;

typedef struct q_obj_UserDefFlatUnion2_base q_obj_UserDefFlatUnion2_base;

typedef struct UserDefFlatUnion2 UserDefFlatUnion2;

typedef struct WrapAlternate WrapAlternate;

typedef struct UserDefAlternate UserDefAlternate;

typedef struct UserDefC UserDefC;

typedef struct AltEnumBool AltEnumBool;

typedef struct AltEnumNum AltEnumNum;

typedef struct AltNumEnum AltNumEnum;

typedef struct AltEnumInt AltEnumInt;

typedef struct AltStrObj AltStrObj;

typedef struct q_obj_intList_wrapper q_obj_intList_wrapper;

typedef struct q_obj_int8List_wrapper q_obj_int8List_wrapper;

typedef struct q_obj_int16List_wrapper q_obj_int16List_wrapper;

typedef struct q_obj_int32List_wrapper q_obj_int32List_wrapper;

typedef struct q_obj_int64List_wrapper q_obj_int64List_wrapper;

typedef struct q_obj_uint8List_wrapper q_obj_uint8List_wrapper;

typedef struct q_obj_uint16List_wrapper q_obj_uint16List_wrapper;

typedef struct q_obj_uint32List_wrapper q_obj_uint32List_wrapper;

typedef struct q_obj_uint64List_wrapper q_obj_uint64List_wrapper;

typedef struct q_obj_numberList_wrapper q_obj_numberList_wrapper;

typedef struct q_obj_boolList_wrapper q_obj_boolList_wrapper;

typedef struct q_obj_strList_wrapper q_obj_strList_wrapper;

typedef struct q_obj_sizeList_wrapper q_obj_sizeList_wrapper;

typedef struct q_obj_anyList_wrapper q_obj_anyList_wrapper;

typedef enum UserDefNativeListUnionKind {
    USER_DEF_NATIVE_LIST_UNION_KIND_INTEGER = 0,
    USER_DEF_NATIVE_LIST_UNION_KIND_S8 = 1,
    USER_DEF_NATIVE_LIST_UNION_KIND_S16 = 2,
    USER_DEF_NATIVE_LIST_UNION_KIND_S32 = 3,
    USER_DEF_NATIVE_LIST_UNION_KIND_S64 = 4,
    USER_DEF_NATIVE_LIST_UNION_KIND_U8 = 5,
    USER_DEF_NATIVE_LIST_UNION_KIND_U16 = 6,
    USER_DEF_NATIVE_LIST_UNION_KIND_U32 = 7,
    USER_DEF_NATIVE_LIST_UNION_KIND_U64 = 8,
    USER_DEF_NATIVE_LIST_UNION_KIND_NUMBER = 9,
    USER_DEF_NATIVE_LIST_UNION_KIND_BOOLEAN = 10,
    USER_DEF_NATIVE_LIST_UNION_KIND_STRING = 11,
    USER_DEF_NATIVE_LIST_UNION_KIND_SIZES = 12,
    USER_DEF_NATIVE_LIST_UNION_KIND_ANY = 13,
    USER_DEF_NATIVE_LIST_UNION_KIND__MAX = 14,
} UserDefNativeListUnionKind;

#define UserDefNativeListUnionKind_str(val) \
    qapi_enum_lookup(&UserDefNativeListUnionKind_lookup, (val))

extern const QEnumLookup UserDefNativeListUnionKind_lookup;

typedef struct UserDefNativeListUnion UserDefNativeListUnion;

typedef struct q_obj_user_def_cmd1_arg q_obj_user_def_cmd1_arg;

typedef struct q_obj_user_def_cmd2_arg q_obj_user_def_cmd2_arg;

typedef struct q_obj_guest_get_time_arg q_obj_guest_get_time_arg;

typedef struct q_obj_guest_sync_arg q_obj_guest_sync_arg;

typedef struct UserDefOptions UserDefOptions;

typedef struct EventStructOne EventStructOne;

typedef struct q_obj_EVENT_C_arg q_obj_EVENT_C_arg;

typedef struct q_obj_EVENT_D_arg q_obj_EVENT_D_arg;

typedef enum __org_qemu_x_Enum {
    ORG_QEMU_X_ENUM___ORG_QEMU_X_VALUE = 0,
    ORG_QEMU_X_ENUM__MAX = 1,
} __org_qemu_x_Enum;

#define __org_qemu_x_Enum_str(val) \
    qapi_enum_lookup(&__org_qemu_x_Enum_lookup, (val))

extern const QEnumLookup __org_qemu_x_Enum_lookup;

typedef struct __org_qemu_x_Base __org_qemu_x_Base;

typedef struct __org_qemu_x_Struct __org_qemu_x_Struct;

typedef struct q_obj_str_wrapper q_obj_str_wrapper;

typedef enum __org_qemu_x_Union1Kind {
    ORG_QEMU_X_UNION1_KIND___ORG_QEMU_X_BRANCH = 0,
    ORG_QEMU_X_UNION1_KIND__MAX = 1,
} __org_qemu_x_Union1Kind;

#define __org_qemu_x_Union1Kind_str(val) \
    qapi_enum_lookup(&__org_qemu_x_Union1Kind_lookup, (val))

extern const QEnumLookup __org_qemu_x_Union1Kind_lookup;

typedef struct __org_qemu_x_Union1 __org_qemu_x_Union1;

typedef struct __org_qemu_x_Union1List __org_qemu_x_Union1List;

typedef struct __org_qemu_x_Struct2 __org_qemu_x_Struct2;

typedef struct __org_qemu_x_Union2 __org_qemu_x_Union2;

typedef struct __org_qemu_x_Alt __org_qemu_x_Alt;

typedef struct __org_qemu_x_EnumList __org_qemu_x_EnumList;

typedef struct __org_qemu_x_StructList __org_qemu_x_StructList;

typedef struct q_obj___org_qemu_x_command_arg q_obj___org_qemu_x_command_arg;

struct TestStruct {
    int64_t integer;
    bool boolean;
    char *string;
};

void qapi_free_TestStruct(TestStruct *obj);

struct NestedEnumsOne {
    EnumOne enum1;
    bool has_enum2;
    EnumOne enum2;
    EnumOne enum3;
    bool has_enum4;
    EnumOne enum4;
};

void qapi_free_NestedEnumsOne(NestedEnumsOne *obj);

struct Empty1 {
    char qapi_dummy_for_empty_struct;
};

void qapi_free_Empty1(Empty1 *obj);

struct Empty2 {
    /* Members inherited from Empty1: */
    /* Own members: */
    char qapi_dummy_for_empty_struct;
};

static inline Empty1 *qapi_Empty2_base(const Empty2 *obj)
{
    return (Empty1 *)obj;
}

void qapi_free_Empty2(Empty2 *obj);

struct UserDefOne {
    /* Members inherited from UserDefZero: */
    int64_t integer;
    /* Own members: */
    char *string;
    bool has_enum1;
    EnumOne enum1;
};

static inline UserDefZero *qapi_UserDefOne_base(const UserDefOne *obj)
{
    return (UserDefZero *)obj;
}

void qapi_free_UserDefOne(UserDefOne *obj);

struct UserDefZero {
    int64_t integer;
};

void qapi_free_UserDefZero(UserDefZero *obj);

struct UserDefTwoDictDict {
    UserDefOne *userdef;
    char *string;
};

void qapi_free_UserDefTwoDictDict(UserDefTwoDictDict *obj);

struct UserDefTwoDict {
    char *string1;
    UserDefTwoDictDict *dict2;
    bool has_dict3;
    UserDefTwoDictDict *dict3;
};

void qapi_free_UserDefTwoDict(UserDefTwoDict *obj);

struct UserDefTwo {
    char *string0;
    UserDefTwoDict *dict1;
};

void qapi_free_UserDefTwo(UserDefTwo *obj);

struct UserDefOneList {
    UserDefOneList *next;
    UserDefOne *value;
};

void qapi_free_UserDefOneList(UserDefOneList *obj);

struct UserDefTwoList {
    UserDefTwoList *next;
    UserDefTwo *value;
};

void qapi_free_UserDefTwoList(UserDefTwoList *obj);

struct TestStructList {
    TestStructList *next;
    TestStruct *value;
};

void qapi_free_TestStructList(TestStructList *obj);

struct ForceArrays {
    UserDefOneList *unused1;
    UserDefTwoList *unused2;
    TestStructList *unused3;
};

void qapi_free_ForceArrays(ForceArrays *obj);

struct UserDefA {
    bool boolean;
    bool has_a_b;
    int64_t a_b;
};

void qapi_free_UserDefA(UserDefA *obj);

struct UserDefB {
    int64_t intb;
    bool has_a_b;
    bool a_b;
};

void qapi_free_UserDefB(UserDefB *obj);

struct UserDefFlatUnion {
    /* Members inherited from UserDefUnionBase: */
    int64_t integer;
    char *string;
    EnumOne enum1;
    /* Own members: */
    union { /* union tag is @enum1 */
        UserDefA value1;
        UserDefB value2;
        UserDefB value3;
    } u;
};

static inline UserDefUnionBase *qapi_UserDefFlatUnion_base(const UserDefFlatUnion *obj)
{
    return (UserDefUnionBase *)obj;
}

void qapi_free_UserDefFlatUnion(UserDefFlatUnion *obj);

struct UserDefUnionBase {
    /* Members inherited from UserDefZero: */
    int64_t integer;
    /* Own members: */
    char *string;
    EnumOne enum1;
};

static inline UserDefZero *qapi_UserDefUnionBase_base(const UserDefUnionBase *obj)
{
    return (UserDefZero *)obj;
}

void qapi_free_UserDefUnionBase(UserDefUnionBase *obj);

struct q_obj_UserDefFlatUnion2_base {
    bool has_integer;
    int64_t integer;
    char *string;
    QEnumTwo enum1;
};

struct UserDefC {
    char *string1;
    char *string2;
};

struct UserDefFlatUnion2 {
    bool has_integer;
    int64_t integer;
    char *string;
    QEnumTwo enum1;
    union { /* union tag is @enum1 */
        UserDefC value1;
        UserDefB value2;
    } u;
};

void qapi_free_UserDefFlatUnion2(UserDefFlatUnion2 *obj);

struct WrapAlternate {
    UserDefAlternate *alt;
};

void qapi_free_WrapAlternate(WrapAlternate *obj);

struct UserDefAlternate {
    QType type;
    union { /* union tag is @type */
        UserDefFlatUnion udfu;
        EnumOne e;
        int64_t i;
        QNull *n;
    } u;
};

void qapi_free_UserDefAlternate(UserDefAlternate *obj);

void qapi_free_UserDefC(UserDefC *obj);

struct AltEnumBool {
    QType type;
    union { /* union tag is @type */
        EnumOne e;
        bool b;
    } u;
};

void qapi_free_AltEnumBool(AltEnumBool *obj);

struct AltEnumNum {
    QType type;
    union { /* union tag is @type */
        EnumOne e;
        double n;
    } u;
};

void qapi_free_AltEnumNum(AltEnumNum *obj);

struct AltNumEnum {
    QType type;
    union { /* union tag is @type */
        double n;
        EnumOne e;
    } u;
};

void qapi_free_AltNumEnum(AltNumEnum *obj);

struct AltEnumInt {
    QType type;
    union { /* union tag is @type */
        EnumOne e;
        int64_t i;
    } u;
};

void qapi_free_AltEnumInt(AltEnumInt *obj);

struct AltStrObj {
    QType type;
    union { /* union tag is @type */
        char *s;
        TestStruct o;
    } u;
};

void qapi_free_AltStrObj(AltStrObj *obj);

struct q_obj_intList_wrapper {
    intList *data;
};

struct q_obj_int8List_wrapper {
    int8List *data;
};

struct q_obj_int16List_wrapper {
    int16List *data;
};

struct q_obj_int32List_wrapper {
    int32List *data;
};

struct q_obj_int64List_wrapper {
    int64List *data;
};

struct q_obj_uint8List_wrapper {
    uint8List *data;
};

struct q_obj_uint16List_wrapper {
    uint16List *data;
};

struct q_obj_uint32List_wrapper {
    uint32List *data;
};

struct q_obj_uint64List_wrapper {
    uint64List *data;
};

struct q_obj_numberList_wrapper {
    numberList *data;
};

struct q_obj_boolList_wrapper {
    boolList *data;
};

struct q_obj_strList_wrapper {
    strList *data;
};

struct q_obj_sizeList_wrapper {
    sizeList *data;
};

struct q_obj_anyList_wrapper {
    anyList *data;
};

struct UserDefNativeListUnion {
    UserDefNativeListUnionKind type;
    union { /* union tag is @type */
        q_obj_intList_wrapper integer;
        q_obj_int8List_wrapper s8;
        q_obj_int16List_wrapper s16;
        q_obj_int32List_wrapper s32;
        q_obj_int64List_wrapper s64;
        q_obj_uint8List_wrapper u8;
        q_obj_uint16List_wrapper u16;
        q_obj_uint32List_wrapper u32;
        q_obj_uint64List_wrapper u64;
        q_obj_numberList_wrapper number;
        q_obj_boolList_wrapper boolean;
        q_obj_strList_wrapper string;
        q_obj_sizeList_wrapper sizes;
        q_obj_anyList_wrapper any;
    } u;
};

void qapi_free_UserDefNativeListUnion(UserDefNativeListUnion *obj);

struct q_obj_user_def_cmd1_arg {
    UserDefOne *ud1a;
};

struct q_obj_user_def_cmd2_arg {
    UserDefOne *ud1a;
    bool has_ud1b;
    UserDefOne *ud1b;
};

struct q_obj_guest_get_time_arg {
    int64_t a;
    bool has_b;
    int64_t b;
};

struct q_obj_guest_sync_arg {
    QObject *arg;
};

struct UserDefOptions {
    bool has_i64;
    intList *i64;
    bool has_u64;
    uint64List *u64;
    bool has_u16;
    uint16List *u16;
    bool has_i64x;
    int64_t i64x;
    bool has_u64x;
    uint64_t u64x;
};

void qapi_free_UserDefOptions(UserDefOptions *obj);

struct EventStructOne {
    UserDefOne *struct1;
    char *string;
    bool has_enum2;
    EnumOne enum2;
};

void qapi_free_EventStructOne(EventStructOne *obj);

struct q_obj_EVENT_C_arg {
    bool has_a;
    int64_t a;
    bool has_b;
    UserDefOne *b;
    char *c;
};

struct q_obj_EVENT_D_arg {
    EventStructOne *a;
    char *b;
    bool has_c;
    char *c;
    bool has_enum3;
    EnumOne enum3;
};

struct __org_qemu_x_Base {
    __org_qemu_x_Enum __org_qemu_x_member1;
};

void qapi_free___org_qemu_x_Base(__org_qemu_x_Base *obj);

struct __org_qemu_x_Struct {
    /* Members inherited from __org_qemu_x_Base: */
    __org_qemu_x_Enum __org_qemu_x_member1;
    /* Own members: */
    char *__org_qemu_x_member2;
    bool has_q_wchar_t;
    int64_t q_wchar_t;
};

static inline __org_qemu_x_Base *qapi___org_qemu_x_Struct_base(const __org_qemu_x_Struct *obj)
{
    return (__org_qemu_x_Base *)obj;
}

void qapi_free___org_qemu_x_Struct(__org_qemu_x_Struct *obj);

struct q_obj_str_wrapper {
    char *data;
};

struct __org_qemu_x_Union1 {
    __org_qemu_x_Union1Kind type;
    union { /* union tag is @type */
        q_obj_str_wrapper __org_qemu_x_branch;
    } u;
};

void qapi_free___org_qemu_x_Union1(__org_qemu_x_Union1 *obj);

struct __org_qemu_x_Union1List {
    __org_qemu_x_Union1List *next;
    __org_qemu_x_Union1 *value;
};

void qapi_free___org_qemu_x_Union1List(__org_qemu_x_Union1List *obj);

struct __org_qemu_x_Struct2 {
    __org_qemu_x_Union1List *array;
};

void qapi_free___org_qemu_x_Struct2(__org_qemu_x_Struct2 *obj);

struct __org_qemu_x_Union2 {
    /* Members inherited from __org_qemu_x_Base: */
    __org_qemu_x_Enum __org_qemu_x_member1;
    /* Own members: */
    union { /* union tag is @__org_qemu_x_member1 */
        __org_qemu_x_Struct2 __org_qemu_x_value;
    } u;
};

static inline __org_qemu_x_Base *qapi___org_qemu_x_Union2_base(const __org_qemu_x_Union2 *obj)
{
    return (__org_qemu_x_Base *)obj;
}

void qapi_free___org_qemu_x_Union2(__org_qemu_x_Union2 *obj);

struct __org_qemu_x_Alt {
    QType type;
    union { /* union tag is @type */
        char *__org_qemu_x_branch;
        __org_qemu_x_Base b;
    } u;
};

void qapi_free___org_qemu_x_Alt(__org_qemu_x_Alt *obj);

struct __org_qemu_x_EnumList {
    __org_qemu_x_EnumList *next;
    __org_qemu_x_Enum value;
};

void qapi_free___org_qemu_x_EnumList(__org_qemu_x_EnumList *obj);

struct __org_qemu_x_StructList {
    __org_qemu_x_StructList *next;
    __org_qemu_x_Struct *value;
};

void qapi_free___org_qemu_x_StructList(__org_qemu_x_StructList *obj);

struct q_obj___org_qemu_x_command_arg {
    __org_qemu_x_EnumList *a;
    __org_qemu_x_StructList *b;
    __org_qemu_x_Union2 *c;
    __org_qemu_x_Alt *d;
};

#endif /* TEST_QAPI_TYPES_H */

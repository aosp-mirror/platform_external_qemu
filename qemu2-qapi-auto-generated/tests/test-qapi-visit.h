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

#ifndef TEST_QAPI_VISIT_H
#define TEST_QAPI_VISIT_H

#include "qapi/qapi-builtin-visit.h"
#include "test-qapi-types.h"


void visit_type_TestStruct_members(Visitor *v, TestStruct *obj, Error **errp);
void visit_type_TestStruct(Visitor *v, const char *name, TestStruct **obj, Error **errp);

void visit_type_NestedEnumsOne_members(Visitor *v, NestedEnumsOne *obj, Error **errp);
void visit_type_NestedEnumsOne(Visitor *v, const char *name, NestedEnumsOne **obj, Error **errp);
void visit_type_MyEnum(Visitor *v, const char *name, MyEnum *obj, Error **errp);

void visit_type_Empty1_members(Visitor *v, Empty1 *obj, Error **errp);
void visit_type_Empty1(Visitor *v, const char *name, Empty1 **obj, Error **errp);

void visit_type_Empty2_members(Visitor *v, Empty2 *obj, Error **errp);
void visit_type_Empty2(Visitor *v, const char *name, Empty2 **obj, Error **errp);
void visit_type_QEnumTwo(Visitor *v, const char *name, QEnumTwo *obj, Error **errp);

void visit_type_UserDefOne_members(Visitor *v, UserDefOne *obj, Error **errp);
void visit_type_UserDefOne(Visitor *v, const char *name, UserDefOne **obj, Error **errp);
void visit_type_EnumOne(Visitor *v, const char *name, EnumOne *obj, Error **errp);

void visit_type_UserDefZero_members(Visitor *v, UserDefZero *obj, Error **errp);
void visit_type_UserDefZero(Visitor *v, const char *name, UserDefZero **obj, Error **errp);

void visit_type_UserDefTwoDictDict_members(Visitor *v, UserDefTwoDictDict *obj, Error **errp);
void visit_type_UserDefTwoDictDict(Visitor *v, const char *name, UserDefTwoDictDict **obj, Error **errp);

void visit_type_UserDefTwoDict_members(Visitor *v, UserDefTwoDict *obj, Error **errp);
void visit_type_UserDefTwoDict(Visitor *v, const char *name, UserDefTwoDict **obj, Error **errp);

void visit_type_UserDefTwo_members(Visitor *v, UserDefTwo *obj, Error **errp);
void visit_type_UserDefTwo(Visitor *v, const char *name, UserDefTwo **obj, Error **errp);
void visit_type_UserDefOneList(Visitor *v, const char *name, UserDefOneList **obj, Error **errp);
void visit_type_UserDefTwoList(Visitor *v, const char *name, UserDefTwoList **obj, Error **errp);
void visit_type_TestStructList(Visitor *v, const char *name, TestStructList **obj, Error **errp);

void visit_type_ForceArrays_members(Visitor *v, ForceArrays *obj, Error **errp);
void visit_type_ForceArrays(Visitor *v, const char *name, ForceArrays **obj, Error **errp);

void visit_type_UserDefA_members(Visitor *v, UserDefA *obj, Error **errp);
void visit_type_UserDefA(Visitor *v, const char *name, UserDefA **obj, Error **errp);

void visit_type_UserDefB_members(Visitor *v, UserDefB *obj, Error **errp);
void visit_type_UserDefB(Visitor *v, const char *name, UserDefB **obj, Error **errp);

void visit_type_UserDefFlatUnion_members(Visitor *v, UserDefFlatUnion *obj, Error **errp);
void visit_type_UserDefFlatUnion(Visitor *v, const char *name, UserDefFlatUnion **obj, Error **errp);

void visit_type_UserDefUnionBase_members(Visitor *v, UserDefUnionBase *obj, Error **errp);
void visit_type_UserDefUnionBase(Visitor *v, const char *name, UserDefUnionBase **obj, Error **errp);

void visit_type_q_obj_UserDefFlatUnion2_base_members(Visitor *v, q_obj_UserDefFlatUnion2_base *obj, Error **errp);

void visit_type_UserDefFlatUnion2_members(Visitor *v, UserDefFlatUnion2 *obj, Error **errp);
void visit_type_UserDefFlatUnion2(Visitor *v, const char *name, UserDefFlatUnion2 **obj, Error **errp);

void visit_type_WrapAlternate_members(Visitor *v, WrapAlternate *obj, Error **errp);
void visit_type_WrapAlternate(Visitor *v, const char *name, WrapAlternate **obj, Error **errp);
void visit_type_UserDefAlternate(Visitor *v, const char *name, UserDefAlternate **obj, Error **errp);

void visit_type_UserDefC_members(Visitor *v, UserDefC *obj, Error **errp);
void visit_type_UserDefC(Visitor *v, const char *name, UserDefC **obj, Error **errp);
void visit_type_AltEnumBool(Visitor *v, const char *name, AltEnumBool **obj, Error **errp);
void visit_type_AltEnumNum(Visitor *v, const char *name, AltEnumNum **obj, Error **errp);
void visit_type_AltNumEnum(Visitor *v, const char *name, AltNumEnum **obj, Error **errp);
void visit_type_AltEnumInt(Visitor *v, const char *name, AltEnumInt **obj, Error **errp);
void visit_type_AltStrObj(Visitor *v, const char *name, AltStrObj **obj, Error **errp);

void visit_type_q_obj_intList_wrapper_members(Visitor *v, q_obj_intList_wrapper *obj, Error **errp);

void visit_type_q_obj_int8List_wrapper_members(Visitor *v, q_obj_int8List_wrapper *obj, Error **errp);

void visit_type_q_obj_int16List_wrapper_members(Visitor *v, q_obj_int16List_wrapper *obj, Error **errp);

void visit_type_q_obj_int32List_wrapper_members(Visitor *v, q_obj_int32List_wrapper *obj, Error **errp);

void visit_type_q_obj_int64List_wrapper_members(Visitor *v, q_obj_int64List_wrapper *obj, Error **errp);

void visit_type_q_obj_uint8List_wrapper_members(Visitor *v, q_obj_uint8List_wrapper *obj, Error **errp);

void visit_type_q_obj_uint16List_wrapper_members(Visitor *v, q_obj_uint16List_wrapper *obj, Error **errp);

void visit_type_q_obj_uint32List_wrapper_members(Visitor *v, q_obj_uint32List_wrapper *obj, Error **errp);

void visit_type_q_obj_uint64List_wrapper_members(Visitor *v, q_obj_uint64List_wrapper *obj, Error **errp);

void visit_type_q_obj_numberList_wrapper_members(Visitor *v, q_obj_numberList_wrapper *obj, Error **errp);

void visit_type_q_obj_boolList_wrapper_members(Visitor *v, q_obj_boolList_wrapper *obj, Error **errp);

void visit_type_q_obj_strList_wrapper_members(Visitor *v, q_obj_strList_wrapper *obj, Error **errp);

void visit_type_q_obj_sizeList_wrapper_members(Visitor *v, q_obj_sizeList_wrapper *obj, Error **errp);

void visit_type_q_obj_anyList_wrapper_members(Visitor *v, q_obj_anyList_wrapper *obj, Error **errp);
void visit_type_UserDefNativeListUnionKind(Visitor *v, const char *name, UserDefNativeListUnionKind *obj, Error **errp);

void visit_type_UserDefNativeListUnion_members(Visitor *v, UserDefNativeListUnion *obj, Error **errp);
void visit_type_UserDefNativeListUnion(Visitor *v, const char *name, UserDefNativeListUnion **obj, Error **errp);

void visit_type_q_obj_user_def_cmd1_arg_members(Visitor *v, q_obj_user_def_cmd1_arg *obj, Error **errp);

void visit_type_q_obj_user_def_cmd2_arg_members(Visitor *v, q_obj_user_def_cmd2_arg *obj, Error **errp);

void visit_type_q_obj_guest_get_time_arg_members(Visitor *v, q_obj_guest_get_time_arg *obj, Error **errp);

void visit_type_q_obj_guest_sync_arg_members(Visitor *v, q_obj_guest_sync_arg *obj, Error **errp);

void visit_type_UserDefOptions_members(Visitor *v, UserDefOptions *obj, Error **errp);
void visit_type_UserDefOptions(Visitor *v, const char *name, UserDefOptions **obj, Error **errp);

void visit_type_EventStructOne_members(Visitor *v, EventStructOne *obj, Error **errp);
void visit_type_EventStructOne(Visitor *v, const char *name, EventStructOne **obj, Error **errp);

void visit_type_q_obj_EVENT_C_arg_members(Visitor *v, q_obj_EVENT_C_arg *obj, Error **errp);

void visit_type_q_obj_EVENT_D_arg_members(Visitor *v, q_obj_EVENT_D_arg *obj, Error **errp);
void visit_type___org_qemu_x_Enum(Visitor *v, const char *name, __org_qemu_x_Enum *obj, Error **errp);

void visit_type___org_qemu_x_Base_members(Visitor *v, __org_qemu_x_Base *obj, Error **errp);
void visit_type___org_qemu_x_Base(Visitor *v, const char *name, __org_qemu_x_Base **obj, Error **errp);

void visit_type___org_qemu_x_Struct_members(Visitor *v, __org_qemu_x_Struct *obj, Error **errp);
void visit_type___org_qemu_x_Struct(Visitor *v, const char *name, __org_qemu_x_Struct **obj, Error **errp);

void visit_type_q_obj_str_wrapper_members(Visitor *v, q_obj_str_wrapper *obj, Error **errp);
void visit_type___org_qemu_x_Union1Kind(Visitor *v, const char *name, __org_qemu_x_Union1Kind *obj, Error **errp);

void visit_type___org_qemu_x_Union1_members(Visitor *v, __org_qemu_x_Union1 *obj, Error **errp);
void visit_type___org_qemu_x_Union1(Visitor *v, const char *name, __org_qemu_x_Union1 **obj, Error **errp);
void visit_type___org_qemu_x_Union1List(Visitor *v, const char *name, __org_qemu_x_Union1List **obj, Error **errp);

void visit_type___org_qemu_x_Struct2_members(Visitor *v, __org_qemu_x_Struct2 *obj, Error **errp);
void visit_type___org_qemu_x_Struct2(Visitor *v, const char *name, __org_qemu_x_Struct2 **obj, Error **errp);

void visit_type___org_qemu_x_Union2_members(Visitor *v, __org_qemu_x_Union2 *obj, Error **errp);
void visit_type___org_qemu_x_Union2(Visitor *v, const char *name, __org_qemu_x_Union2 **obj, Error **errp);
void visit_type___org_qemu_x_Alt(Visitor *v, const char *name, __org_qemu_x_Alt **obj, Error **errp);
void visit_type___org_qemu_x_EnumList(Visitor *v, const char *name, __org_qemu_x_EnumList **obj, Error **errp);
void visit_type___org_qemu_x_StructList(Visitor *v, const char *name, __org_qemu_x_StructList **obj, Error **errp);

void visit_type_q_obj___org_qemu_x_command_arg_members(Visitor *v, q_obj___org_qemu_x_command_arg *obj, Error **errp);

#endif /* TEST_QAPI_VISIT_H */

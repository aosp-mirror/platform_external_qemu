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

#ifndef QAPI_VISIT_INTROSPECT_H
#define QAPI_VISIT_INTROSPECT_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-introspect.h"

void visit_type_SchemaInfoList(Visitor *v, const char *name, SchemaInfoList **obj, Error **errp);
void visit_type_SchemaMetaType(Visitor *v, const char *name, SchemaMetaType *obj, Error **errp);

void visit_type_q_obj_SchemaInfo_base_members(Visitor *v, q_obj_SchemaInfo_base *obj, Error **errp);

void visit_type_SchemaInfo_members(Visitor *v, SchemaInfo *obj, Error **errp);
void visit_type_SchemaInfo(Visitor *v, const char *name, SchemaInfo **obj, Error **errp);

void visit_type_SchemaInfoBuiltin_members(Visitor *v, SchemaInfoBuiltin *obj, Error **errp);
void visit_type_SchemaInfoBuiltin(Visitor *v, const char *name, SchemaInfoBuiltin **obj, Error **errp);
void visit_type_JSONType(Visitor *v, const char *name, JSONType *obj, Error **errp);

void visit_type_SchemaInfoEnum_members(Visitor *v, SchemaInfoEnum *obj, Error **errp);
void visit_type_SchemaInfoEnum(Visitor *v, const char *name, SchemaInfoEnum **obj, Error **errp);

void visit_type_SchemaInfoArray_members(Visitor *v, SchemaInfoArray *obj, Error **errp);
void visit_type_SchemaInfoArray(Visitor *v, const char *name, SchemaInfoArray **obj, Error **errp);
void visit_type_SchemaInfoObjectMemberList(Visitor *v, const char *name, SchemaInfoObjectMemberList **obj, Error **errp);
void visit_type_SchemaInfoObjectVariantList(Visitor *v, const char *name, SchemaInfoObjectVariantList **obj, Error **errp);

void visit_type_SchemaInfoObject_members(Visitor *v, SchemaInfoObject *obj, Error **errp);
void visit_type_SchemaInfoObject(Visitor *v, const char *name, SchemaInfoObject **obj, Error **errp);

void visit_type_SchemaInfoObjectMember_members(Visitor *v, SchemaInfoObjectMember *obj, Error **errp);
void visit_type_SchemaInfoObjectMember(Visitor *v, const char *name, SchemaInfoObjectMember **obj, Error **errp);

void visit_type_SchemaInfoObjectVariant_members(Visitor *v, SchemaInfoObjectVariant *obj, Error **errp);
void visit_type_SchemaInfoObjectVariant(Visitor *v, const char *name, SchemaInfoObjectVariant **obj, Error **errp);
void visit_type_SchemaInfoAlternateMemberList(Visitor *v, const char *name, SchemaInfoAlternateMemberList **obj, Error **errp);

void visit_type_SchemaInfoAlternate_members(Visitor *v, SchemaInfoAlternate *obj, Error **errp);
void visit_type_SchemaInfoAlternate(Visitor *v, const char *name, SchemaInfoAlternate **obj, Error **errp);

void visit_type_SchemaInfoAlternateMember_members(Visitor *v, SchemaInfoAlternateMember *obj, Error **errp);
void visit_type_SchemaInfoAlternateMember(Visitor *v, const char *name, SchemaInfoAlternateMember **obj, Error **errp);

void visit_type_SchemaInfoCommand_members(Visitor *v, SchemaInfoCommand *obj, Error **errp);
void visit_type_SchemaInfoCommand(Visitor *v, const char *name, SchemaInfoCommand **obj, Error **errp);

void visit_type_SchemaInfoEvent_members(Visitor *v, SchemaInfoEvent *obj, Error **errp);
void visit_type_SchemaInfoEvent(Visitor *v, const char *name, SchemaInfoEvent **obj, Error **errp);

#endif /* QAPI_VISIT_INTROSPECT_H */

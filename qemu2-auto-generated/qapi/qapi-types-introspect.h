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

#ifndef QAPI_TYPES_INTROSPECT_H
#define QAPI_TYPES_INTROSPECT_H

#include "qapi/qapi-builtin-types.h"

typedef struct SchemaInfoList SchemaInfoList;

typedef enum SchemaMetaType {
    SCHEMA_META_TYPE_BUILTIN = 0,
    SCHEMA_META_TYPE_ENUM = 1,
    SCHEMA_META_TYPE_ARRAY = 2,
    SCHEMA_META_TYPE_OBJECT = 3,
    SCHEMA_META_TYPE_ALTERNATE = 4,
    SCHEMA_META_TYPE_COMMAND = 5,
    SCHEMA_META_TYPE_EVENT = 6,
    SCHEMA_META_TYPE__MAX = 7,
} SchemaMetaType;

#define SchemaMetaType_str(val) \
    qapi_enum_lookup(&SchemaMetaType_lookup, (val))

extern const QEnumLookup SchemaMetaType_lookup;

typedef struct q_obj_SchemaInfo_base q_obj_SchemaInfo_base;

typedef struct SchemaInfo SchemaInfo;

typedef struct SchemaInfoBuiltin SchemaInfoBuiltin;

typedef enum JSONType {
    JSON_TYPE_STRING = 0,
    JSON_TYPE_NUMBER = 1,
    JSON_TYPE_INT = 2,
    JSON_TYPE_BOOLEAN = 3,
    JSON_TYPE_NULL = 4,
    JSON_TYPE_OBJECT = 5,
    JSON_TYPE_ARRAY = 6,
    JSON_TYPE_VALUE = 7,
    JSON_TYPE__MAX = 8,
} JSONType;

#define JSONType_str(val) \
    qapi_enum_lookup(&JSONType_lookup, (val))

extern const QEnumLookup JSONType_lookup;

typedef struct SchemaInfoEnum SchemaInfoEnum;

typedef struct SchemaInfoArray SchemaInfoArray;

typedef struct SchemaInfoObjectMemberList SchemaInfoObjectMemberList;

typedef struct SchemaInfoObjectVariantList SchemaInfoObjectVariantList;

typedef struct SchemaInfoObject SchemaInfoObject;

typedef struct SchemaInfoObjectMember SchemaInfoObjectMember;

typedef struct SchemaInfoObjectVariant SchemaInfoObjectVariant;

typedef struct SchemaInfoAlternateMemberList SchemaInfoAlternateMemberList;

typedef struct SchemaInfoAlternate SchemaInfoAlternate;

typedef struct SchemaInfoAlternateMember SchemaInfoAlternateMember;

typedef struct SchemaInfoCommand SchemaInfoCommand;

typedef struct SchemaInfoEvent SchemaInfoEvent;

struct SchemaInfoList {
    SchemaInfoList *next;
    SchemaInfo *value;
};

void qapi_free_SchemaInfoList(SchemaInfoList *obj);

struct q_obj_SchemaInfo_base {
    char *name;
    SchemaMetaType meta_type;
};

struct SchemaInfoBuiltin {
    JSONType json_type;
};

struct SchemaInfoEnum {
    strList *values;
};

struct SchemaInfoArray {
    char *element_type;
};

struct SchemaInfoObject {
    SchemaInfoObjectMemberList *members;
    bool has_tag;
    char *tag;
    bool has_variants;
    SchemaInfoObjectVariantList *variants;
};

struct SchemaInfoAlternate {
    SchemaInfoAlternateMemberList *members;
};

struct SchemaInfoCommand {
    char *arg_type;
    char *ret_type;
    bool allow_oob;
};

struct SchemaInfoEvent {
    char *arg_type;
};

struct SchemaInfo {
    char *name;
    SchemaMetaType meta_type;
    union { /* union tag is @meta_type */
        SchemaInfoBuiltin builtin;
        SchemaInfoEnum q_enum;
        SchemaInfoArray array;
        SchemaInfoObject object;
        SchemaInfoAlternate alternate;
        SchemaInfoCommand command;
        SchemaInfoEvent event;
    } u;
};

void qapi_free_SchemaInfo(SchemaInfo *obj);

void qapi_free_SchemaInfoBuiltin(SchemaInfoBuiltin *obj);

void qapi_free_SchemaInfoEnum(SchemaInfoEnum *obj);

void qapi_free_SchemaInfoArray(SchemaInfoArray *obj);

struct SchemaInfoObjectMemberList {
    SchemaInfoObjectMemberList *next;
    SchemaInfoObjectMember *value;
};

void qapi_free_SchemaInfoObjectMemberList(SchemaInfoObjectMemberList *obj);

struct SchemaInfoObjectVariantList {
    SchemaInfoObjectVariantList *next;
    SchemaInfoObjectVariant *value;
};

void qapi_free_SchemaInfoObjectVariantList(SchemaInfoObjectVariantList *obj);

void qapi_free_SchemaInfoObject(SchemaInfoObject *obj);

struct SchemaInfoObjectMember {
    char *name;
    char *type;
    bool has_q_default;
    QObject *q_default;
};

void qapi_free_SchemaInfoObjectMember(SchemaInfoObjectMember *obj);

struct SchemaInfoObjectVariant {
    char *q_case;
    char *type;
};

void qapi_free_SchemaInfoObjectVariant(SchemaInfoObjectVariant *obj);

struct SchemaInfoAlternateMemberList {
    SchemaInfoAlternateMemberList *next;
    SchemaInfoAlternateMember *value;
};

void qapi_free_SchemaInfoAlternateMemberList(SchemaInfoAlternateMemberList *obj);

void qapi_free_SchemaInfoAlternate(SchemaInfoAlternate *obj);

struct SchemaInfoAlternateMember {
    char *type;
};

void qapi_free_SchemaInfoAlternateMember(SchemaInfoAlternateMember *obj);

void qapi_free_SchemaInfoCommand(SchemaInfoCommand *obj);

void qapi_free_SchemaInfoEvent(SchemaInfoEvent *obj);

#endif /* QAPI_TYPES_INTROSPECT_H */

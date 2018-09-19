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
#include "qapi-types-common.h"
#include "qapi-visit-common.h"

const QEnumLookup QapiErrorClass_lookup = {
    .array = (const char *const[]) {
        [QAPI_ERROR_CLASS_GENERICERROR] = "GenericError",
        [QAPI_ERROR_CLASS_COMMANDNOTFOUND] = "CommandNotFound",
        [QAPI_ERROR_CLASS_DEVICENOTACTIVE] = "DeviceNotActive",
        [QAPI_ERROR_CLASS_DEVICENOTFOUND] = "DeviceNotFound",
        [QAPI_ERROR_CLASS_KVMMISSINGCAP] = "KVMMissingCap",
    },
    .size = QAPI_ERROR_CLASS__MAX
};

const QEnumLookup IoOperationType_lookup = {
    .array = (const char *const[]) {
        [IO_OPERATION_TYPE_READ] = "read",
        [IO_OPERATION_TYPE_WRITE] = "write",
    },
    .size = IO_OPERATION_TYPE__MAX
};

const QEnumLookup OnOffAuto_lookup = {
    .array = (const char *const[]) {
        [ON_OFF_AUTO_AUTO] = "auto",
        [ON_OFF_AUTO_ON] = "on",
        [ON_OFF_AUTO_OFF] = "off",
    },
    .size = ON_OFF_AUTO__MAX
};

const QEnumLookup OnOffSplit_lookup = {
    .array = (const char *const[]) {
        [ON_OFF_SPLIT_ON] = "on",
        [ON_OFF_SPLIT_OFF] = "off",
        [ON_OFF_SPLIT_SPLIT] = "split",
    },
    .size = ON_OFF_SPLIT__MAX
};

void qapi_free_String(String *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_String(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_StrOrNull(StrOrNull *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_StrOrNull(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup OffAutoPCIBAR_lookup = {
    .array = (const char *const[]) {
        [OFF_AUTOPCIBAR_OFF] = "off",
        [OFF_AUTOPCIBAR_AUTO] = "auto",
        [OFF_AUTOPCIBAR_BAR0] = "bar0",
        [OFF_AUTOPCIBAR_BAR1] = "bar1",
        [OFF_AUTOPCIBAR_BAR2] = "bar2",
        [OFF_AUTOPCIBAR_BAR3] = "bar3",
        [OFF_AUTOPCIBAR_BAR4] = "bar4",
        [OFF_AUTOPCIBAR_BAR5] = "bar5",
    },
    .size = OFF_AUTOPCIBAR__MAX
};
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_common_c;

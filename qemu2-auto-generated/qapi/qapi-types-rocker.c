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
#include "qapi-types-rocker.h"
#include "qapi-visit-rocker.h"

void qapi_free_RockerSwitch(RockerSwitch *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerSwitch(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup RockerPortDuplex_lookup = {
    .array = (const char *const[]) {
        [ROCKER_PORT_DUPLEX_HALF] = "half",
        [ROCKER_PORT_DUPLEX_FULL] = "full",
    },
    .size = ROCKER_PORT_DUPLEX__MAX
};

const QEnumLookup RockerPortAutoneg_lookup = {
    .array = (const char *const[]) {
        [ROCKER_PORT_AUTONEG_OFF] = "off",
        [ROCKER_PORT_AUTONEG_ON] = "on",
    },
    .size = ROCKER_PORT_AUTONEG__MAX
};

void qapi_free_RockerPort(RockerPort *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerPort(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerPortList(RockerPortList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerPortList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerOfDpaFlowKey(RockerOfDpaFlowKey *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaFlowKey(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerOfDpaFlowMask(RockerOfDpaFlowMask *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaFlowMask(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerOfDpaFlowAction(RockerOfDpaFlowAction *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaFlowAction(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerOfDpaFlow(RockerOfDpaFlow *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaFlow(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerOfDpaFlowList(RockerOfDpaFlowList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaFlowList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerOfDpaGroup(RockerOfDpaGroup *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaGroup(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_RockerOfDpaGroupList(RockerOfDpaGroupList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_RockerOfDpaGroupList(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_rocker_c;

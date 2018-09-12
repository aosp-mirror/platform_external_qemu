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

#ifndef QAPI_VISIT_RUN_STATE_H
#define QAPI_VISIT_RUN_STATE_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-run-state.h"

void visit_type_RunState(Visitor *v, const char *name, RunState *obj, Error **errp);

void visit_type_StatusInfo_members(Visitor *v, StatusInfo *obj, Error **errp);
void visit_type_StatusInfo(Visitor *v, const char *name, StatusInfo **obj, Error **errp);

void visit_type_q_obj_SHUTDOWN_arg_members(Visitor *v, q_obj_SHUTDOWN_arg *obj, Error **errp);

void visit_type_q_obj_RESET_arg_members(Visitor *v, q_obj_RESET_arg *obj, Error **errp);

void visit_type_q_obj_WATCHDOG_arg_members(Visitor *v, q_obj_WATCHDOG_arg *obj, Error **errp);
void visit_type_WatchdogAction(Visitor *v, const char *name, WatchdogAction *obj, Error **errp);

void visit_type_q_obj_watchdog_set_action_arg_members(Visitor *v, q_obj_watchdog_set_action_arg *obj, Error **errp);

void visit_type_q_obj_GUEST_PANICKED_arg_members(Visitor *v, q_obj_GUEST_PANICKED_arg *obj, Error **errp);
void visit_type_GuestPanicAction(Visitor *v, const char *name, GuestPanicAction *obj, Error **errp);
void visit_type_GuestPanicInformationType(Visitor *v, const char *name, GuestPanicInformationType *obj, Error **errp);

void visit_type_q_obj_GuestPanicInformation_base_members(Visitor *v, q_obj_GuestPanicInformation_base *obj, Error **errp);

void visit_type_GuestPanicInformation_members(Visitor *v, GuestPanicInformation *obj, Error **errp);
void visit_type_GuestPanicInformation(Visitor *v, const char *name, GuestPanicInformation **obj, Error **errp);

void visit_type_GuestPanicInformationHyperV_members(Visitor *v, GuestPanicInformationHyperV *obj, Error **errp);
void visit_type_GuestPanicInformationHyperV(Visitor *v, const char *name, GuestPanicInformationHyperV **obj, Error **errp);
void visit_type_S390CrashReason(Visitor *v, const char *name, S390CrashReason *obj, Error **errp);

void visit_type_GuestPanicInformationS390_members(Visitor *v, GuestPanicInformationS390 *obj, Error **errp);
void visit_type_GuestPanicInformationS390(Visitor *v, const char *name, GuestPanicInformationS390 **obj, Error **errp);

#endif /* QAPI_VISIT_RUN_STATE_H */

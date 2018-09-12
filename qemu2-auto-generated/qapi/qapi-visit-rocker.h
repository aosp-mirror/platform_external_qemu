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

#ifndef QAPI_VISIT_ROCKER_H
#define QAPI_VISIT_ROCKER_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-rocker.h"


void visit_type_RockerSwitch_members(Visitor *v, RockerSwitch *obj, Error **errp);
void visit_type_RockerSwitch(Visitor *v, const char *name, RockerSwitch **obj, Error **errp);

void visit_type_q_obj_query_rocker_arg_members(Visitor *v, q_obj_query_rocker_arg *obj, Error **errp);
void visit_type_RockerPortDuplex(Visitor *v, const char *name, RockerPortDuplex *obj, Error **errp);
void visit_type_RockerPortAutoneg(Visitor *v, const char *name, RockerPortAutoneg *obj, Error **errp);

void visit_type_RockerPort_members(Visitor *v, RockerPort *obj, Error **errp);
void visit_type_RockerPort(Visitor *v, const char *name, RockerPort **obj, Error **errp);

void visit_type_q_obj_query_rocker_ports_arg_members(Visitor *v, q_obj_query_rocker_ports_arg *obj, Error **errp);
void visit_type_RockerPortList(Visitor *v, const char *name, RockerPortList **obj, Error **errp);

void visit_type_RockerOfDpaFlowKey_members(Visitor *v, RockerOfDpaFlowKey *obj, Error **errp);
void visit_type_RockerOfDpaFlowKey(Visitor *v, const char *name, RockerOfDpaFlowKey **obj, Error **errp);

void visit_type_RockerOfDpaFlowMask_members(Visitor *v, RockerOfDpaFlowMask *obj, Error **errp);
void visit_type_RockerOfDpaFlowMask(Visitor *v, const char *name, RockerOfDpaFlowMask **obj, Error **errp);

void visit_type_RockerOfDpaFlowAction_members(Visitor *v, RockerOfDpaFlowAction *obj, Error **errp);
void visit_type_RockerOfDpaFlowAction(Visitor *v, const char *name, RockerOfDpaFlowAction **obj, Error **errp);

void visit_type_RockerOfDpaFlow_members(Visitor *v, RockerOfDpaFlow *obj, Error **errp);
void visit_type_RockerOfDpaFlow(Visitor *v, const char *name, RockerOfDpaFlow **obj, Error **errp);

void visit_type_q_obj_query_rocker_of_dpa_flows_arg_members(Visitor *v, q_obj_query_rocker_of_dpa_flows_arg *obj, Error **errp);
void visit_type_RockerOfDpaFlowList(Visitor *v, const char *name, RockerOfDpaFlowList **obj, Error **errp);

void visit_type_RockerOfDpaGroup_members(Visitor *v, RockerOfDpaGroup *obj, Error **errp);
void visit_type_RockerOfDpaGroup(Visitor *v, const char *name, RockerOfDpaGroup **obj, Error **errp);

void visit_type_q_obj_query_rocker_of_dpa_groups_arg_members(Visitor *v, q_obj_query_rocker_of_dpa_groups_arg *obj, Error **errp);
void visit_type_RockerOfDpaGroupList(Visitor *v, const char *name, RockerOfDpaGroupList **obj, Error **errp);

#endif /* QAPI_VISIT_ROCKER_H */

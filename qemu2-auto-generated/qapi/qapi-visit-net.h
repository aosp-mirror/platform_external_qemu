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

#ifndef QAPI_VISIT_NET_H
#define QAPI_VISIT_NET_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-net.h"

#include "qapi-visit-common.h"

void visit_type_q_obj_set_link_arg_members(Visitor *v, q_obj_set_link_arg *obj, Error **errp);

void visit_type_q_obj_netdev_add_arg_members(Visitor *v, q_obj_netdev_add_arg *obj, Error **errp);

void visit_type_q_obj_netdev_del_arg_members(Visitor *v, q_obj_netdev_del_arg *obj, Error **errp);

void visit_type_NetdevNoneOptions_members(Visitor *v, NetdevNoneOptions *obj, Error **errp);
void visit_type_NetdevNoneOptions(Visitor *v, const char *name, NetdevNoneOptions **obj, Error **errp);

void visit_type_NetLegacyNicOptions_members(Visitor *v, NetLegacyNicOptions *obj, Error **errp);
void visit_type_NetLegacyNicOptions(Visitor *v, const char *name, NetLegacyNicOptions **obj, Error **errp);
void visit_type_StringList(Visitor *v, const char *name, StringList **obj, Error **errp);

void visit_type_NetdevUserOptions_members(Visitor *v, NetdevUserOptions *obj, Error **errp);
void visit_type_NetdevUserOptions(Visitor *v, const char *name, NetdevUserOptions **obj, Error **errp);

void visit_type_NetdevTapOptions_members(Visitor *v, NetdevTapOptions *obj, Error **errp);
void visit_type_NetdevTapOptions(Visitor *v, const char *name, NetdevTapOptions **obj, Error **errp);

void visit_type_NetdevSocketOptions_members(Visitor *v, NetdevSocketOptions *obj, Error **errp);
void visit_type_NetdevSocketOptions(Visitor *v, const char *name, NetdevSocketOptions **obj, Error **errp);

void visit_type_NetdevL2TPv3Options_members(Visitor *v, NetdevL2TPv3Options *obj, Error **errp);
void visit_type_NetdevL2TPv3Options(Visitor *v, const char *name, NetdevL2TPv3Options **obj, Error **errp);

void visit_type_NetdevVdeOptions_members(Visitor *v, NetdevVdeOptions *obj, Error **errp);
void visit_type_NetdevVdeOptions(Visitor *v, const char *name, NetdevVdeOptions **obj, Error **errp);

void visit_type_NetdevBridgeOptions_members(Visitor *v, NetdevBridgeOptions *obj, Error **errp);
void visit_type_NetdevBridgeOptions(Visitor *v, const char *name, NetdevBridgeOptions **obj, Error **errp);

void visit_type_NetdevHubPortOptions_members(Visitor *v, NetdevHubPortOptions *obj, Error **errp);
void visit_type_NetdevHubPortOptions(Visitor *v, const char *name, NetdevHubPortOptions **obj, Error **errp);

void visit_type_NetdevNetmapOptions_members(Visitor *v, NetdevNetmapOptions *obj, Error **errp);
void visit_type_NetdevNetmapOptions(Visitor *v, const char *name, NetdevNetmapOptions **obj, Error **errp);

void visit_type_NetdevVhostUserOptions_members(Visitor *v, NetdevVhostUserOptions *obj, Error **errp);
void visit_type_NetdevVhostUserOptions(Visitor *v, const char *name, NetdevVhostUserOptions **obj, Error **errp);
void visit_type_NetClientDriver(Visitor *v, const char *name, NetClientDriver *obj, Error **errp);

void visit_type_q_obj_Netdev_base_members(Visitor *v, q_obj_Netdev_base *obj, Error **errp);

void visit_type_Netdev_members(Visitor *v, Netdev *obj, Error **errp);
void visit_type_Netdev(Visitor *v, const char *name, Netdev **obj, Error **errp);

void visit_type_NetLegacy_members(Visitor *v, NetLegacy *obj, Error **errp);
void visit_type_NetLegacy(Visitor *v, const char *name, NetLegacy **obj, Error **errp);
void visit_type_NetLegacyOptionsType(Visitor *v, const char *name, NetLegacyOptionsType *obj, Error **errp);

void visit_type_q_obj_NetLegacyOptions_base_members(Visitor *v, q_obj_NetLegacyOptions_base *obj, Error **errp);

void visit_type_NetLegacyOptions_members(Visitor *v, NetLegacyOptions *obj, Error **errp);
void visit_type_NetLegacyOptions(Visitor *v, const char *name, NetLegacyOptions **obj, Error **errp);
void visit_type_NetFilterDirection(Visitor *v, const char *name, NetFilterDirection *obj, Error **errp);
void visit_type_RxState(Visitor *v, const char *name, RxState *obj, Error **errp);

void visit_type_RxFilterInfo_members(Visitor *v, RxFilterInfo *obj, Error **errp);
void visit_type_RxFilterInfo(Visitor *v, const char *name, RxFilterInfo **obj, Error **errp);

void visit_type_q_obj_query_rx_filter_arg_members(Visitor *v, q_obj_query_rx_filter_arg *obj, Error **errp);
void visit_type_RxFilterInfoList(Visitor *v, const char *name, RxFilterInfoList **obj, Error **errp);

void visit_type_q_obj_NIC_RX_FILTER_CHANGED_arg_members(Visitor *v, q_obj_NIC_RX_FILTER_CHANGED_arg *obj, Error **errp);

#endif /* QAPI_VISIT_NET_H */

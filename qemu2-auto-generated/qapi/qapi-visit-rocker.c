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

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "qapi/error.h"
#include "qapi/qmp/qerror.h"
#include "qapi-visit-rocker.h"

void visit_type_RockerSwitch_members(Visitor *v, RockerSwitch *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_uint32(v, "ports", &obj->ports, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerSwitch(Visitor *v, const char *name, RockerSwitch **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RockerSwitch), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RockerSwitch_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerSwitch(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_rocker_arg_members(Visitor *v, q_obj_query_rocker_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerPortDuplex(Visitor *v, const char *name, RockerPortDuplex *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &RockerPortDuplex_lookup, errp);
    *obj = value;
}

void visit_type_RockerPortAutoneg(Visitor *v, const char *name, RockerPortAutoneg *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &RockerPortAutoneg_lookup, errp);
    *obj = value;
}

void visit_type_RockerPort_members(Visitor *v, RockerPort *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "enabled", &obj->enabled, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "link-up", &obj->link_up, &err);
    if (err) {
        goto out;
    }
    visit_type_uint32(v, "speed", &obj->speed, &err);
    if (err) {
        goto out;
    }
    visit_type_RockerPortDuplex(v, "duplex", &obj->duplex, &err);
    if (err) {
        goto out;
    }
    visit_type_RockerPortAutoneg(v, "autoneg", &obj->autoneg, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerPort(Visitor *v, const char *name, RockerPort **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RockerPort), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RockerPort_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerPort(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_rocker_ports_arg_members(Visitor *v, q_obj_query_rocker_ports_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerPortList(Visitor *v, const char *name, RockerPortList **obj, Error **errp)
{
    Error *err = NULL;
    RockerPortList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (RockerPortList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_RockerPort(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerPortList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlowKey_members(Visitor *v, RockerOfDpaFlowKey *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint32(v, "priority", &obj->priority, &err);
    if (err) {
        goto out;
    }
    visit_type_uint32(v, "tbl-id", &obj->tbl_id, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "in-pport", &obj->has_in_pport)) {
        visit_type_uint32(v, "in-pport", &obj->in_pport, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tunnel-id", &obj->has_tunnel_id)) {
        visit_type_uint32(v, "tunnel-id", &obj->tunnel_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vlan-id", &obj->has_vlan_id)) {
        visit_type_uint16(v, "vlan-id", &obj->vlan_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "eth-type", &obj->has_eth_type)) {
        visit_type_uint16(v, "eth-type", &obj->eth_type, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "eth-src", &obj->has_eth_src)) {
        visit_type_str(v, "eth-src", &obj->eth_src, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "eth-dst", &obj->has_eth_dst)) {
        visit_type_str(v, "eth-dst", &obj->eth_dst, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ip-proto", &obj->has_ip_proto)) {
        visit_type_uint8(v, "ip-proto", &obj->ip_proto, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ip-tos", &obj->has_ip_tos)) {
        visit_type_uint8(v, "ip-tos", &obj->ip_tos, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ip-dst", &obj->has_ip_dst)) {
        visit_type_str(v, "ip-dst", &obj->ip_dst, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlowKey(Visitor *v, const char *name, RockerOfDpaFlowKey **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RockerOfDpaFlowKey), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RockerOfDpaFlowKey_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerOfDpaFlowKey(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlowMask_members(Visitor *v, RockerOfDpaFlowMask *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "in-pport", &obj->has_in_pport)) {
        visit_type_uint32(v, "in-pport", &obj->in_pport, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tunnel-id", &obj->has_tunnel_id)) {
        visit_type_uint32(v, "tunnel-id", &obj->tunnel_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vlan-id", &obj->has_vlan_id)) {
        visit_type_uint16(v, "vlan-id", &obj->vlan_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "eth-src", &obj->has_eth_src)) {
        visit_type_str(v, "eth-src", &obj->eth_src, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "eth-dst", &obj->has_eth_dst)) {
        visit_type_str(v, "eth-dst", &obj->eth_dst, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ip-proto", &obj->has_ip_proto)) {
        visit_type_uint8(v, "ip-proto", &obj->ip_proto, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ip-tos", &obj->has_ip_tos)) {
        visit_type_uint8(v, "ip-tos", &obj->ip_tos, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlowMask(Visitor *v, const char *name, RockerOfDpaFlowMask **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RockerOfDpaFlowMask), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RockerOfDpaFlowMask_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerOfDpaFlowMask(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlowAction_members(Visitor *v, RockerOfDpaFlowAction *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "goto-tbl", &obj->has_goto_tbl)) {
        visit_type_uint32(v, "goto-tbl", &obj->goto_tbl, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "group-id", &obj->has_group_id)) {
        visit_type_uint32(v, "group-id", &obj->group_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tunnel-lport", &obj->has_tunnel_lport)) {
        visit_type_uint32(v, "tunnel-lport", &obj->tunnel_lport, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vlan-id", &obj->has_vlan_id)) {
        visit_type_uint16(v, "vlan-id", &obj->vlan_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "new-vlan-id", &obj->has_new_vlan_id)) {
        visit_type_uint16(v, "new-vlan-id", &obj->new_vlan_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "out-pport", &obj->has_out_pport)) {
        visit_type_uint32(v, "out-pport", &obj->out_pport, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlowAction(Visitor *v, const char *name, RockerOfDpaFlowAction **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RockerOfDpaFlowAction), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RockerOfDpaFlowAction_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerOfDpaFlowAction(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlow_members(Visitor *v, RockerOfDpaFlow *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint64(v, "cookie", &obj->cookie, &err);
    if (err) {
        goto out;
    }
    visit_type_uint64(v, "hits", &obj->hits, &err);
    if (err) {
        goto out;
    }
    visit_type_RockerOfDpaFlowKey(v, "key", &obj->key, &err);
    if (err) {
        goto out;
    }
    visit_type_RockerOfDpaFlowMask(v, "mask", &obj->mask, &err);
    if (err) {
        goto out;
    }
    visit_type_RockerOfDpaFlowAction(v, "action", &obj->action, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlow(Visitor *v, const char *name, RockerOfDpaFlow **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RockerOfDpaFlow), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RockerOfDpaFlow_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerOfDpaFlow(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_rocker_of_dpa_flows_arg_members(Visitor *v, q_obj_query_rocker_of_dpa_flows_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "tbl-id", &obj->has_tbl_id)) {
        visit_type_uint32(v, "tbl-id", &obj->tbl_id, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaFlowList(Visitor *v, const char *name, RockerOfDpaFlowList **obj, Error **errp)
{
    Error *err = NULL;
    RockerOfDpaFlowList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (RockerOfDpaFlowList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_RockerOfDpaFlow(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerOfDpaFlowList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaGroup_members(Visitor *v, RockerOfDpaGroup *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_uint32(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_uint8(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "vlan-id", &obj->has_vlan_id)) {
        visit_type_uint16(v, "vlan-id", &obj->vlan_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "pport", &obj->has_pport)) {
        visit_type_uint32(v, "pport", &obj->pport, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "index", &obj->has_index)) {
        visit_type_uint32(v, "index", &obj->index, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "out-pport", &obj->has_out_pport)) {
        visit_type_uint32(v, "out-pport", &obj->out_pport, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "group-id", &obj->has_group_id)) {
        visit_type_uint32(v, "group-id", &obj->group_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "set-vlan-id", &obj->has_set_vlan_id)) {
        visit_type_uint16(v, "set-vlan-id", &obj->set_vlan_id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "pop-vlan", &obj->has_pop_vlan)) {
        visit_type_uint8(v, "pop-vlan", &obj->pop_vlan, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "group-ids", &obj->has_group_ids)) {
        visit_type_uint32List(v, "group-ids", &obj->group_ids, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "set-eth-src", &obj->has_set_eth_src)) {
        visit_type_str(v, "set-eth-src", &obj->set_eth_src, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "set-eth-dst", &obj->has_set_eth_dst)) {
        visit_type_str(v, "set-eth-dst", &obj->set_eth_dst, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ttl-check", &obj->has_ttl_check)) {
        visit_type_uint8(v, "ttl-check", &obj->ttl_check, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaGroup(Visitor *v, const char *name, RockerOfDpaGroup **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RockerOfDpaGroup), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RockerOfDpaGroup_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerOfDpaGroup(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_rocker_of_dpa_groups_arg_members(Visitor *v, q_obj_query_rocker_of_dpa_groups_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "type", &obj->has_type)) {
        visit_type_uint8(v, "type", &obj->type, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_RockerOfDpaGroupList(Visitor *v, const char *name, RockerOfDpaGroupList **obj, Error **errp)
{
    Error *err = NULL;
    RockerOfDpaGroupList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (RockerOfDpaGroupList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_RockerOfDpaGroup(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RockerOfDpaGroupList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_rocker_c;

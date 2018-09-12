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
#include "qapi-visit-net.h"

void visit_type_q_obj_set_link_arg_members(Visitor *v, q_obj_set_link_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "up", &obj->up, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_netdev_add_arg_members(Visitor *v, q_obj_netdev_add_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_netdev_del_arg_members(Visitor *v, q_obj_netdev_del_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevNoneOptions_members(Visitor *v, NetdevNoneOptions *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_NetdevNoneOptions(Visitor *v, const char *name, NetdevNoneOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevNoneOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevNoneOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevNoneOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetLegacyNicOptions_members(Visitor *v, NetLegacyNicOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "netdev", &obj->has_netdev)) {
        visit_type_str(v, "netdev", &obj->netdev, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "macaddr", &obj->has_macaddr)) {
        visit_type_str(v, "macaddr", &obj->macaddr, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "model", &obj->has_model)) {
        visit_type_str(v, "model", &obj->model, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "addr", &obj->has_addr)) {
        visit_type_str(v, "addr", &obj->addr, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vectors", &obj->has_vectors)) {
        visit_type_uint32(v, "vectors", &obj->vectors, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetLegacyNicOptions(Visitor *v, const char *name, NetLegacyNicOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetLegacyNicOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetLegacyNicOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetLegacyNicOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_StringList(Visitor *v, const char *name, StringList **obj, Error **errp)
{
    Error *err = NULL;
    StringList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (StringList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_String(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_StringList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevUserOptions_members(Visitor *v, NetdevUserOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "hostname", &obj->has_hostname)) {
        visit_type_str(v, "hostname", &obj->hostname, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "restrict", &obj->has_q_restrict)) {
        visit_type_bool(v, "restrict", &obj->q_restrict, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ipv4", &obj->has_ipv4)) {
        visit_type_bool(v, "ipv4", &obj->ipv4, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ipv6", &obj->has_ipv6)) {
        visit_type_bool(v, "ipv6", &obj->ipv6, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ip", &obj->has_ip)) {
        visit_type_str(v, "ip", &obj->ip, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "net", &obj->has_net)) {
        visit_type_str(v, "net", &obj->net, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "host", &obj->has_host)) {
        visit_type_str(v, "host", &obj->host, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tftp", &obj->has_tftp)) {
        visit_type_str(v, "tftp", &obj->tftp, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "bootfile", &obj->has_bootfile)) {
        visit_type_str(v, "bootfile", &obj->bootfile, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "dhcpstart", &obj->has_dhcpstart)) {
        visit_type_str(v, "dhcpstart", &obj->dhcpstart, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "dns", &obj->has_dns)) {
        visit_type_str(v, "dns", &obj->dns, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "dnssearch", &obj->has_dnssearch)) {
        visit_type_StringList(v, "dnssearch", &obj->dnssearch, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ipv6-prefix", &obj->has_ipv6_prefix)) {
        visit_type_str(v, "ipv6-prefix", &obj->ipv6_prefix, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ipv6-prefixlen", &obj->has_ipv6_prefixlen)) {
        visit_type_int(v, "ipv6-prefixlen", &obj->ipv6_prefixlen, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ipv6-host", &obj->has_ipv6_host)) {
        visit_type_str(v, "ipv6-host", &obj->ipv6_host, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ipv6-dns", &obj->has_ipv6_dns)) {
        visit_type_str(v, "ipv6-dns", &obj->ipv6_dns, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "smb", &obj->has_smb)) {
        visit_type_str(v, "smb", &obj->smb, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "smbserver", &obj->has_smbserver)) {
        visit_type_str(v, "smbserver", &obj->smbserver, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "hostfwd", &obj->has_hostfwd)) {
        visit_type_StringList(v, "hostfwd", &obj->hostfwd, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "guestfwd", &obj->has_guestfwd)) {
        visit_type_StringList(v, "guestfwd", &obj->guestfwd, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevUserOptions(Visitor *v, const char *name, NetdevUserOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevUserOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevUserOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevUserOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevTapOptions_members(Visitor *v, NetdevTapOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "ifname", &obj->has_ifname)) {
        visit_type_str(v, "ifname", &obj->ifname, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "fd", &obj->has_fd)) {
        visit_type_str(v, "fd", &obj->fd, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "fds", &obj->has_fds)) {
        visit_type_str(v, "fds", &obj->fds, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "script", &obj->has_script)) {
        visit_type_str(v, "script", &obj->script, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "downscript", &obj->has_downscript)) {
        visit_type_str(v, "downscript", &obj->downscript, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "br", &obj->has_br)) {
        visit_type_str(v, "br", &obj->br, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "helper", &obj->has_helper)) {
        visit_type_str(v, "helper", &obj->helper, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "sndbuf", &obj->has_sndbuf)) {
        visit_type_size(v, "sndbuf", &obj->sndbuf, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vnet_hdr", &obj->has_vnet_hdr)) {
        visit_type_bool(v, "vnet_hdr", &obj->vnet_hdr, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vhost", &obj->has_vhost)) {
        visit_type_bool(v, "vhost", &obj->vhost, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vhostfd", &obj->has_vhostfd)) {
        visit_type_str(v, "vhostfd", &obj->vhostfd, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vhostfds", &obj->has_vhostfds)) {
        visit_type_str(v, "vhostfds", &obj->vhostfds, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "vhostforce", &obj->has_vhostforce)) {
        visit_type_bool(v, "vhostforce", &obj->vhostforce, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "queues", &obj->has_queues)) {
        visit_type_uint32(v, "queues", &obj->queues, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "poll-us", &obj->has_poll_us)) {
        visit_type_uint32(v, "poll-us", &obj->poll_us, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevTapOptions(Visitor *v, const char *name, NetdevTapOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevTapOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevTapOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevTapOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevSocketOptions_members(Visitor *v, NetdevSocketOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "fd", &obj->has_fd)) {
        visit_type_str(v, "fd", &obj->fd, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "listen", &obj->has_listen)) {
        visit_type_str(v, "listen", &obj->listen, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "connect", &obj->has_connect)) {
        visit_type_str(v, "connect", &obj->connect, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "mcast", &obj->has_mcast)) {
        visit_type_str(v, "mcast", &obj->mcast, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "localaddr", &obj->has_localaddr)) {
        visit_type_str(v, "localaddr", &obj->localaddr, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "udp", &obj->has_udp)) {
        visit_type_str(v, "udp", &obj->udp, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevSocketOptions(Visitor *v, const char *name, NetdevSocketOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevSocketOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevSocketOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevSocketOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevL2TPv3Options_members(Visitor *v, NetdevL2TPv3Options *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "src", &obj->src, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "dst", &obj->dst, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "srcport", &obj->has_srcport)) {
        visit_type_str(v, "srcport", &obj->srcport, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "dstport", &obj->has_dstport)) {
        visit_type_str(v, "dstport", &obj->dstport, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "ipv6", &obj->has_ipv6)) {
        visit_type_bool(v, "ipv6", &obj->ipv6, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "udp", &obj->has_udp)) {
        visit_type_bool(v, "udp", &obj->udp, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cookie64", &obj->has_cookie64)) {
        visit_type_bool(v, "cookie64", &obj->cookie64, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "counter", &obj->has_counter)) {
        visit_type_bool(v, "counter", &obj->counter, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "pincounter", &obj->has_pincounter)) {
        visit_type_bool(v, "pincounter", &obj->pincounter, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "txcookie", &obj->has_txcookie)) {
        visit_type_uint64(v, "txcookie", &obj->txcookie, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "rxcookie", &obj->has_rxcookie)) {
        visit_type_uint64(v, "rxcookie", &obj->rxcookie, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_uint32(v, "txsession", &obj->txsession, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "rxsession", &obj->has_rxsession)) {
        visit_type_uint32(v, "rxsession", &obj->rxsession, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "offset", &obj->has_offset)) {
        visit_type_uint32(v, "offset", &obj->offset, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevL2TPv3Options(Visitor *v, const char *name, NetdevL2TPv3Options **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevL2TPv3Options), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevL2TPv3Options_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevL2TPv3Options(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevVdeOptions_members(Visitor *v, NetdevVdeOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "sock", &obj->has_sock)) {
        visit_type_str(v, "sock", &obj->sock, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "port", &obj->has_port)) {
        visit_type_uint16(v, "port", &obj->port, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "group", &obj->has_group)) {
        visit_type_str(v, "group", &obj->group, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "mode", &obj->has_mode)) {
        visit_type_uint16(v, "mode", &obj->mode, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevVdeOptions(Visitor *v, const char *name, NetdevVdeOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevVdeOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevVdeOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevVdeOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevBridgeOptions_members(Visitor *v, NetdevBridgeOptions *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "br", &obj->has_br)) {
        visit_type_str(v, "br", &obj->br, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "helper", &obj->has_helper)) {
        visit_type_str(v, "helper", &obj->helper, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevBridgeOptions(Visitor *v, const char *name, NetdevBridgeOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevBridgeOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevBridgeOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevBridgeOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevHubPortOptions_members(Visitor *v, NetdevHubPortOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int32(v, "hubid", &obj->hubid, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "netdev", &obj->has_netdev)) {
        visit_type_str(v, "netdev", &obj->netdev, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevHubPortOptions(Visitor *v, const char *name, NetdevHubPortOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevHubPortOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevHubPortOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevHubPortOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevNetmapOptions_members(Visitor *v, NetdevNetmapOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "ifname", &obj->ifname, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "devname", &obj->has_devname)) {
        visit_type_str(v, "devname", &obj->devname, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevNetmapOptions(Visitor *v, const char *name, NetdevNetmapOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevNetmapOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevNetmapOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevNetmapOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetdevVhostUserOptions_members(Visitor *v, NetdevVhostUserOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "chardev", &obj->chardev, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "vhostforce", &obj->has_vhostforce)) {
        visit_type_bool(v, "vhostforce", &obj->vhostforce, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "queues", &obj->has_queues)) {
        visit_type_int(v, "queues", &obj->queues, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetdevVhostUserOptions(Visitor *v, const char *name, NetdevVhostUserOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetdevVhostUserOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetdevVhostUserOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetdevVhostUserOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetClientDriver(Visitor *v, const char *name, NetClientDriver *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &NetClientDriver_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_Netdev_base_members(Visitor *v, q_obj_Netdev_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_NetClientDriver(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_Netdev_members(Visitor *v, Netdev *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_Netdev_base_members(v, (q_obj_Netdev_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case NET_CLIENT_DRIVER_NONE:
        visit_type_NetdevNoneOptions_members(v, &obj->u.none, &err);
        break;
    case NET_CLIENT_DRIVER_NIC:
        visit_type_NetLegacyNicOptions_members(v, &obj->u.nic, &err);
        break;
    case NET_CLIENT_DRIVER_USER:
        visit_type_NetdevUserOptions_members(v, &obj->u.user, &err);
        break;
    case NET_CLIENT_DRIVER_TAP:
        visit_type_NetdevTapOptions_members(v, &obj->u.tap, &err);
        break;
    case NET_CLIENT_DRIVER_L2TPV3:
        visit_type_NetdevL2TPv3Options_members(v, &obj->u.l2tpv3, &err);
        break;
    case NET_CLIENT_DRIVER_SOCKET:
        visit_type_NetdevSocketOptions_members(v, &obj->u.socket, &err);
        break;
    case NET_CLIENT_DRIVER_VDE:
        visit_type_NetdevVdeOptions_members(v, &obj->u.vde, &err);
        break;
    case NET_CLIENT_DRIVER_BRIDGE:
        visit_type_NetdevBridgeOptions_members(v, &obj->u.bridge, &err);
        break;
    case NET_CLIENT_DRIVER_HUBPORT:
        visit_type_NetdevHubPortOptions_members(v, &obj->u.hubport, &err);
        break;
    case NET_CLIENT_DRIVER_NETMAP:
        visit_type_NetdevNetmapOptions_members(v, &obj->u.netmap, &err);
        break;
    case NET_CLIENT_DRIVER_VHOST_USER:
        visit_type_NetdevVhostUserOptions_members(v, &obj->u.vhost_user, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_Netdev(Visitor *v, const char *name, Netdev **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(Netdev), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_Netdev_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_Netdev(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetLegacy_members(Visitor *v, NetLegacy *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "vlan", &obj->has_vlan)) {
        visit_type_int32(v, "vlan", &obj->vlan, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "id", &obj->has_id)) {
        visit_type_str(v, "id", &obj->id, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "name", &obj->has_name)) {
        visit_type_str(v, "name", &obj->name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_NetLegacyOptions(v, "opts", &obj->opts, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetLegacy(Visitor *v, const char *name, NetLegacy **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetLegacy), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetLegacy_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetLegacy(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetLegacyOptionsType(Visitor *v, const char *name, NetLegacyOptionsType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &NetLegacyOptionsType_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_NetLegacyOptions_base_members(Visitor *v, q_obj_NetLegacyOptions_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_NetLegacyOptionsType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetLegacyOptions_members(Visitor *v, NetLegacyOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_NetLegacyOptions_base_members(v, (q_obj_NetLegacyOptions_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case NET_LEGACY_OPTIONS_TYPE_NONE:
        visit_type_NetdevNoneOptions_members(v, &obj->u.none, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_NIC:
        visit_type_NetLegacyNicOptions_members(v, &obj->u.nic, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_USER:
        visit_type_NetdevUserOptions_members(v, &obj->u.user, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_TAP:
        visit_type_NetdevTapOptions_members(v, &obj->u.tap, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_L2TPV3:
        visit_type_NetdevL2TPv3Options_members(v, &obj->u.l2tpv3, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_SOCKET:
        visit_type_NetdevSocketOptions_members(v, &obj->u.socket, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_VDE:
        visit_type_NetdevVdeOptions_members(v, &obj->u.vde, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_BRIDGE:
        visit_type_NetdevBridgeOptions_members(v, &obj->u.bridge, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_NETMAP:
        visit_type_NetdevNetmapOptions_members(v, &obj->u.netmap, &err);
        break;
    case NET_LEGACY_OPTIONS_TYPE_VHOST_USER:
        visit_type_NetdevVhostUserOptions_members(v, &obj->u.vhost_user, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_NetLegacyOptions(Visitor *v, const char *name, NetLegacyOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(NetLegacyOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_NetLegacyOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_NetLegacyOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_NetFilterDirection(Visitor *v, const char *name, NetFilterDirection *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &NetFilterDirection_lookup, errp);
    *obj = value;
}

void visit_type_RxState(Visitor *v, const char *name, RxState *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &RxState_lookup, errp);
    *obj = value;
}

void visit_type_RxFilterInfo_members(Visitor *v, RxFilterInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "promiscuous", &obj->promiscuous, &err);
    if (err) {
        goto out;
    }
    visit_type_RxState(v, "multicast", &obj->multicast, &err);
    if (err) {
        goto out;
    }
    visit_type_RxState(v, "unicast", &obj->unicast, &err);
    if (err) {
        goto out;
    }
    visit_type_RxState(v, "vlan", &obj->vlan, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "broadcast-allowed", &obj->broadcast_allowed, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "multicast-overflow", &obj->multicast_overflow, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "unicast-overflow", &obj->unicast_overflow, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "main-mac", &obj->main_mac, &err);
    if (err) {
        goto out;
    }
    visit_type_intList(v, "vlan-table", &obj->vlan_table, &err);
    if (err) {
        goto out;
    }
    visit_type_strList(v, "unicast-table", &obj->unicast_table, &err);
    if (err) {
        goto out;
    }
    visit_type_strList(v, "multicast-table", &obj->multicast_table, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_RxFilterInfo(Visitor *v, const char *name, RxFilterInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(RxFilterInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_RxFilterInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RxFilterInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_query_rx_filter_arg_members(Visitor *v, q_obj_query_rx_filter_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "name", &obj->has_name)) {
        visit_type_str(v, "name", &obj->name, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_RxFilterInfoList(Visitor *v, const char *name, RxFilterInfoList **obj, Error **errp)
{
    Error *err = NULL;
    RxFilterInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (RxFilterInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_RxFilterInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_RxFilterInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_NIC_RX_FILTER_CHANGED_arg_members(Visitor *v, q_obj_NIC_RX_FILTER_CHANGED_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "name", &obj->has_name)) {
        visit_type_str(v, "name", &obj->name, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_net_c;

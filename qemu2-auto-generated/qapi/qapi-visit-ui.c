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
#include "qapi-visit-ui.h"

void visit_type_q_obj_set_password_arg_members(Visitor *v, q_obj_set_password_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "protocol", &obj->protocol, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "password", &obj->password, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "connected", &obj->has_connected)) {
        visit_type_str(v, "connected", &obj->connected, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_expire_password_arg_members(Visitor *v, q_obj_expire_password_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "protocol", &obj->protocol, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "time", &obj->time, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_screendump_arg_members(Visitor *v, q_obj_screendump_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "head", &obj->has_head)) {
        visit_type_int(v, "head", &obj->head, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_SpiceBasicInfo_members(Visitor *v, SpiceBasicInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "host", &obj->host, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "port", &obj->port, &err);
    if (err) {
        goto out;
    }
    visit_type_NetworkAddressFamily(v, "family", &obj->family, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SpiceBasicInfo(Visitor *v, const char *name, SpiceBasicInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SpiceBasicInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SpiceBasicInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SpiceBasicInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SpiceServerInfo_members(Visitor *v, SpiceServerInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SpiceBasicInfo_members(v, (SpiceBasicInfo *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "auth", &obj->has_auth)) {
        visit_type_str(v, "auth", &obj->auth, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_SpiceServerInfo(Visitor *v, const char *name, SpiceServerInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SpiceServerInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SpiceServerInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SpiceServerInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SpiceChannel_members(Visitor *v, SpiceChannel *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SpiceBasicInfo_members(v, (SpiceBasicInfo *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "connection-id", &obj->connection_id, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "channel-type", &obj->channel_type, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "channel-id", &obj->channel_id, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "tls", &obj->tls, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SpiceChannel(Visitor *v, const char *name, SpiceChannel **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SpiceChannel), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SpiceChannel_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SpiceChannel(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SpiceQueryMouseMode(Visitor *v, const char *name, SpiceQueryMouseMode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SpiceQueryMouseMode_lookup, errp);
    *obj = value;
}

void visit_type_SpiceChannelList(Visitor *v, const char *name, SpiceChannelList **obj, Error **errp)
{
    Error *err = NULL;
    SpiceChannelList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (SpiceChannelList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_SpiceChannel(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SpiceChannelList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SpiceInfo_members(Visitor *v, SpiceInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "enabled", &obj->enabled, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "migrated", &obj->migrated, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "host", &obj->has_host)) {
        visit_type_str(v, "host", &obj->host, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "port", &obj->has_port)) {
        visit_type_int(v, "port", &obj->port, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tls-port", &obj->has_tls_port)) {
        visit_type_int(v, "tls-port", &obj->tls_port, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "auth", &obj->has_auth)) {
        visit_type_str(v, "auth", &obj->auth, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "compiled-version", &obj->has_compiled_version)) {
        visit_type_str(v, "compiled-version", &obj->compiled_version, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_SpiceQueryMouseMode(v, "mouse-mode", &obj->mouse_mode, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "channels", &obj->has_channels)) {
        visit_type_SpiceChannelList(v, "channels", &obj->channels, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_SpiceInfo(Visitor *v, const char *name, SpiceInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SpiceInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SpiceInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SpiceInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_SPICE_CONNECTED_arg_members(Visitor *v, q_obj_SPICE_CONNECTED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SpiceBasicInfo(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_SpiceBasicInfo(v, "client", &obj->client, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_SPICE_INITIALIZED_arg_members(Visitor *v, q_obj_SPICE_INITIALIZED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SpiceServerInfo(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_SpiceChannel(v, "client", &obj->client, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_SPICE_DISCONNECTED_arg_members(Visitor *v, q_obj_SPICE_DISCONNECTED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SpiceBasicInfo(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_SpiceBasicInfo(v, "client", &obj->client, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_VncBasicInfo_members(Visitor *v, VncBasicInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "host", &obj->host, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "service", &obj->service, &err);
    if (err) {
        goto out;
    }
    visit_type_NetworkAddressFamily(v, "family", &obj->family, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "websocket", &obj->websocket, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_VncBasicInfo(Visitor *v, const char *name, VncBasicInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VncBasicInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VncBasicInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncBasicInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncServerInfo_members(Visitor *v, VncServerInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VncBasicInfo_members(v, (VncBasicInfo *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "auth", &obj->has_auth)) {
        visit_type_str(v, "auth", &obj->auth, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_VncServerInfo(Visitor *v, const char *name, VncServerInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VncServerInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VncServerInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncServerInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncClientInfo_members(Visitor *v, VncClientInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VncBasicInfo_members(v, (VncBasicInfo *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "x509_dname", &obj->has_x509_dname)) {
        visit_type_str(v, "x509_dname", &obj->x509_dname, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "sasl_username", &obj->has_sasl_username)) {
        visit_type_str(v, "sasl_username", &obj->sasl_username, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_VncClientInfo(Visitor *v, const char *name, VncClientInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VncClientInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VncClientInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncClientInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncClientInfoList(Visitor *v, const char *name, VncClientInfoList **obj, Error **errp)
{
    Error *err = NULL;
    VncClientInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (VncClientInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_VncClientInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncClientInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncInfo_members(Visitor *v, VncInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_bool(v, "enabled", &obj->enabled, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "host", &obj->has_host)) {
        visit_type_str(v, "host", &obj->host, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "family", &obj->has_family)) {
        visit_type_NetworkAddressFamily(v, "family", &obj->family, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "service", &obj->has_service)) {
        visit_type_str(v, "service", &obj->service, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "auth", &obj->has_auth)) {
        visit_type_str(v, "auth", &obj->auth, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "clients", &obj->has_clients)) {
        visit_type_VncClientInfoList(v, "clients", &obj->clients, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_VncInfo(Visitor *v, const char *name, VncInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VncInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VncInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncPrimaryAuth(Visitor *v, const char *name, VncPrimaryAuth *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &VncPrimaryAuth_lookup, errp);
    *obj = value;
}

void visit_type_VncVencryptSubAuth(Visitor *v, const char *name, VncVencryptSubAuth *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &VncVencryptSubAuth_lookup, errp);
    *obj = value;
}

void visit_type_VncServerInfo2_members(Visitor *v, VncServerInfo2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VncBasicInfo_members(v, (VncBasicInfo *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_VncPrimaryAuth(v, "auth", &obj->auth, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "vencrypt", &obj->has_vencrypt)) {
        visit_type_VncVencryptSubAuth(v, "vencrypt", &obj->vencrypt, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_VncServerInfo2(Visitor *v, const char *name, VncServerInfo2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VncServerInfo2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VncServerInfo2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncServerInfo2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncServerInfo2List(Visitor *v, const char *name, VncServerInfo2List **obj, Error **errp)
{
    Error *err = NULL;
    VncServerInfo2List *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (VncServerInfo2List *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_VncServerInfo2(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncServerInfo2List(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncInfo2_members(Visitor *v, VncInfo2 *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_VncServerInfo2List(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_VncClientInfoList(v, "clients", &obj->clients, &err);
    if (err) {
        goto out;
    }
    visit_type_VncPrimaryAuth(v, "auth", &obj->auth, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "vencrypt", &obj->has_vencrypt)) {
        visit_type_VncVencryptSubAuth(v, "vencrypt", &obj->vencrypt, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "display", &obj->has_display)) {
        visit_type_str(v, "display", &obj->display, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_VncInfo2(Visitor *v, const char *name, VncInfo2 **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VncInfo2), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VncInfo2_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncInfo2(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VncInfo2List(Visitor *v, const char *name, VncInfo2List **obj, Error **errp)
{
    Error *err = NULL;
    VncInfo2List *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (VncInfo2List *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_VncInfo2(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VncInfo2List(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_change_vnc_password_arg_members(Visitor *v, q_obj_change_vnc_password_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "password", &obj->password, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_VNC_CONNECTED_arg_members(Visitor *v, q_obj_VNC_CONNECTED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VncServerInfo(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_VncBasicInfo(v, "client", &obj->client, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_VNC_INITIALIZED_arg_members(Visitor *v, q_obj_VNC_INITIALIZED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VncServerInfo(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_VncClientInfo(v, "client", &obj->client, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_VNC_DISCONNECTED_arg_members(Visitor *v, q_obj_VNC_DISCONNECTED_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VncServerInfo(v, "server", &obj->server, &err);
    if (err) {
        goto out;
    }
    visit_type_VncClientInfo(v, "client", &obj->client, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_MouseInfo_members(Visitor *v, MouseInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "index", &obj->index, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "current", &obj->current, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "absolute", &obj->absolute, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_MouseInfo(Visitor *v, const char *name, MouseInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(MouseInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_MouseInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MouseInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_MouseInfoList(Visitor *v, const char *name, MouseInfoList **obj, Error **errp)
{
    Error *err = NULL;
    MouseInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (MouseInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_MouseInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_MouseInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_QKeyCode(Visitor *v, const char *name, QKeyCode *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &QKeyCode_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_int_wrapper_members(Visitor *v, q_obj_int_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_int(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_QKeyCode_wrapper_members(Visitor *v, q_obj_QKeyCode_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_QKeyCode(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_KeyValueKind(Visitor *v, const char *name, KeyValueKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &KeyValueKind_lookup, errp);
    *obj = value;
}

void visit_type_KeyValue_members(Visitor *v, KeyValue *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_KeyValueKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case KEY_VALUE_KIND_NUMBER:
        visit_type_q_obj_int_wrapper_members(v, &obj->u.number, &err);
        break;
    case KEY_VALUE_KIND_QCODE:
        visit_type_q_obj_QKeyCode_wrapper_members(v, &obj->u.qcode, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_KeyValue(Visitor *v, const char *name, KeyValue **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(KeyValue), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_KeyValue_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_KeyValue(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_KeyValueList(Visitor *v, const char *name, KeyValueList **obj, Error **errp)
{
    Error *err = NULL;
    KeyValueList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (KeyValueList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_KeyValue(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_KeyValueList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_send_key_arg_members(Visitor *v, q_obj_send_key_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_KeyValueList(v, "keys", &obj->keys, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "hold-time", &obj->has_hold_time)) {
        visit_type_int(v, "hold-time", &obj->hold_time, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_InputButton(Visitor *v, const char *name, InputButton *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &InputButton_lookup, errp);
    *obj = value;
}

void visit_type_InputAxis(Visitor *v, const char *name, InputAxis *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &InputAxis_lookup, errp);
    *obj = value;
}

void visit_type_InputKeyEvent_members(Visitor *v, InputKeyEvent *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_KeyValue(v, "key", &obj->key, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "down", &obj->down, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_InputKeyEvent(Visitor *v, const char *name, InputKeyEvent **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(InputKeyEvent), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_InputKeyEvent_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InputKeyEvent(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_InputBtnEvent_members(Visitor *v, InputBtnEvent *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InputButton(v, "button", &obj->button, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "down", &obj->down, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_InputBtnEvent(Visitor *v, const char *name, InputBtnEvent **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(InputBtnEvent), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_InputBtnEvent_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InputBtnEvent(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_InputMoveEvent_members(Visitor *v, InputMoveEvent *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InputAxis(v, "axis", &obj->axis, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "value", &obj->value, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_InputMoveEvent(Visitor *v, const char *name, InputMoveEvent **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(InputMoveEvent), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_InputMoveEvent_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InputMoveEvent(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_InputKeyEvent_wrapper_members(Visitor *v, q_obj_InputKeyEvent_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InputKeyEvent(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_InputBtnEvent_wrapper_members(Visitor *v, q_obj_InputBtnEvent_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InputBtnEvent(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_InputMoveEvent_wrapper_members(Visitor *v, q_obj_InputMoveEvent_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InputMoveEvent(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_InputEventKind(Visitor *v, const char *name, InputEventKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &InputEventKind_lookup, errp);
    *obj = value;
}

void visit_type_InputEvent_members(Visitor *v, InputEvent *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InputEventKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case INPUT_EVENT_KIND_KEY:
        visit_type_q_obj_InputKeyEvent_wrapper_members(v, &obj->u.key, &err);
        break;
    case INPUT_EVENT_KIND_BTN:
        visit_type_q_obj_InputBtnEvent_wrapper_members(v, &obj->u.btn, &err);
        break;
    case INPUT_EVENT_KIND_REL:
        visit_type_q_obj_InputMoveEvent_wrapper_members(v, &obj->u.rel, &err);
        break;
    case INPUT_EVENT_KIND_ABS:
        visit_type_q_obj_InputMoveEvent_wrapper_members(v, &obj->u.abs, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_InputEvent(Visitor *v, const char *name, InputEvent **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(InputEvent), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_InputEvent_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InputEvent(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_InputEventList(Visitor *v, const char *name, InputEventList **obj, Error **errp)
{
    Error *err = NULL;
    InputEventList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (InputEventList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_InputEvent(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InputEventList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_input_send_event_arg_members(Visitor *v, q_obj_input_send_event_arg *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "device", &obj->has_device)) {
        visit_type_str(v, "device", &obj->device, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "head", &obj->has_head)) {
        visit_type_int(v, "head", &obj->head, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_InputEventList(v, "events", &obj->events, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_DisplayNoOpts_members(Visitor *v, DisplayNoOpts *obj, Error **errp)
{
    Error *err = NULL;

    error_propagate(errp, err);
}

void visit_type_DisplayNoOpts(Visitor *v, const char *name, DisplayNoOpts **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DisplayNoOpts), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DisplayNoOpts_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DisplayNoOpts(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DisplayGTK_members(Visitor *v, DisplayGTK *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "grab-on-hover", &obj->has_grab_on_hover)) {
        visit_type_bool(v, "grab-on-hover", &obj->grab_on_hover, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_DisplayGTK(Visitor *v, const char *name, DisplayGTK **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DisplayGTK), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DisplayGTK_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DisplayGTK(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DisplayType(Visitor *v, const char *name, DisplayType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &DisplayType_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_DisplayOptions_base_members(Visitor *v, q_obj_DisplayOptions_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_DisplayType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "full-screen", &obj->has_full_screen)) {
        visit_type_bool(v, "full-screen", &obj->full_screen, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "window-close", &obj->has_window_close)) {
        visit_type_bool(v, "window-close", &obj->window_close, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "gl", &obj->has_gl)) {
        visit_type_bool(v, "gl", &obj->gl, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_DisplayOptions_members(Visitor *v, DisplayOptions *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_DisplayOptions_base_members(v, (q_obj_DisplayOptions_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case DISPLAY_TYPE_DEFAULT:
        visit_type_DisplayNoOpts_members(v, &obj->u.q_default, &err);
        break;
    case DISPLAY_TYPE_NONE:
        visit_type_DisplayNoOpts_members(v, &obj->u.none, &err);
        break;
    case DISPLAY_TYPE_GTK:
        visit_type_DisplayGTK_members(v, &obj->u.gtk, &err);
        break;
    case DISPLAY_TYPE_SDL:
        visit_type_DisplayNoOpts_members(v, &obj->u.sdl, &err);
        break;
    case DISPLAY_TYPE_EGL_HEADLESS:
        visit_type_DisplayNoOpts_members(v, &obj->u.egl_headless, &err);
        break;
    case DISPLAY_TYPE_CURSES:
        visit_type_DisplayNoOpts_members(v, &obj->u.curses, &err);
        break;
    case DISPLAY_TYPE_COCOA:
        visit_type_DisplayNoOpts_members(v, &obj->u.cocoa, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_DisplayOptions(Visitor *v, const char *name, DisplayOptions **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(DisplayOptions), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_DisplayOptions_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_DisplayOptions(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_ui_c;

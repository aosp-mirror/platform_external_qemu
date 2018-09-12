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
#include "qapi-visit-sockets.h"

void visit_type_NetworkAddressFamily(Visitor *v, const char *name, NetworkAddressFamily *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &NetworkAddressFamily_lookup, errp);
    *obj = value;
}

void visit_type_InetSocketAddressBase_members(Visitor *v, InetSocketAddressBase *obj, Error **errp)
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

out:
    error_propagate(errp, err);
}

void visit_type_InetSocketAddressBase(Visitor *v, const char *name, InetSocketAddressBase **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(InetSocketAddressBase), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_InetSocketAddressBase_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InetSocketAddressBase(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_InetSocketAddress_members(Visitor *v, InetSocketAddress *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InetSocketAddressBase_members(v, (InetSocketAddressBase *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "numeric", &obj->has_numeric)) {
        visit_type_bool(v, "numeric", &obj->numeric, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "to", &obj->has_to)) {
        visit_type_uint16(v, "to", &obj->to, &err);
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

out:
    error_propagate(errp, err);
}

void visit_type_InetSocketAddress(Visitor *v, const char *name, InetSocketAddress **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(InetSocketAddress), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_InetSocketAddress_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_InetSocketAddress(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_UnixSocketAddress_members(Visitor *v, UnixSocketAddress *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "path", &obj->path, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_UnixSocketAddress(Visitor *v, const char *name, UnixSocketAddress **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(UnixSocketAddress), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_UnixSocketAddress_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_UnixSocketAddress(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_VsockSocketAddress_members(Visitor *v, VsockSocketAddress *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "cid", &obj->cid, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "port", &obj->port, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_VsockSocketAddress(Visitor *v, const char *name, VsockSocketAddress **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(VsockSocketAddress), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_VsockSocketAddress_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_VsockSocketAddress(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_InetSocketAddress_wrapper_members(Visitor *v, q_obj_InetSocketAddress_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_InetSocketAddress(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_UnixSocketAddress_wrapper_members(Visitor *v, q_obj_UnixSocketAddress_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_UnixSocketAddress(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_VsockSocketAddress_wrapper_members(Visitor *v, q_obj_VsockSocketAddress_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_VsockSocketAddress(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_String_wrapper_members(Visitor *v, q_obj_String_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_String(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SocketAddressLegacyKind(Visitor *v, const char *name, SocketAddressLegacyKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SocketAddressLegacyKind_lookup, errp);
    *obj = value;
}

void visit_type_SocketAddressLegacy_members(Visitor *v, SocketAddressLegacy *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SocketAddressLegacyKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case SOCKET_ADDRESS_LEGACY_KIND_INET:
        visit_type_q_obj_InetSocketAddress_wrapper_members(v, &obj->u.inet, &err);
        break;
    case SOCKET_ADDRESS_LEGACY_KIND_UNIX:
        visit_type_q_obj_UnixSocketAddress_wrapper_members(v, &obj->u.q_unix, &err);
        break;
    case SOCKET_ADDRESS_LEGACY_KIND_VSOCK:
        visit_type_q_obj_VsockSocketAddress_wrapper_members(v, &obj->u.vsock, &err);
        break;
    case SOCKET_ADDRESS_LEGACY_KIND_FD:
        visit_type_q_obj_String_wrapper_members(v, &obj->u.fd, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_SocketAddressLegacy(Visitor *v, const char *name, SocketAddressLegacy **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SocketAddressLegacy), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SocketAddressLegacy_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SocketAddressLegacy(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_SocketAddressType(Visitor *v, const char *name, SocketAddressType *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &SocketAddressType_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_SocketAddress_base_members(Visitor *v, q_obj_SocketAddress_base *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_SocketAddressType(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_SocketAddress_members(Visitor *v, SocketAddress *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_q_obj_SocketAddress_base_members(v, (q_obj_SocketAddress_base *)obj, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case SOCKET_ADDRESS_TYPE_INET:
        visit_type_InetSocketAddress_members(v, &obj->u.inet, &err);
        break;
    case SOCKET_ADDRESS_TYPE_UNIX:
        visit_type_UnixSocketAddress_members(v, &obj->u.q_unix, &err);
        break;
    case SOCKET_ADDRESS_TYPE_VSOCK:
        visit_type_VsockSocketAddress_members(v, &obj->u.vsock, &err);
        break;
    case SOCKET_ADDRESS_TYPE_FD:
        visit_type_String_members(v, &obj->u.fd, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_SocketAddress(Visitor *v, const char *name, SocketAddress **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(SocketAddress), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_SocketAddress_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_SocketAddress(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_sockets_c;

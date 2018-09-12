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
#include "qapi-types-sockets.h"
#include "qapi-visit-sockets.h"

const QEnumLookup NetworkAddressFamily_lookup = {
    .array = (const char *const[]) {
        [NETWORK_ADDRESS_FAMILY_IPV4] = "ipv4",
        [NETWORK_ADDRESS_FAMILY_IPV6] = "ipv6",
        [NETWORK_ADDRESS_FAMILY_UNIX] = "unix",
        [NETWORK_ADDRESS_FAMILY_VSOCK] = "vsock",
        [NETWORK_ADDRESS_FAMILY_UNKNOWN] = "unknown",
    },
    .size = NETWORK_ADDRESS_FAMILY__MAX
};

void qapi_free_InetSocketAddressBase(InetSocketAddressBase *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InetSocketAddressBase(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_InetSocketAddress(InetSocketAddress *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_InetSocketAddress(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_UnixSocketAddress(UnixSocketAddress *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_UnixSocketAddress(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_VsockSocketAddress(VsockSocketAddress *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_VsockSocketAddress(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup SocketAddressLegacyKind_lookup = {
    .array = (const char *const[]) {
        [SOCKET_ADDRESS_LEGACY_KIND_INET] = "inet",
        [SOCKET_ADDRESS_LEGACY_KIND_UNIX] = "unix",
        [SOCKET_ADDRESS_LEGACY_KIND_VSOCK] = "vsock",
        [SOCKET_ADDRESS_LEGACY_KIND_FD] = "fd",
    },
    .size = SOCKET_ADDRESS_LEGACY_KIND__MAX
};

void qapi_free_SocketAddressLegacy(SocketAddressLegacy *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SocketAddressLegacy(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup SocketAddressType_lookup = {
    .array = (const char *const[]) {
        [SOCKET_ADDRESS_TYPE_INET] = "inet",
        [SOCKET_ADDRESS_TYPE_UNIX] = "unix",
        [SOCKET_ADDRESS_TYPE_VSOCK] = "vsock",
        [SOCKET_ADDRESS_TYPE_FD] = "fd",
    },
    .size = SOCKET_ADDRESS_TYPE__MAX
};

void qapi_free_SocketAddress(SocketAddress *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_SocketAddress(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_sockets_c;

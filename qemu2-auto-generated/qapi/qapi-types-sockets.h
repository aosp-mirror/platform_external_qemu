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

#ifndef QAPI_TYPES_SOCKETS_H
#define QAPI_TYPES_SOCKETS_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-common.h"

typedef enum NetworkAddressFamily {
    NETWORK_ADDRESS_FAMILY_IPV4 = 0,
    NETWORK_ADDRESS_FAMILY_IPV6 = 1,
    NETWORK_ADDRESS_FAMILY_UNIX = 2,
    NETWORK_ADDRESS_FAMILY_VSOCK = 3,
    NETWORK_ADDRESS_FAMILY_UNKNOWN = 4,
    NETWORK_ADDRESS_FAMILY__MAX = 5,
} NetworkAddressFamily;

#define NetworkAddressFamily_str(val) \
    qapi_enum_lookup(&NetworkAddressFamily_lookup, (val))

extern const QEnumLookup NetworkAddressFamily_lookup;

typedef struct InetSocketAddressBase InetSocketAddressBase;

typedef struct InetSocketAddress InetSocketAddress;

typedef struct UnixSocketAddress UnixSocketAddress;

typedef struct VsockSocketAddress VsockSocketAddress;

typedef struct q_obj_InetSocketAddress_wrapper q_obj_InetSocketAddress_wrapper;

typedef struct q_obj_UnixSocketAddress_wrapper q_obj_UnixSocketAddress_wrapper;

typedef struct q_obj_VsockSocketAddress_wrapper q_obj_VsockSocketAddress_wrapper;

typedef struct q_obj_String_wrapper q_obj_String_wrapper;

typedef enum SocketAddressLegacyKind {
    SOCKET_ADDRESS_LEGACY_KIND_INET = 0,
    SOCKET_ADDRESS_LEGACY_KIND_UNIX = 1,
    SOCKET_ADDRESS_LEGACY_KIND_VSOCK = 2,
    SOCKET_ADDRESS_LEGACY_KIND_FD = 3,
    SOCKET_ADDRESS_LEGACY_KIND__MAX = 4,
} SocketAddressLegacyKind;

#define SocketAddressLegacyKind_str(val) \
    qapi_enum_lookup(&SocketAddressLegacyKind_lookup, (val))

extern const QEnumLookup SocketAddressLegacyKind_lookup;

typedef struct SocketAddressLegacy SocketAddressLegacy;

typedef enum SocketAddressType {
    SOCKET_ADDRESS_TYPE_INET = 0,
    SOCKET_ADDRESS_TYPE_UNIX = 1,
    SOCKET_ADDRESS_TYPE_VSOCK = 2,
    SOCKET_ADDRESS_TYPE_FD = 3,
    SOCKET_ADDRESS_TYPE__MAX = 4,
} SocketAddressType;

#define SocketAddressType_str(val) \
    qapi_enum_lookup(&SocketAddressType_lookup, (val))

extern const QEnumLookup SocketAddressType_lookup;

typedef struct q_obj_SocketAddress_base q_obj_SocketAddress_base;

typedef struct SocketAddress SocketAddress;

struct InetSocketAddressBase {
    char *host;
    char *port;
};

void qapi_free_InetSocketAddressBase(InetSocketAddressBase *obj);

struct InetSocketAddress {
    /* Members inherited from InetSocketAddressBase: */
    char *host;
    char *port;
    /* Own members: */
    bool has_numeric;
    bool numeric;
    bool has_to;
    uint16_t to;
    bool has_ipv4;
    bool ipv4;
    bool has_ipv6;
    bool ipv6;
};

static inline InetSocketAddressBase *qapi_InetSocketAddress_base(const InetSocketAddress *obj)
{
    return (InetSocketAddressBase *)obj;
}

void qapi_free_InetSocketAddress(InetSocketAddress *obj);

struct UnixSocketAddress {
    char *path;
};

void qapi_free_UnixSocketAddress(UnixSocketAddress *obj);

struct VsockSocketAddress {
    char *cid;
    char *port;
};

void qapi_free_VsockSocketAddress(VsockSocketAddress *obj);

struct q_obj_InetSocketAddress_wrapper {
    InetSocketAddress *data;
};

struct q_obj_UnixSocketAddress_wrapper {
    UnixSocketAddress *data;
};

struct q_obj_VsockSocketAddress_wrapper {
    VsockSocketAddress *data;
};

struct q_obj_String_wrapper {
    String *data;
};

struct SocketAddressLegacy {
    SocketAddressLegacyKind type;
    union { /* union tag is @type */
        q_obj_InetSocketAddress_wrapper inet;
        q_obj_UnixSocketAddress_wrapper q_unix;
        q_obj_VsockSocketAddress_wrapper vsock;
        q_obj_String_wrapper fd;
    } u;
};

void qapi_free_SocketAddressLegacy(SocketAddressLegacy *obj);

struct q_obj_SocketAddress_base {
    SocketAddressType type;
};

struct SocketAddress {
    SocketAddressType type;
    union { /* union tag is @type */
        InetSocketAddress inet;
        UnixSocketAddress q_unix;
        VsockSocketAddress vsock;
        String fd;
    } u;
};

void qapi_free_SocketAddress(SocketAddress *obj);

#endif /* QAPI_TYPES_SOCKETS_H */

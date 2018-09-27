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

#ifndef QAPI_VISIT_SOCKETS_H
#define QAPI_VISIT_SOCKETS_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-sockets.h"

#include "qapi-visit-common.h"
void visit_type_NetworkAddressFamily(Visitor *v, const char *name, NetworkAddressFamily *obj, Error **errp);

void visit_type_InetSocketAddressBase_members(Visitor *v, InetSocketAddressBase *obj, Error **errp);
void visit_type_InetSocketAddressBase(Visitor *v, const char *name, InetSocketAddressBase **obj, Error **errp);

void visit_type_InetSocketAddress_members(Visitor *v, InetSocketAddress *obj, Error **errp);
void visit_type_InetSocketAddress(Visitor *v, const char *name, InetSocketAddress **obj, Error **errp);

void visit_type_UnixSocketAddress_members(Visitor *v, UnixSocketAddress *obj, Error **errp);
void visit_type_UnixSocketAddress(Visitor *v, const char *name, UnixSocketAddress **obj, Error **errp);

void visit_type_VsockSocketAddress_members(Visitor *v, VsockSocketAddress *obj, Error **errp);
void visit_type_VsockSocketAddress(Visitor *v, const char *name, VsockSocketAddress **obj, Error **errp);

void visit_type_q_obj_InetSocketAddress_wrapper_members(Visitor *v, q_obj_InetSocketAddress_wrapper *obj, Error **errp);

void visit_type_q_obj_UnixSocketAddress_wrapper_members(Visitor *v, q_obj_UnixSocketAddress_wrapper *obj, Error **errp);

void visit_type_q_obj_VsockSocketAddress_wrapper_members(Visitor *v, q_obj_VsockSocketAddress_wrapper *obj, Error **errp);

void visit_type_q_obj_String_wrapper_members(Visitor *v, q_obj_String_wrapper *obj, Error **errp);
void visit_type_SocketAddressLegacyKind(Visitor *v, const char *name, SocketAddressLegacyKind *obj, Error **errp);

void visit_type_SocketAddressLegacy_members(Visitor *v, SocketAddressLegacy *obj, Error **errp);
void visit_type_SocketAddressLegacy(Visitor *v, const char *name, SocketAddressLegacy **obj, Error **errp);
void visit_type_SocketAddressType(Visitor *v, const char *name, SocketAddressType *obj, Error **errp);

void visit_type_q_obj_SocketAddress_base_members(Visitor *v, q_obj_SocketAddress_base *obj, Error **errp);

void visit_type_SocketAddress_members(Visitor *v, SocketAddress *obj, Error **errp);
void visit_type_SocketAddress(Visitor *v, const char *name, SocketAddress **obj, Error **errp);

#endif /* QAPI_VISIT_SOCKETS_H */

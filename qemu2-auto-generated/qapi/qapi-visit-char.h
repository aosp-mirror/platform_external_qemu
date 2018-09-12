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

#ifndef QAPI_VISIT_CHAR_H
#define QAPI_VISIT_CHAR_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-char.h"

#include "qapi-visit-sockets.h"

void visit_type_ChardevInfo_members(Visitor *v, ChardevInfo *obj, Error **errp);
void visit_type_ChardevInfo(Visitor *v, const char *name, ChardevInfo **obj, Error **errp);
void visit_type_ChardevInfoList(Visitor *v, const char *name, ChardevInfoList **obj, Error **errp);

void visit_type_ChardevBackendInfo_members(Visitor *v, ChardevBackendInfo *obj, Error **errp);
void visit_type_ChardevBackendInfo(Visitor *v, const char *name, ChardevBackendInfo **obj, Error **errp);
void visit_type_ChardevBackendInfoList(Visitor *v, const char *name, ChardevBackendInfoList **obj, Error **errp);
void visit_type_DataFormat(Visitor *v, const char *name, DataFormat *obj, Error **errp);

void visit_type_q_obj_ringbuf_write_arg_members(Visitor *v, q_obj_ringbuf_write_arg *obj, Error **errp);

void visit_type_q_obj_ringbuf_read_arg_members(Visitor *v, q_obj_ringbuf_read_arg *obj, Error **errp);

void visit_type_ChardevCommon_members(Visitor *v, ChardevCommon *obj, Error **errp);
void visit_type_ChardevCommon(Visitor *v, const char *name, ChardevCommon **obj, Error **errp);

void visit_type_ChardevFile_members(Visitor *v, ChardevFile *obj, Error **errp);
void visit_type_ChardevFile(Visitor *v, const char *name, ChardevFile **obj, Error **errp);

void visit_type_ChardevHostdev_members(Visitor *v, ChardevHostdev *obj, Error **errp);
void visit_type_ChardevHostdev(Visitor *v, const char *name, ChardevHostdev **obj, Error **errp);

void visit_type_ChardevSocket_members(Visitor *v, ChardevSocket *obj, Error **errp);
void visit_type_ChardevSocket(Visitor *v, const char *name, ChardevSocket **obj, Error **errp);

void visit_type_ChardevUdp_members(Visitor *v, ChardevUdp *obj, Error **errp);
void visit_type_ChardevUdp(Visitor *v, const char *name, ChardevUdp **obj, Error **errp);

void visit_type_ChardevMux_members(Visitor *v, ChardevMux *obj, Error **errp);
void visit_type_ChardevMux(Visitor *v, const char *name, ChardevMux **obj, Error **errp);

void visit_type_ChardevStdio_members(Visitor *v, ChardevStdio *obj, Error **errp);
void visit_type_ChardevStdio(Visitor *v, const char *name, ChardevStdio **obj, Error **errp);

void visit_type_ChardevSpiceChannel_members(Visitor *v, ChardevSpiceChannel *obj, Error **errp);
void visit_type_ChardevSpiceChannel(Visitor *v, const char *name, ChardevSpiceChannel **obj, Error **errp);

void visit_type_ChardevSpicePort_members(Visitor *v, ChardevSpicePort *obj, Error **errp);
void visit_type_ChardevSpicePort(Visitor *v, const char *name, ChardevSpicePort **obj, Error **errp);

void visit_type_ChardevVC_members(Visitor *v, ChardevVC *obj, Error **errp);
void visit_type_ChardevVC(Visitor *v, const char *name, ChardevVC **obj, Error **errp);

void visit_type_ChardevRingbuf_members(Visitor *v, ChardevRingbuf *obj, Error **errp);
void visit_type_ChardevRingbuf(Visitor *v, const char *name, ChardevRingbuf **obj, Error **errp);

void visit_type_q_obj_ChardevFile_wrapper_members(Visitor *v, q_obj_ChardevFile_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevHostdev_wrapper_members(Visitor *v, q_obj_ChardevHostdev_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevSocket_wrapper_members(Visitor *v, q_obj_ChardevSocket_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevUdp_wrapper_members(Visitor *v, q_obj_ChardevUdp_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevCommon_wrapper_members(Visitor *v, q_obj_ChardevCommon_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevMux_wrapper_members(Visitor *v, q_obj_ChardevMux_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevStdio_wrapper_members(Visitor *v, q_obj_ChardevStdio_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevSpiceChannel_wrapper_members(Visitor *v, q_obj_ChardevSpiceChannel_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevSpicePort_wrapper_members(Visitor *v, q_obj_ChardevSpicePort_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevVC_wrapper_members(Visitor *v, q_obj_ChardevVC_wrapper *obj, Error **errp);

void visit_type_q_obj_ChardevRingbuf_wrapper_members(Visitor *v, q_obj_ChardevRingbuf_wrapper *obj, Error **errp);
void visit_type_ChardevBackendKind(Visitor *v, const char *name, ChardevBackendKind *obj, Error **errp);

void visit_type_ChardevBackend_members(Visitor *v, ChardevBackend *obj, Error **errp);
void visit_type_ChardevBackend(Visitor *v, const char *name, ChardevBackend **obj, Error **errp);

void visit_type_ChardevReturn_members(Visitor *v, ChardevReturn *obj, Error **errp);
void visit_type_ChardevReturn(Visitor *v, const char *name, ChardevReturn **obj, Error **errp);

void visit_type_q_obj_chardev_add_arg_members(Visitor *v, q_obj_chardev_add_arg *obj, Error **errp);

void visit_type_q_obj_chardev_change_arg_members(Visitor *v, q_obj_chardev_change_arg *obj, Error **errp);

void visit_type_q_obj_chardev_remove_arg_members(Visitor *v, q_obj_chardev_remove_arg *obj, Error **errp);

void visit_type_q_obj_chardev_send_break_arg_members(Visitor *v, q_obj_chardev_send_break_arg *obj, Error **errp);

void visit_type_q_obj_VSERPORT_CHANGE_arg_members(Visitor *v, q_obj_VSERPORT_CHANGE_arg *obj, Error **errp);

#endif /* QAPI_VISIT_CHAR_H */

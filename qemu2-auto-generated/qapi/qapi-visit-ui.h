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

#ifndef QAPI_VISIT_UI_H
#define QAPI_VISIT_UI_H

#include "qapi/qapi-builtin-visit.h"
#include "qapi-types-ui.h"

#include "qapi-visit-sockets.h"

void visit_type_q_obj_set_password_arg_members(Visitor *v, q_obj_set_password_arg *obj, Error **errp);

void visit_type_q_obj_expire_password_arg_members(Visitor *v, q_obj_expire_password_arg *obj, Error **errp);

void visit_type_q_obj_screendump_arg_members(Visitor *v, q_obj_screendump_arg *obj, Error **errp);

void visit_type_SpiceBasicInfo_members(Visitor *v, SpiceBasicInfo *obj, Error **errp);
void visit_type_SpiceBasicInfo(Visitor *v, const char *name, SpiceBasicInfo **obj, Error **errp);

void visit_type_SpiceServerInfo_members(Visitor *v, SpiceServerInfo *obj, Error **errp);
void visit_type_SpiceServerInfo(Visitor *v, const char *name, SpiceServerInfo **obj, Error **errp);

void visit_type_SpiceChannel_members(Visitor *v, SpiceChannel *obj, Error **errp);
void visit_type_SpiceChannel(Visitor *v, const char *name, SpiceChannel **obj, Error **errp);
void visit_type_SpiceQueryMouseMode(Visitor *v, const char *name, SpiceQueryMouseMode *obj, Error **errp);
void visit_type_SpiceChannelList(Visitor *v, const char *name, SpiceChannelList **obj, Error **errp);

void visit_type_SpiceInfo_members(Visitor *v, SpiceInfo *obj, Error **errp);
void visit_type_SpiceInfo(Visitor *v, const char *name, SpiceInfo **obj, Error **errp);

void visit_type_q_obj_SPICE_CONNECTED_arg_members(Visitor *v, q_obj_SPICE_CONNECTED_arg *obj, Error **errp);

void visit_type_q_obj_SPICE_INITIALIZED_arg_members(Visitor *v, q_obj_SPICE_INITIALIZED_arg *obj, Error **errp);

void visit_type_q_obj_SPICE_DISCONNECTED_arg_members(Visitor *v, q_obj_SPICE_DISCONNECTED_arg *obj, Error **errp);

void visit_type_VncBasicInfo_members(Visitor *v, VncBasicInfo *obj, Error **errp);
void visit_type_VncBasicInfo(Visitor *v, const char *name, VncBasicInfo **obj, Error **errp);

void visit_type_VncServerInfo_members(Visitor *v, VncServerInfo *obj, Error **errp);
void visit_type_VncServerInfo(Visitor *v, const char *name, VncServerInfo **obj, Error **errp);

void visit_type_VncClientInfo_members(Visitor *v, VncClientInfo *obj, Error **errp);
void visit_type_VncClientInfo(Visitor *v, const char *name, VncClientInfo **obj, Error **errp);
void visit_type_VncClientInfoList(Visitor *v, const char *name, VncClientInfoList **obj, Error **errp);

void visit_type_VncInfo_members(Visitor *v, VncInfo *obj, Error **errp);
void visit_type_VncInfo(Visitor *v, const char *name, VncInfo **obj, Error **errp);
void visit_type_VncPrimaryAuth(Visitor *v, const char *name, VncPrimaryAuth *obj, Error **errp);
void visit_type_VncVencryptSubAuth(Visitor *v, const char *name, VncVencryptSubAuth *obj, Error **errp);

void visit_type_VncServerInfo2_members(Visitor *v, VncServerInfo2 *obj, Error **errp);
void visit_type_VncServerInfo2(Visitor *v, const char *name, VncServerInfo2 **obj, Error **errp);
void visit_type_VncServerInfo2List(Visitor *v, const char *name, VncServerInfo2List **obj, Error **errp);

void visit_type_VncInfo2_members(Visitor *v, VncInfo2 *obj, Error **errp);
void visit_type_VncInfo2(Visitor *v, const char *name, VncInfo2 **obj, Error **errp);
void visit_type_VncInfo2List(Visitor *v, const char *name, VncInfo2List **obj, Error **errp);

void visit_type_q_obj_change_vnc_password_arg_members(Visitor *v, q_obj_change_vnc_password_arg *obj, Error **errp);

void visit_type_q_obj_VNC_CONNECTED_arg_members(Visitor *v, q_obj_VNC_CONNECTED_arg *obj, Error **errp);

void visit_type_q_obj_VNC_INITIALIZED_arg_members(Visitor *v, q_obj_VNC_INITIALIZED_arg *obj, Error **errp);

void visit_type_q_obj_VNC_DISCONNECTED_arg_members(Visitor *v, q_obj_VNC_DISCONNECTED_arg *obj, Error **errp);

void visit_type_MouseInfo_members(Visitor *v, MouseInfo *obj, Error **errp);
void visit_type_MouseInfo(Visitor *v, const char *name, MouseInfo **obj, Error **errp);
void visit_type_MouseInfoList(Visitor *v, const char *name, MouseInfoList **obj, Error **errp);
void visit_type_QKeyCode(Visitor *v, const char *name, QKeyCode *obj, Error **errp);

void visit_type_q_obj_int_wrapper_members(Visitor *v, q_obj_int_wrapper *obj, Error **errp);

void visit_type_q_obj_QKeyCode_wrapper_members(Visitor *v, q_obj_QKeyCode_wrapper *obj, Error **errp);
void visit_type_KeyValueKind(Visitor *v, const char *name, KeyValueKind *obj, Error **errp);

void visit_type_KeyValue_members(Visitor *v, KeyValue *obj, Error **errp);
void visit_type_KeyValue(Visitor *v, const char *name, KeyValue **obj, Error **errp);
void visit_type_KeyValueList(Visitor *v, const char *name, KeyValueList **obj, Error **errp);

void visit_type_q_obj_send_key_arg_members(Visitor *v, q_obj_send_key_arg *obj, Error **errp);
void visit_type_InputButton(Visitor *v, const char *name, InputButton *obj, Error **errp);
void visit_type_InputAxis(Visitor *v, const char *name, InputAxis *obj, Error **errp);

void visit_type_InputKeyEvent_members(Visitor *v, InputKeyEvent *obj, Error **errp);
void visit_type_InputKeyEvent(Visitor *v, const char *name, InputKeyEvent **obj, Error **errp);

void visit_type_InputBtnEvent_members(Visitor *v, InputBtnEvent *obj, Error **errp);
void visit_type_InputBtnEvent(Visitor *v, const char *name, InputBtnEvent **obj, Error **errp);

void visit_type_InputMoveEvent_members(Visitor *v, InputMoveEvent *obj, Error **errp);
void visit_type_InputMoveEvent(Visitor *v, const char *name, InputMoveEvent **obj, Error **errp);

void visit_type_q_obj_InputKeyEvent_wrapper_members(Visitor *v, q_obj_InputKeyEvent_wrapper *obj, Error **errp);

void visit_type_q_obj_InputBtnEvent_wrapper_members(Visitor *v, q_obj_InputBtnEvent_wrapper *obj, Error **errp);

void visit_type_q_obj_InputMoveEvent_wrapper_members(Visitor *v, q_obj_InputMoveEvent_wrapper *obj, Error **errp);
void visit_type_InputEventKind(Visitor *v, const char *name, InputEventKind *obj, Error **errp);

void visit_type_InputEvent_members(Visitor *v, InputEvent *obj, Error **errp);
void visit_type_InputEvent(Visitor *v, const char *name, InputEvent **obj, Error **errp);
void visit_type_InputEventList(Visitor *v, const char *name, InputEventList **obj, Error **errp);

void visit_type_q_obj_input_send_event_arg_members(Visitor *v, q_obj_input_send_event_arg *obj, Error **errp);

void visit_type_DisplayNoOpts_members(Visitor *v, DisplayNoOpts *obj, Error **errp);
void visit_type_DisplayNoOpts(Visitor *v, const char *name, DisplayNoOpts **obj, Error **errp);

void visit_type_DisplayGTK_members(Visitor *v, DisplayGTK *obj, Error **errp);
void visit_type_DisplayGTK(Visitor *v, const char *name, DisplayGTK **obj, Error **errp);
void visit_type_DisplayType(Visitor *v, const char *name, DisplayType *obj, Error **errp);

void visit_type_q_obj_DisplayOptions_base_members(Visitor *v, q_obj_DisplayOptions_base *obj, Error **errp);

void visit_type_DisplayOptions_members(Visitor *v, DisplayOptions *obj, Error **errp);
void visit_type_DisplayOptions(Visitor *v, const char *name, DisplayOptions **obj, Error **errp);

#endif /* QAPI_VISIT_UI_H */

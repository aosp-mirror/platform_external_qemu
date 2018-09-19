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
#include "qapi-visit-char.h"

void visit_type_ChardevInfo_members(Visitor *v, ChardevInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "label", &obj->label, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "filename", &obj->filename, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "frontend-open", &obj->frontend_open, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevInfo(Visitor *v, const char *name, ChardevInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevInfoList(Visitor *v, const char *name, ChardevInfoList **obj, Error **errp)
{
    Error *err = NULL;
    ChardevInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (ChardevInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_ChardevInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevBackendInfo_members(Visitor *v, ChardevBackendInfo *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "name", &obj->name, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevBackendInfo(Visitor *v, const char *name, ChardevBackendInfo **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevBackendInfo), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevBackendInfo_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevBackendInfo(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevBackendInfoList(Visitor *v, const char *name, ChardevBackendInfoList **obj, Error **errp)
{
    Error *err = NULL;
    ChardevBackendInfoList *tail;
    size_t size = sizeof(**obj);

    visit_start_list(v, name, (GenericList **)obj, size, &err);
    if (err) {
        goto out;
    }

    for (tail = *obj; tail;
         tail = (ChardevBackendInfoList *)visit_next_list(v, (GenericList *)tail, size)) {
        visit_type_ChardevBackendInfo(v, NULL, &tail->value, &err);
        if (err) {
            break;
        }
    }

    if (!err) {
        visit_check_list(v, &err);
    }
    visit_end_list(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevBackendInfoList(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_DataFormat(Visitor *v, const char *name, DataFormat *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &DataFormat_lookup, errp);
    *obj = value;
}

void visit_type_q_obj_ringbuf_write_arg_members(Visitor *v, q_obj_ringbuf_write_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "format", &obj->has_format)) {
        visit_type_DataFormat(v, "format", &obj->format, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ringbuf_read_arg_members(Visitor *v, q_obj_ringbuf_read_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }
    visit_type_int(v, "size", &obj->size, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "format", &obj->has_format)) {
        visit_type_DataFormat(v, "format", &obj->format, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevCommon_members(Visitor *v, ChardevCommon *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "logfile", &obj->has_logfile)) {
        visit_type_str(v, "logfile", &obj->logfile, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "logappend", &obj->has_logappend)) {
        visit_type_bool(v, "logappend", &obj->logappend, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevCommon(Visitor *v, const char *name, ChardevCommon **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevCommon), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevCommon_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevCommon(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevFile_members(Visitor *v, ChardevFile *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "in", &obj->has_in)) {
        visit_type_str(v, "in", &obj->in, &err);
        if (err) {
            goto out;
        }
    }
    visit_type_str(v, "out", &obj->out, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "append", &obj->has_append)) {
        visit_type_bool(v, "append", &obj->append, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevFile(Visitor *v, const char *name, ChardevFile **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevFile), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevFile_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevFile(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevHostdev_members(Visitor *v, ChardevHostdev *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "device", &obj->device, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevHostdev(Visitor *v, const char *name, ChardevHostdev **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevHostdev), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevHostdev_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevHostdev(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevSocket_members(Visitor *v, ChardevSocket *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_SocketAddressLegacy(v, "addr", &obj->addr, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "tls-creds", &obj->has_tls_creds)) {
        visit_type_str(v, "tls-creds", &obj->tls_creds, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "server", &obj->has_server)) {
        visit_type_bool(v, "server", &obj->server, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "wait", &obj->has_wait)) {
        visit_type_bool(v, "wait", &obj->wait, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "nodelay", &obj->has_nodelay)) {
        visit_type_bool(v, "nodelay", &obj->nodelay, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "telnet", &obj->has_telnet)) {
        visit_type_bool(v, "telnet", &obj->telnet, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "tn3270", &obj->has_tn3270)) {
        visit_type_bool(v, "tn3270", &obj->tn3270, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "reconnect", &obj->has_reconnect)) {
        visit_type_int(v, "reconnect", &obj->reconnect, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevSocket(Visitor *v, const char *name, ChardevSocket **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevSocket), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevSocket_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevSocket(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevUdp_members(Visitor *v, ChardevUdp *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_SocketAddressLegacy(v, "remote", &obj->remote, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "local", &obj->has_local)) {
        visit_type_SocketAddressLegacy(v, "local", &obj->local, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevUdp(Visitor *v, const char *name, ChardevUdp **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevUdp), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevUdp_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevUdp(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevMux_members(Visitor *v, ChardevMux *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "chardev", &obj->chardev, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevMux(Visitor *v, const char *name, ChardevMux **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevMux), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevMux_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevMux(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevStdio_members(Visitor *v, ChardevStdio *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "signal", &obj->has_signal)) {
        visit_type_bool(v, "signal", &obj->signal, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevStdio(Visitor *v, const char *name, ChardevStdio **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevStdio), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevStdio_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevStdio(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevSpiceChannel_members(Visitor *v, ChardevSpiceChannel *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevSpiceChannel(Visitor *v, const char *name, ChardevSpiceChannel **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevSpiceChannel), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevSpiceChannel_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevSpiceChannel(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevSpicePort_members(Visitor *v, ChardevSpicePort *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    visit_type_str(v, "fqdn", &obj->fqdn, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevSpicePort(Visitor *v, const char *name, ChardevSpicePort **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevSpicePort), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevSpicePort_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevSpicePort(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevVC_members(Visitor *v, ChardevVC *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "width", &obj->has_width)) {
        visit_type_int(v, "width", &obj->width, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "height", &obj->has_height)) {
        visit_type_int(v, "height", &obj->height, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "cols", &obj->has_cols)) {
        visit_type_int(v, "cols", &obj->cols, &err);
        if (err) {
            goto out;
        }
    }
    if (visit_optional(v, "rows", &obj->has_rows)) {
        visit_type_int(v, "rows", &obj->rows, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevVC(Visitor *v, const char *name, ChardevVC **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevVC), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevVC_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevVC(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevRingbuf_members(Visitor *v, ChardevRingbuf *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon_members(v, (ChardevCommon *)obj, &err);
    if (err) {
        goto out;
    }
    if (visit_optional(v, "size", &obj->has_size)) {
        visit_type_int(v, "size", &obj->size, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevRingbuf(Visitor *v, const char *name, ChardevRingbuf **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevRingbuf), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevRingbuf_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevRingbuf(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevFile_wrapper_members(Visitor *v, q_obj_ChardevFile_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevFile(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevHostdev_wrapper_members(Visitor *v, q_obj_ChardevHostdev_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevHostdev(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevSocket_wrapper_members(Visitor *v, q_obj_ChardevSocket_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevSocket(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevUdp_wrapper_members(Visitor *v, q_obj_ChardevUdp_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevUdp(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevCommon_wrapper_members(Visitor *v, q_obj_ChardevCommon_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevCommon(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevMux_wrapper_members(Visitor *v, q_obj_ChardevMux_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevMux(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevStdio_wrapper_members(Visitor *v, q_obj_ChardevStdio_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevStdio(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevSpiceChannel_wrapper_members(Visitor *v, q_obj_ChardevSpiceChannel_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevSpiceChannel(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevSpicePort_wrapper_members(Visitor *v, q_obj_ChardevSpicePort_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevSpicePort(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevVC_wrapper_members(Visitor *v, q_obj_ChardevVC_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevVC(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_ChardevRingbuf_wrapper_members(Visitor *v, q_obj_ChardevRingbuf_wrapper *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevRingbuf(v, "data", &obj->data, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevBackendKind(Visitor *v, const char *name, ChardevBackendKind *obj, Error **errp)
{
    int value = *obj;
    visit_type_enum(v, name, &value, &ChardevBackendKind_lookup, errp);
    *obj = value;
}

void visit_type_ChardevBackend_members(Visitor *v, ChardevBackend *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_ChardevBackendKind(v, "type", &obj->type, &err);
    if (err) {
        goto out;
    }
    switch (obj->type) {
    case CHARDEV_BACKEND_KIND_FILE:
        visit_type_q_obj_ChardevFile_wrapper_members(v, &obj->u.file, &err);
        break;
    case CHARDEV_BACKEND_KIND_SERIAL:
        visit_type_q_obj_ChardevHostdev_wrapper_members(v, &obj->u.serial, &err);
        break;
    case CHARDEV_BACKEND_KIND_PARALLEL:
        visit_type_q_obj_ChardevHostdev_wrapper_members(v, &obj->u.parallel, &err);
        break;
    case CHARDEV_BACKEND_KIND_PIPE:
        visit_type_q_obj_ChardevHostdev_wrapper_members(v, &obj->u.pipe, &err);
        break;
    case CHARDEV_BACKEND_KIND_SOCKET:
        visit_type_q_obj_ChardevSocket_wrapper_members(v, &obj->u.socket, &err);
        break;
    case CHARDEV_BACKEND_KIND_UDP:
        visit_type_q_obj_ChardevUdp_wrapper_members(v, &obj->u.udp, &err);
        break;
    case CHARDEV_BACKEND_KIND_PTY:
        visit_type_q_obj_ChardevCommon_wrapper_members(v, &obj->u.pty, &err);
        break;
    case CHARDEV_BACKEND_KIND_NULL:
        visit_type_q_obj_ChardevCommon_wrapper_members(v, &obj->u.null, &err);
        break;
    case CHARDEV_BACKEND_KIND_MUX:
        visit_type_q_obj_ChardevMux_wrapper_members(v, &obj->u.mux, &err);
        break;
    case CHARDEV_BACKEND_KIND_MSMOUSE:
        visit_type_q_obj_ChardevCommon_wrapper_members(v, &obj->u.msmouse, &err);
        break;
    case CHARDEV_BACKEND_KIND_WCTABLET:
        visit_type_q_obj_ChardevCommon_wrapper_members(v, &obj->u.wctablet, &err);
        break;
    case CHARDEV_BACKEND_KIND_BRAILLE:
        visit_type_q_obj_ChardevCommon_wrapper_members(v, &obj->u.braille, &err);
        break;
    case CHARDEV_BACKEND_KIND_TESTDEV:
        visit_type_q_obj_ChardevCommon_wrapper_members(v, &obj->u.testdev, &err);
        break;
    case CHARDEV_BACKEND_KIND_STDIO:
        visit_type_q_obj_ChardevStdio_wrapper_members(v, &obj->u.stdio, &err);
        break;
    case CHARDEV_BACKEND_KIND_CONSOLE:
        visit_type_q_obj_ChardevCommon_wrapper_members(v, &obj->u.console, &err);
        break;
    case CHARDEV_BACKEND_KIND_SPICEVMC:
        visit_type_q_obj_ChardevSpiceChannel_wrapper_members(v, &obj->u.spicevmc, &err);
        break;
    case CHARDEV_BACKEND_KIND_SPICEPORT:
        visit_type_q_obj_ChardevSpicePort_wrapper_members(v, &obj->u.spiceport, &err);
        break;
    case CHARDEV_BACKEND_KIND_VC:
        visit_type_q_obj_ChardevVC_wrapper_members(v, &obj->u.vc, &err);
        break;
    case CHARDEV_BACKEND_KIND_RINGBUF:
        visit_type_q_obj_ChardevRingbuf_wrapper_members(v, &obj->u.ringbuf, &err);
        break;
    case CHARDEV_BACKEND_KIND_MEMORY:
        visit_type_q_obj_ChardevRingbuf_wrapper_members(v, &obj->u.memory, &err);
        break;
    default:
        abort();
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevBackend(Visitor *v, const char *name, ChardevBackend **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevBackend), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevBackend_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevBackend(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_ChardevReturn_members(Visitor *v, ChardevReturn *obj, Error **errp)
{
    Error *err = NULL;

    if (visit_optional(v, "pty", &obj->has_pty)) {
        visit_type_str(v, "pty", &obj->pty, &err);
        if (err) {
            goto out;
        }
    }

out:
    error_propagate(errp, err);
}

void visit_type_ChardevReturn(Visitor *v, const char *name, ChardevReturn **obj, Error **errp)
{
    Error *err = NULL;

    visit_start_struct(v, name, (void **)obj, sizeof(ChardevReturn), &err);
    if (err) {
        goto out;
    }
    if (!*obj) {
        goto out_obj;
    }
    visit_type_ChardevReturn_members(v, *obj, &err);
    if (err) {
        goto out_obj;
    }
    visit_check_struct(v, &err);
out_obj:
    visit_end_struct(v, (void **)obj);
    if (err && visit_is_input(v)) {
        qapi_free_ChardevReturn(*obj);
        *obj = NULL;
    }
out:
    error_propagate(errp, err);
}

void visit_type_q_obj_chardev_add_arg_members(Visitor *v, q_obj_chardev_add_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_ChardevBackend(v, "backend", &obj->backend, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_chardev_change_arg_members(Visitor *v, q_obj_chardev_change_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_ChardevBackend(v, "backend", &obj->backend, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_chardev_remove_arg_members(Visitor *v, q_obj_chardev_remove_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_chardev_send_break_arg_members(Visitor *v, q_obj_chardev_send_break_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}

void visit_type_q_obj_VSERPORT_CHANGE_arg_members(Visitor *v, q_obj_VSERPORT_CHANGE_arg *obj, Error **errp)
{
    Error *err = NULL;

    visit_type_str(v, "id", &obj->id, &err);
    if (err) {
        goto out;
    }
    visit_type_bool(v, "open", &obj->open, &err);
    if (err) {
        goto out;
    }

out:
    error_propagate(errp, err);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_visit_char_c;

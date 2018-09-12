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
#include "qapi-types-char.h"
#include "qapi-visit-char.h"

void qapi_free_ChardevInfo(ChardevInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevInfoList(ChardevInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevBackendInfo(ChardevBackendInfo *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevBackendInfo(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevBackendInfoList(ChardevBackendInfoList *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevBackendInfoList(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup DataFormat_lookup = {
    .array = (const char *const[]) {
        [DATA_FORMAT_UTF8] = "utf8",
        [DATA_FORMAT_BASE64] = "base64",
    },
    .size = DATA_FORMAT__MAX
};

void qapi_free_ChardevCommon(ChardevCommon *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevCommon(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevFile(ChardevFile *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevFile(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevHostdev(ChardevHostdev *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevHostdev(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevSocket(ChardevSocket *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevSocket(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevUdp(ChardevUdp *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevUdp(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevMux(ChardevMux *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevMux(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevStdio(ChardevStdio *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevStdio(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevSpiceChannel(ChardevSpiceChannel *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevSpiceChannel(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevSpicePort(ChardevSpicePort *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevSpicePort(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevVC(ChardevVC *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevVC(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevRingbuf(ChardevRingbuf *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevRingbuf(v, NULL, &obj, NULL);
    visit_free(v);
}

const QEnumLookup ChardevBackendKind_lookup = {
    .array = (const char *const[]) {
        [CHARDEV_BACKEND_KIND_FILE] = "file",
        [CHARDEV_BACKEND_KIND_SERIAL] = "serial",
        [CHARDEV_BACKEND_KIND_PARALLEL] = "parallel",
        [CHARDEV_BACKEND_KIND_PIPE] = "pipe",
        [CHARDEV_BACKEND_KIND_SOCKET] = "socket",
        [CHARDEV_BACKEND_KIND_UDP] = "udp",
        [CHARDEV_BACKEND_KIND_PTY] = "pty",
        [CHARDEV_BACKEND_KIND_NULL] = "null",
        [CHARDEV_BACKEND_KIND_MUX] = "mux",
        [CHARDEV_BACKEND_KIND_MSMOUSE] = "msmouse",
        [CHARDEV_BACKEND_KIND_WCTABLET] = "wctablet",
        [CHARDEV_BACKEND_KIND_BRAILLE] = "braille",
        [CHARDEV_BACKEND_KIND_TESTDEV] = "testdev",
        [CHARDEV_BACKEND_KIND_STDIO] = "stdio",
        [CHARDEV_BACKEND_KIND_CONSOLE] = "console",
        [CHARDEV_BACKEND_KIND_SPICEVMC] = "spicevmc",
        [CHARDEV_BACKEND_KIND_SPICEPORT] = "spiceport",
        [CHARDEV_BACKEND_KIND_VC] = "vc",
        [CHARDEV_BACKEND_KIND_RINGBUF] = "ringbuf",
        [CHARDEV_BACKEND_KIND_MEMORY] = "memory",
    },
    .size = CHARDEV_BACKEND_KIND__MAX
};

void qapi_free_ChardevBackend(ChardevBackend *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevBackend(v, NULL, &obj, NULL);
    visit_free(v);
}

void qapi_free_ChardevReturn(ChardevReturn *obj)
{
    Visitor *v;

    if (!obj) {
        return;
    }

    v = qapi_dealloc_visitor_new();
    visit_type_ChardevReturn(v, NULL, &obj, NULL);
    visit_free(v);
}
/* Dummy declaration to prevent empty .o file */
char dummy_qapi_types_char_c;

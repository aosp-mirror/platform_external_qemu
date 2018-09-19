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

#ifndef QAPI_TYPES_CHAR_H
#define QAPI_TYPES_CHAR_H

#include "qapi/qapi-builtin-types.h"
#include "qapi-types-sockets.h"

typedef struct ChardevInfo ChardevInfo;

typedef struct ChardevInfoList ChardevInfoList;

typedef struct ChardevBackendInfo ChardevBackendInfo;

typedef struct ChardevBackendInfoList ChardevBackendInfoList;

typedef enum DataFormat {
    DATA_FORMAT_UTF8 = 0,
    DATA_FORMAT_BASE64 = 1,
    DATA_FORMAT__MAX = 2,
} DataFormat;

#define DataFormat_str(val) \
    qapi_enum_lookup(&DataFormat_lookup, (val))

extern const QEnumLookup DataFormat_lookup;

typedef struct q_obj_ringbuf_write_arg q_obj_ringbuf_write_arg;

typedef struct q_obj_ringbuf_read_arg q_obj_ringbuf_read_arg;

typedef struct ChardevCommon ChardevCommon;

typedef struct ChardevFile ChardevFile;

typedef struct ChardevHostdev ChardevHostdev;

typedef struct ChardevSocket ChardevSocket;

typedef struct ChardevUdp ChardevUdp;

typedef struct ChardevMux ChardevMux;

typedef struct ChardevStdio ChardevStdio;

typedef struct ChardevSpiceChannel ChardevSpiceChannel;

typedef struct ChardevSpicePort ChardevSpicePort;

typedef struct ChardevVC ChardevVC;

typedef struct ChardevRingbuf ChardevRingbuf;

typedef struct q_obj_ChardevFile_wrapper q_obj_ChardevFile_wrapper;

typedef struct q_obj_ChardevHostdev_wrapper q_obj_ChardevHostdev_wrapper;

typedef struct q_obj_ChardevSocket_wrapper q_obj_ChardevSocket_wrapper;

typedef struct q_obj_ChardevUdp_wrapper q_obj_ChardevUdp_wrapper;

typedef struct q_obj_ChardevCommon_wrapper q_obj_ChardevCommon_wrapper;

typedef struct q_obj_ChardevMux_wrapper q_obj_ChardevMux_wrapper;

typedef struct q_obj_ChardevStdio_wrapper q_obj_ChardevStdio_wrapper;

typedef struct q_obj_ChardevSpiceChannel_wrapper q_obj_ChardevSpiceChannel_wrapper;

typedef struct q_obj_ChardevSpicePort_wrapper q_obj_ChardevSpicePort_wrapper;

typedef struct q_obj_ChardevVC_wrapper q_obj_ChardevVC_wrapper;

typedef struct q_obj_ChardevRingbuf_wrapper q_obj_ChardevRingbuf_wrapper;

typedef enum ChardevBackendKind {
    CHARDEV_BACKEND_KIND_FILE = 0,
    CHARDEV_BACKEND_KIND_SERIAL = 1,
    CHARDEV_BACKEND_KIND_PARALLEL = 2,
    CHARDEV_BACKEND_KIND_PIPE = 3,
    CHARDEV_BACKEND_KIND_SOCKET = 4,
    CHARDEV_BACKEND_KIND_UDP = 5,
    CHARDEV_BACKEND_KIND_PTY = 6,
    CHARDEV_BACKEND_KIND_NULL = 7,
    CHARDEV_BACKEND_KIND_MUX = 8,
    CHARDEV_BACKEND_KIND_MSMOUSE = 9,
    CHARDEV_BACKEND_KIND_WCTABLET = 10,
    CHARDEV_BACKEND_KIND_BRAILLE = 11,
    CHARDEV_BACKEND_KIND_TESTDEV = 12,
    CHARDEV_BACKEND_KIND_STDIO = 13,
    CHARDEV_BACKEND_KIND_CONSOLE = 14,
    CHARDEV_BACKEND_KIND_SPICEVMC = 15,
    CHARDEV_BACKEND_KIND_SPICEPORT = 16,
    CHARDEV_BACKEND_KIND_VC = 17,
    CHARDEV_BACKEND_KIND_RINGBUF = 18,
    CHARDEV_BACKEND_KIND_MEMORY = 19,
    CHARDEV_BACKEND_KIND__MAX = 20,
} ChardevBackendKind;

#define ChardevBackendKind_str(val) \
    qapi_enum_lookup(&ChardevBackendKind_lookup, (val))

extern const QEnumLookup ChardevBackendKind_lookup;

typedef struct ChardevBackend ChardevBackend;

typedef struct ChardevReturn ChardevReturn;

typedef struct q_obj_chardev_add_arg q_obj_chardev_add_arg;

typedef struct q_obj_chardev_change_arg q_obj_chardev_change_arg;

typedef struct q_obj_chardev_remove_arg q_obj_chardev_remove_arg;

typedef struct q_obj_chardev_send_break_arg q_obj_chardev_send_break_arg;

typedef struct q_obj_VSERPORT_CHANGE_arg q_obj_VSERPORT_CHANGE_arg;

struct ChardevInfo {
    char *label;
    char *filename;
    bool frontend_open;
};

void qapi_free_ChardevInfo(ChardevInfo *obj);

struct ChardevInfoList {
    ChardevInfoList *next;
    ChardevInfo *value;
};

void qapi_free_ChardevInfoList(ChardevInfoList *obj);

struct ChardevBackendInfo {
    char *name;
};

void qapi_free_ChardevBackendInfo(ChardevBackendInfo *obj);

struct ChardevBackendInfoList {
    ChardevBackendInfoList *next;
    ChardevBackendInfo *value;
};

void qapi_free_ChardevBackendInfoList(ChardevBackendInfoList *obj);

struct q_obj_ringbuf_write_arg {
    char *device;
    char *data;
    bool has_format;
    DataFormat format;
};

struct q_obj_ringbuf_read_arg {
    char *device;
    int64_t size;
    bool has_format;
    DataFormat format;
};

struct ChardevCommon {
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
};

void qapi_free_ChardevCommon(ChardevCommon *obj);

struct ChardevFile {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    bool has_in;
    char *in;
    char *out;
    bool has_append;
    bool append;
};

static inline ChardevCommon *qapi_ChardevFile_base(const ChardevFile *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevFile(ChardevFile *obj);

struct ChardevHostdev {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    char *device;
};

static inline ChardevCommon *qapi_ChardevHostdev_base(const ChardevHostdev *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevHostdev(ChardevHostdev *obj);

struct ChardevSocket {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    SocketAddressLegacy *addr;
    bool has_tls_creds;
    char *tls_creds;
    bool has_server;
    bool server;
    bool has_wait;
    bool wait;
    bool has_nodelay;
    bool nodelay;
    bool has_telnet;
    bool telnet;
    bool has_tn3270;
    bool tn3270;
    bool has_reconnect;
    int64_t reconnect;
};

static inline ChardevCommon *qapi_ChardevSocket_base(const ChardevSocket *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevSocket(ChardevSocket *obj);

struct ChardevUdp {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    SocketAddressLegacy *remote;
    bool has_local;
    SocketAddressLegacy *local;
};

static inline ChardevCommon *qapi_ChardevUdp_base(const ChardevUdp *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevUdp(ChardevUdp *obj);

struct ChardevMux {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    char *chardev;
};

static inline ChardevCommon *qapi_ChardevMux_base(const ChardevMux *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevMux(ChardevMux *obj);

struct ChardevStdio {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    bool has_signal;
    bool signal;
};

static inline ChardevCommon *qapi_ChardevStdio_base(const ChardevStdio *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevStdio(ChardevStdio *obj);

struct ChardevSpiceChannel {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    char *type;
};

static inline ChardevCommon *qapi_ChardevSpiceChannel_base(const ChardevSpiceChannel *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevSpiceChannel(ChardevSpiceChannel *obj);

struct ChardevSpicePort {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    char *fqdn;
};

static inline ChardevCommon *qapi_ChardevSpicePort_base(const ChardevSpicePort *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevSpicePort(ChardevSpicePort *obj);

struct ChardevVC {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    bool has_width;
    int64_t width;
    bool has_height;
    int64_t height;
    bool has_cols;
    int64_t cols;
    bool has_rows;
    int64_t rows;
};

static inline ChardevCommon *qapi_ChardevVC_base(const ChardevVC *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevVC(ChardevVC *obj);

struct ChardevRingbuf {
    /* Members inherited from ChardevCommon: */
    bool has_logfile;
    char *logfile;
    bool has_logappend;
    bool logappend;
    /* Own members: */
    bool has_size;
    int64_t size;
};

static inline ChardevCommon *qapi_ChardevRingbuf_base(const ChardevRingbuf *obj)
{
    return (ChardevCommon *)obj;
}

void qapi_free_ChardevRingbuf(ChardevRingbuf *obj);

struct q_obj_ChardevFile_wrapper {
    ChardevFile *data;
};

struct q_obj_ChardevHostdev_wrapper {
    ChardevHostdev *data;
};

struct q_obj_ChardevSocket_wrapper {
    ChardevSocket *data;
};

struct q_obj_ChardevUdp_wrapper {
    ChardevUdp *data;
};

struct q_obj_ChardevCommon_wrapper {
    ChardevCommon *data;
};

struct q_obj_ChardevMux_wrapper {
    ChardevMux *data;
};

struct q_obj_ChardevStdio_wrapper {
    ChardevStdio *data;
};

struct q_obj_ChardevSpiceChannel_wrapper {
    ChardevSpiceChannel *data;
};

struct q_obj_ChardevSpicePort_wrapper {
    ChardevSpicePort *data;
};

struct q_obj_ChardevVC_wrapper {
    ChardevVC *data;
};

struct q_obj_ChardevRingbuf_wrapper {
    ChardevRingbuf *data;
};

struct ChardevBackend {
    ChardevBackendKind type;
    union { /* union tag is @type */
        q_obj_ChardevFile_wrapper file;
        q_obj_ChardevHostdev_wrapper serial;
        q_obj_ChardevHostdev_wrapper parallel;
        q_obj_ChardevHostdev_wrapper pipe;
        q_obj_ChardevSocket_wrapper socket;
        q_obj_ChardevUdp_wrapper udp;
        q_obj_ChardevCommon_wrapper pty;
        q_obj_ChardevCommon_wrapper null;
        q_obj_ChardevMux_wrapper mux;
        q_obj_ChardevCommon_wrapper msmouse;
        q_obj_ChardevCommon_wrapper wctablet;
        q_obj_ChardevCommon_wrapper braille;
        q_obj_ChardevCommon_wrapper testdev;
        q_obj_ChardevStdio_wrapper stdio;
        q_obj_ChardevCommon_wrapper console;
        q_obj_ChardevSpiceChannel_wrapper spicevmc;
        q_obj_ChardevSpicePort_wrapper spiceport;
        q_obj_ChardevVC_wrapper vc;
        q_obj_ChardevRingbuf_wrapper ringbuf;
        q_obj_ChardevRingbuf_wrapper memory;
    } u;
};

void qapi_free_ChardevBackend(ChardevBackend *obj);

struct ChardevReturn {
    bool has_pty;
    char *pty;
};

void qapi_free_ChardevReturn(ChardevReturn *obj);

struct q_obj_chardev_add_arg {
    char *id;
    ChardevBackend *backend;
};

struct q_obj_chardev_change_arg {
    char *id;
    ChardevBackend *backend;
};

struct q_obj_chardev_remove_arg {
    char *id;
};

struct q_obj_chardev_send_break_arg {
    char *id;
};

struct q_obj_VSERPORT_CHANGE_arg {
    char *id;
    bool open;
};

#endif /* QAPI_TYPES_CHAR_H */

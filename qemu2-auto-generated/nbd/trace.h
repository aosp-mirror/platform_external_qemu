/* This file is autogenerated by tracetool, do not edit. */

#ifndef TRACE_NBD_GENERATED_TRACERS_H
#define TRACE_NBD_GENERATED_TRACERS_H

#include "qemu-common.h"
#include "trace/control.h"

extern TraceEvent _TRACE_NBD_SEND_OPTION_REQUEST_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_OPTION_REPLY_EVENT;
extern TraceEvent _TRACE_NBD_REPLY_ERR_UNSUP_EVENT;
extern TraceEvent _TRACE_NBD_OPT_GO_START_EVENT;
extern TraceEvent _TRACE_NBD_OPT_GO_SUCCESS_EVENT;
extern TraceEvent _TRACE_NBD_OPT_GO_INFO_UNKNOWN_EVENT;
extern TraceEvent _TRACE_NBD_OPT_GO_INFO_BLOCK_SIZE_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_QUERY_EXPORTS_START_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_QUERY_EXPORTS_SUCCESS_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_STARTTLS_NEW_CLIENT_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_STARTTLS_TLS_HANDSHAKE_EVENT;
extern TraceEvent _TRACE_NBD_OPT_META_REQUEST_EVENT;
extern TraceEvent _TRACE_NBD_OPT_META_REPLY_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_NEGOTIATE_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_NEGOTIATE_MAGIC_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_NEGOTIATE_SERVER_FLAGS_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_NEGOTIATE_DEFAULT_NAME_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_NEGOTIATE_SIZE_FLAGS_EVENT;
extern TraceEvent _TRACE_NBD_INIT_SET_SOCKET_EVENT;
extern TraceEvent _TRACE_NBD_INIT_SET_BLOCK_SIZE_EVENT;
extern TraceEvent _TRACE_NBD_INIT_SET_SIZE_EVENT;
extern TraceEvent _TRACE_NBD_INIT_TRAILING_BYTES_EVENT;
extern TraceEvent _TRACE_NBD_INIT_SET_READONLY_EVENT;
extern TraceEvent _TRACE_NBD_INIT_FINISH_EVENT;
extern TraceEvent _TRACE_NBD_CLIENT_LOOP_EVENT;
extern TraceEvent _TRACE_NBD_CLIENT_LOOP_RET_EVENT;
extern TraceEvent _TRACE_NBD_CLIENT_CLEAR_QUEUE_EVENT;
extern TraceEvent _TRACE_NBD_CLIENT_CLEAR_SOCKET_EVENT;
extern TraceEvent _TRACE_NBD_SEND_REQUEST_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_SIMPLE_REPLY_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_STRUCTURED_REPLY_CHUNK_EVENT;
extern TraceEvent _TRACE_NBD_UNKNOWN_ERROR_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_SEND_REP_LEN_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_SEND_REP_ERR_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_SEND_REP_LIST_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_REQUEST_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_SEND_INFO_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUESTS_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUEST_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_HANDLE_INFO_BLOCK_SIZE_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_HANDSHAKE_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_META_CONTEXT_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_META_QUERY_SKIP_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_META_QUERY_PARSE_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_META_QUERY_REPLY_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_OPTIONS_FLAGS_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_MAGIC_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_OPTION_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_BEGIN_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_OLD_STYLE_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_NEW_STYLE_SIZE_FLAGS_EVENT;
extern TraceEvent _TRACE_NBD_NEGOTIATE_SUCCESS_EVENT;
extern TraceEvent _TRACE_NBD_RECEIVE_REQUEST_EVENT;
extern TraceEvent _TRACE_NBD_BLK_AIO_ATTACHED_EVENT;
extern TraceEvent _TRACE_NBD_BLK_AIO_DETACH_EVENT;
extern TraceEvent _TRACE_NBD_CO_SEND_SIMPLE_REPLY_EVENT;
extern TraceEvent _TRACE_NBD_CO_SEND_STRUCTURED_DONE_EVENT;
extern TraceEvent _TRACE_NBD_CO_SEND_STRUCTURED_READ_EVENT;
extern TraceEvent _TRACE_NBD_CO_SEND_STRUCTURED_READ_HOLE_EVENT;
extern TraceEvent _TRACE_NBD_CO_SEND_STRUCTURED_ERROR_EVENT;
extern TraceEvent _TRACE_NBD_CO_RECEIVE_REQUEST_DECODE_TYPE_EVENT;
extern TraceEvent _TRACE_NBD_CO_RECEIVE_REQUEST_PAYLOAD_RECEIVED_EVENT;
extern TraceEvent _TRACE_NBD_CO_RECEIVE_REQUEST_CMD_WRITE_EVENT;
extern TraceEvent _TRACE_NBD_TRIP_EVENT;
extern uint16_t _TRACE_NBD_SEND_OPTION_REQUEST_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_OPTION_REPLY_DSTATE;
extern uint16_t _TRACE_NBD_REPLY_ERR_UNSUP_DSTATE;
extern uint16_t _TRACE_NBD_OPT_GO_START_DSTATE;
extern uint16_t _TRACE_NBD_OPT_GO_SUCCESS_DSTATE;
extern uint16_t _TRACE_NBD_OPT_GO_INFO_UNKNOWN_DSTATE;
extern uint16_t _TRACE_NBD_OPT_GO_INFO_BLOCK_SIZE_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_QUERY_EXPORTS_START_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_QUERY_EXPORTS_SUCCESS_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_STARTTLS_NEW_CLIENT_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_STARTTLS_TLS_HANDSHAKE_DSTATE;
extern uint16_t _TRACE_NBD_OPT_META_REQUEST_DSTATE;
extern uint16_t _TRACE_NBD_OPT_META_REPLY_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_NEGOTIATE_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_NEGOTIATE_MAGIC_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_NEGOTIATE_SERVER_FLAGS_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_NEGOTIATE_DEFAULT_NAME_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_NEGOTIATE_SIZE_FLAGS_DSTATE;
extern uint16_t _TRACE_NBD_INIT_SET_SOCKET_DSTATE;
extern uint16_t _TRACE_NBD_INIT_SET_BLOCK_SIZE_DSTATE;
extern uint16_t _TRACE_NBD_INIT_SET_SIZE_DSTATE;
extern uint16_t _TRACE_NBD_INIT_TRAILING_BYTES_DSTATE;
extern uint16_t _TRACE_NBD_INIT_SET_READONLY_DSTATE;
extern uint16_t _TRACE_NBD_INIT_FINISH_DSTATE;
extern uint16_t _TRACE_NBD_CLIENT_LOOP_DSTATE;
extern uint16_t _TRACE_NBD_CLIENT_LOOP_RET_DSTATE;
extern uint16_t _TRACE_NBD_CLIENT_CLEAR_QUEUE_DSTATE;
extern uint16_t _TRACE_NBD_CLIENT_CLEAR_SOCKET_DSTATE;
extern uint16_t _TRACE_NBD_SEND_REQUEST_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_SIMPLE_REPLY_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_STRUCTURED_REPLY_CHUNK_DSTATE;
extern uint16_t _TRACE_NBD_UNKNOWN_ERROR_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_SEND_REP_LEN_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_SEND_REP_ERR_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_SEND_REP_LIST_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_REQUEST_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_SEND_INFO_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUESTS_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUEST_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_HANDLE_INFO_BLOCK_SIZE_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_HANDSHAKE_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_META_CONTEXT_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_META_QUERY_SKIP_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_META_QUERY_PARSE_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_META_QUERY_REPLY_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_OPTIONS_FLAGS_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_MAGIC_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_OPTION_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_BEGIN_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_OLD_STYLE_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_NEW_STYLE_SIZE_FLAGS_DSTATE;
extern uint16_t _TRACE_NBD_NEGOTIATE_SUCCESS_DSTATE;
extern uint16_t _TRACE_NBD_RECEIVE_REQUEST_DSTATE;
extern uint16_t _TRACE_NBD_BLK_AIO_ATTACHED_DSTATE;
extern uint16_t _TRACE_NBD_BLK_AIO_DETACH_DSTATE;
extern uint16_t _TRACE_NBD_CO_SEND_SIMPLE_REPLY_DSTATE;
extern uint16_t _TRACE_NBD_CO_SEND_STRUCTURED_DONE_DSTATE;
extern uint16_t _TRACE_NBD_CO_SEND_STRUCTURED_READ_DSTATE;
extern uint16_t _TRACE_NBD_CO_SEND_STRUCTURED_READ_HOLE_DSTATE;
extern uint16_t _TRACE_NBD_CO_SEND_STRUCTURED_ERROR_DSTATE;
extern uint16_t _TRACE_NBD_CO_RECEIVE_REQUEST_DECODE_TYPE_DSTATE;
extern uint16_t _TRACE_NBD_CO_RECEIVE_REQUEST_PAYLOAD_RECEIVED_DSTATE;
extern uint16_t _TRACE_NBD_CO_RECEIVE_REQUEST_CMD_WRITE_DSTATE;
extern uint16_t _TRACE_NBD_TRIP_DSTATE;
#define TRACE_NBD_SEND_OPTION_REQUEST_ENABLED 1
#define TRACE_NBD_RECEIVE_OPTION_REPLY_ENABLED 1
#define TRACE_NBD_REPLY_ERR_UNSUP_ENABLED 1
#define TRACE_NBD_OPT_GO_START_ENABLED 1
#define TRACE_NBD_OPT_GO_SUCCESS_ENABLED 1
#define TRACE_NBD_OPT_GO_INFO_UNKNOWN_ENABLED 1
#define TRACE_NBD_OPT_GO_INFO_BLOCK_SIZE_ENABLED 1
#define TRACE_NBD_RECEIVE_QUERY_EXPORTS_START_ENABLED 1
#define TRACE_NBD_RECEIVE_QUERY_EXPORTS_SUCCESS_ENABLED 1
#define TRACE_NBD_RECEIVE_STARTTLS_NEW_CLIENT_ENABLED 1
#define TRACE_NBD_RECEIVE_STARTTLS_TLS_HANDSHAKE_ENABLED 1
#define TRACE_NBD_OPT_META_REQUEST_ENABLED 1
#define TRACE_NBD_OPT_META_REPLY_ENABLED 1
#define TRACE_NBD_RECEIVE_NEGOTIATE_ENABLED 1
#define TRACE_NBD_RECEIVE_NEGOTIATE_MAGIC_ENABLED 1
#define TRACE_NBD_RECEIVE_NEGOTIATE_SERVER_FLAGS_ENABLED 1
#define TRACE_NBD_RECEIVE_NEGOTIATE_DEFAULT_NAME_ENABLED 1
#define TRACE_NBD_RECEIVE_NEGOTIATE_SIZE_FLAGS_ENABLED 1
#define TRACE_NBD_INIT_SET_SOCKET_ENABLED 1
#define TRACE_NBD_INIT_SET_BLOCK_SIZE_ENABLED 1
#define TRACE_NBD_INIT_SET_SIZE_ENABLED 1
#define TRACE_NBD_INIT_TRAILING_BYTES_ENABLED 1
#define TRACE_NBD_INIT_SET_READONLY_ENABLED 1
#define TRACE_NBD_INIT_FINISH_ENABLED 1
#define TRACE_NBD_CLIENT_LOOP_ENABLED 1
#define TRACE_NBD_CLIENT_LOOP_RET_ENABLED 1
#define TRACE_NBD_CLIENT_CLEAR_QUEUE_ENABLED 1
#define TRACE_NBD_CLIENT_CLEAR_SOCKET_ENABLED 1
#define TRACE_NBD_SEND_REQUEST_ENABLED 1
#define TRACE_NBD_RECEIVE_SIMPLE_REPLY_ENABLED 1
#define TRACE_NBD_RECEIVE_STRUCTURED_REPLY_CHUNK_ENABLED 1
#define TRACE_NBD_UNKNOWN_ERROR_ENABLED 1
#define TRACE_NBD_NEGOTIATE_SEND_REP_LEN_ENABLED 1
#define TRACE_NBD_NEGOTIATE_SEND_REP_ERR_ENABLED 1
#define TRACE_NBD_NEGOTIATE_SEND_REP_LIST_ENABLED 1
#define TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_ENABLED 1
#define TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_REQUEST_ENABLED 1
#define TRACE_NBD_NEGOTIATE_SEND_INFO_ENABLED 1
#define TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUESTS_ENABLED 1
#define TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUEST_ENABLED 1
#define TRACE_NBD_NEGOTIATE_HANDLE_INFO_BLOCK_SIZE_ENABLED 1
#define TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_ENABLED 1
#define TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_HANDSHAKE_ENABLED 1
#define TRACE_NBD_NEGOTIATE_META_CONTEXT_ENABLED 1
#define TRACE_NBD_NEGOTIATE_META_QUERY_SKIP_ENABLED 1
#define TRACE_NBD_NEGOTIATE_META_QUERY_PARSE_ENABLED 1
#define TRACE_NBD_NEGOTIATE_META_QUERY_REPLY_ENABLED 1
#define TRACE_NBD_NEGOTIATE_OPTIONS_FLAGS_ENABLED 1
#define TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_MAGIC_ENABLED 1
#define TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_OPTION_ENABLED 1
#define TRACE_NBD_NEGOTIATE_BEGIN_ENABLED 1
#define TRACE_NBD_NEGOTIATE_OLD_STYLE_ENABLED 1
#define TRACE_NBD_NEGOTIATE_NEW_STYLE_SIZE_FLAGS_ENABLED 1
#define TRACE_NBD_NEGOTIATE_SUCCESS_ENABLED 1
#define TRACE_NBD_RECEIVE_REQUEST_ENABLED 1
#define TRACE_NBD_BLK_AIO_ATTACHED_ENABLED 1
#define TRACE_NBD_BLK_AIO_DETACH_ENABLED 1
#define TRACE_NBD_CO_SEND_SIMPLE_REPLY_ENABLED 1
#define TRACE_NBD_CO_SEND_STRUCTURED_DONE_ENABLED 1
#define TRACE_NBD_CO_SEND_STRUCTURED_READ_ENABLED 1
#define TRACE_NBD_CO_SEND_STRUCTURED_READ_HOLE_ENABLED 1
#define TRACE_NBD_CO_SEND_STRUCTURED_ERROR_ENABLED 1
#define TRACE_NBD_CO_RECEIVE_REQUEST_DECODE_TYPE_ENABLED 1
#define TRACE_NBD_CO_RECEIVE_REQUEST_PAYLOAD_RECEIVED_ENABLED 1
#define TRACE_NBD_CO_RECEIVE_REQUEST_CMD_WRITE_ENABLED 1
#define TRACE_NBD_TRIP_ENABLED 1

#define TRACE_NBD_SEND_OPTION_REQUEST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_send_option_request(uint32_t opt, const char * name, uint32_t len)
{
}

static inline void trace_nbd_send_option_request(uint32_t opt, const char * name, uint32_t len)
{
    if (true) {
        _nocheck__trace_nbd_send_option_request(opt, name, len);
    }
}

#define TRACE_NBD_RECEIVE_OPTION_REPLY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_option_reply(uint32_t option, const char * optname, uint32_t type, const char * typename, uint32_t length)
{
}

static inline void trace_nbd_receive_option_reply(uint32_t option, const char * optname, uint32_t type, const char * typename, uint32_t length)
{
    if (true) {
        _nocheck__trace_nbd_receive_option_reply(option, optname, type, typename, length);
    }
}

#define TRACE_NBD_REPLY_ERR_UNSUP_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_reply_err_unsup(uint32_t option, const char * name)
{
}

static inline void trace_nbd_reply_err_unsup(uint32_t option, const char * name)
{
    if (true) {
        _nocheck__trace_nbd_reply_err_unsup(option, name);
    }
}

#define TRACE_NBD_OPT_GO_START_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_opt_go_start(const char * name)
{
}

static inline void trace_nbd_opt_go_start(const char * name)
{
    if (true) {
        _nocheck__trace_nbd_opt_go_start(name);
    }
}

#define TRACE_NBD_OPT_GO_SUCCESS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_opt_go_success(void)
{
}

static inline void trace_nbd_opt_go_success(void)
{
    if (true) {
        _nocheck__trace_nbd_opt_go_success();
    }
}

#define TRACE_NBD_OPT_GO_INFO_UNKNOWN_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_opt_go_info_unknown(int info, const char * name)
{
}

static inline void trace_nbd_opt_go_info_unknown(int info, const char * name)
{
    if (true) {
        _nocheck__trace_nbd_opt_go_info_unknown(info, name);
    }
}

#define TRACE_NBD_OPT_GO_INFO_BLOCK_SIZE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_opt_go_info_block_size(uint32_t minimum, uint32_t preferred, uint32_t maximum)
{
}

static inline void trace_nbd_opt_go_info_block_size(uint32_t minimum, uint32_t preferred, uint32_t maximum)
{
    if (true) {
        _nocheck__trace_nbd_opt_go_info_block_size(minimum, preferred, maximum);
    }
}

#define TRACE_NBD_RECEIVE_QUERY_EXPORTS_START_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_query_exports_start(const char * wantname)
{
}

static inline void trace_nbd_receive_query_exports_start(const char * wantname)
{
    if (true) {
        _nocheck__trace_nbd_receive_query_exports_start(wantname);
    }
}

#define TRACE_NBD_RECEIVE_QUERY_EXPORTS_SUCCESS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_query_exports_success(const char * wantname)
{
}

static inline void trace_nbd_receive_query_exports_success(const char * wantname)
{
    if (true) {
        _nocheck__trace_nbd_receive_query_exports_success(wantname);
    }
}

#define TRACE_NBD_RECEIVE_STARTTLS_NEW_CLIENT_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_starttls_new_client(void)
{
}

static inline void trace_nbd_receive_starttls_new_client(void)
{
    if (true) {
        _nocheck__trace_nbd_receive_starttls_new_client();
    }
}

#define TRACE_NBD_RECEIVE_STARTTLS_TLS_HANDSHAKE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_starttls_tls_handshake(void)
{
}

static inline void trace_nbd_receive_starttls_tls_handshake(void)
{
    if (true) {
        _nocheck__trace_nbd_receive_starttls_tls_handshake();
    }
}

#define TRACE_NBD_OPT_META_REQUEST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_opt_meta_request(const char * context, const char * export)
{
}

static inline void trace_nbd_opt_meta_request(const char * context, const char * export)
{
    if (true) {
        _nocheck__trace_nbd_opt_meta_request(context, export);
    }
}

#define TRACE_NBD_OPT_META_REPLY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_opt_meta_reply(const char * context, uint32_t id)
{
}

static inline void trace_nbd_opt_meta_reply(const char * context, uint32_t id)
{
    if (true) {
        _nocheck__trace_nbd_opt_meta_reply(context, id);
    }
}

#define TRACE_NBD_RECEIVE_NEGOTIATE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_negotiate(void * tlscreds, const char * hostname)
{
}

static inline void trace_nbd_receive_negotiate(void * tlscreds, const char * hostname)
{
    if (true) {
        _nocheck__trace_nbd_receive_negotiate(tlscreds, hostname);
    }
}

#define TRACE_NBD_RECEIVE_NEGOTIATE_MAGIC_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_negotiate_magic(uint64_t magic)
{
}

static inline void trace_nbd_receive_negotiate_magic(uint64_t magic)
{
    if (true) {
        _nocheck__trace_nbd_receive_negotiate_magic(magic);
    }
}

#define TRACE_NBD_RECEIVE_NEGOTIATE_SERVER_FLAGS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_negotiate_server_flags(uint32_t globalflags)
{
}

static inline void trace_nbd_receive_negotiate_server_flags(uint32_t globalflags)
{
    if (true) {
        _nocheck__trace_nbd_receive_negotiate_server_flags(globalflags);
    }
}

#define TRACE_NBD_RECEIVE_NEGOTIATE_DEFAULT_NAME_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_negotiate_default_name(void)
{
}

static inline void trace_nbd_receive_negotiate_default_name(void)
{
    if (true) {
        _nocheck__trace_nbd_receive_negotiate_default_name();
    }
}

#define TRACE_NBD_RECEIVE_NEGOTIATE_SIZE_FLAGS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_negotiate_size_flags(uint64_t size, uint16_t flags)
{
}

static inline void trace_nbd_receive_negotiate_size_flags(uint64_t size, uint16_t flags)
{
    if (true) {
        _nocheck__trace_nbd_receive_negotiate_size_flags(size, flags);
    }
}

#define TRACE_NBD_INIT_SET_SOCKET_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_init_set_socket(void)
{
}

static inline void trace_nbd_init_set_socket(void)
{
    if (true) {
        _nocheck__trace_nbd_init_set_socket();
    }
}

#define TRACE_NBD_INIT_SET_BLOCK_SIZE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_init_set_block_size(unsigned long block_size)
{
}

static inline void trace_nbd_init_set_block_size(unsigned long block_size)
{
    if (true) {
        _nocheck__trace_nbd_init_set_block_size(block_size);
    }
}

#define TRACE_NBD_INIT_SET_SIZE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_init_set_size(unsigned long sectors)
{
}

static inline void trace_nbd_init_set_size(unsigned long sectors)
{
    if (true) {
        _nocheck__trace_nbd_init_set_size(sectors);
    }
}

#define TRACE_NBD_INIT_TRAILING_BYTES_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_init_trailing_bytes(int ignored_bytes)
{
}

static inline void trace_nbd_init_trailing_bytes(int ignored_bytes)
{
    if (true) {
        _nocheck__trace_nbd_init_trailing_bytes(ignored_bytes);
    }
}

#define TRACE_NBD_INIT_SET_READONLY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_init_set_readonly(void)
{
}

static inline void trace_nbd_init_set_readonly(void)
{
    if (true) {
        _nocheck__trace_nbd_init_set_readonly();
    }
}

#define TRACE_NBD_INIT_FINISH_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_init_finish(void)
{
}

static inline void trace_nbd_init_finish(void)
{
    if (true) {
        _nocheck__trace_nbd_init_finish();
    }
}

#define TRACE_NBD_CLIENT_LOOP_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_client_loop(void)
{
}

static inline void trace_nbd_client_loop(void)
{
    if (true) {
        _nocheck__trace_nbd_client_loop();
    }
}

#define TRACE_NBD_CLIENT_LOOP_RET_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_client_loop_ret(int ret, const char * error)
{
}

static inline void trace_nbd_client_loop_ret(int ret, const char * error)
{
    if (true) {
        _nocheck__trace_nbd_client_loop_ret(ret, error);
    }
}

#define TRACE_NBD_CLIENT_CLEAR_QUEUE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_client_clear_queue(void)
{
}

static inline void trace_nbd_client_clear_queue(void)
{
    if (true) {
        _nocheck__trace_nbd_client_clear_queue();
    }
}

#define TRACE_NBD_CLIENT_CLEAR_SOCKET_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_client_clear_socket(void)
{
}

static inline void trace_nbd_client_clear_socket(void)
{
    if (true) {
        _nocheck__trace_nbd_client_clear_socket();
    }
}

#define TRACE_NBD_SEND_REQUEST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_send_request(uint64_t from, uint32_t len, uint64_t handle, uint16_t flags, uint16_t type, const char * name)
{
}

static inline void trace_nbd_send_request(uint64_t from, uint32_t len, uint64_t handle, uint16_t flags, uint16_t type, const char * name)
{
    if (true) {
        _nocheck__trace_nbd_send_request(from, len, handle, flags, type, name);
    }
}

#define TRACE_NBD_RECEIVE_SIMPLE_REPLY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_simple_reply(int32_t error, const char * errname, uint64_t handle)
{
}

static inline void trace_nbd_receive_simple_reply(int32_t error, const char * errname, uint64_t handle)
{
    if (true) {
        _nocheck__trace_nbd_receive_simple_reply(error, errname, handle);
    }
}

#define TRACE_NBD_RECEIVE_STRUCTURED_REPLY_CHUNK_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_structured_reply_chunk(uint16_t flags, uint16_t type, const char * name, uint64_t handle, uint32_t length)
{
}

static inline void trace_nbd_receive_structured_reply_chunk(uint16_t flags, uint16_t type, const char * name, uint64_t handle, uint32_t length)
{
    if (true) {
        _nocheck__trace_nbd_receive_structured_reply_chunk(flags, type, name, handle, length);
    }
}

#define TRACE_NBD_UNKNOWN_ERROR_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_unknown_error(int err)
{
}

static inline void trace_nbd_unknown_error(int err)
{
    if (true) {
        _nocheck__trace_nbd_unknown_error(err);
    }
}

#define TRACE_NBD_NEGOTIATE_SEND_REP_LEN_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_send_rep_len(uint32_t opt, const char * optname, uint32_t type, const char * typename, uint32_t len)
{
}

static inline void trace_nbd_negotiate_send_rep_len(uint32_t opt, const char * optname, uint32_t type, const char * typename, uint32_t len)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_send_rep_len(opt, optname, type, typename, len);
    }
}

#define TRACE_NBD_NEGOTIATE_SEND_REP_ERR_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_send_rep_err(const char * msg)
{
}

static inline void trace_nbd_negotiate_send_rep_err(const char * msg)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_send_rep_err(msg);
    }
}

#define TRACE_NBD_NEGOTIATE_SEND_REP_LIST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_send_rep_list(const char * name, const char * desc)
{
}

static inline void trace_nbd_negotiate_send_rep_list(const char * name, const char * desc)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_send_rep_list(name, desc);
    }
}

#define TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_handle_export_name(void)
{
}

static inline void trace_nbd_negotiate_handle_export_name(void)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_handle_export_name();
    }
}

#define TRACE_NBD_NEGOTIATE_HANDLE_EXPORT_NAME_REQUEST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_handle_export_name_request(const char * name)
{
}

static inline void trace_nbd_negotiate_handle_export_name_request(const char * name)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_handle_export_name_request(name);
    }
}

#define TRACE_NBD_NEGOTIATE_SEND_INFO_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_send_info(int info, const char * name, uint32_t length)
{
}

static inline void trace_nbd_negotiate_send_info(int info, const char * name, uint32_t length)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_send_info(info, name, length);
    }
}

#define TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUESTS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_handle_info_requests(int requests)
{
}

static inline void trace_nbd_negotiate_handle_info_requests(int requests)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_handle_info_requests(requests);
    }
}

#define TRACE_NBD_NEGOTIATE_HANDLE_INFO_REQUEST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_handle_info_request(int request, const char * name)
{
}

static inline void trace_nbd_negotiate_handle_info_request(int request, const char * name)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_handle_info_request(request, name);
    }
}

#define TRACE_NBD_NEGOTIATE_HANDLE_INFO_BLOCK_SIZE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_handle_info_block_size(uint32_t minimum, uint32_t preferred, uint32_t maximum)
{
}

static inline void trace_nbd_negotiate_handle_info_block_size(uint32_t minimum, uint32_t preferred, uint32_t maximum)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_handle_info_block_size(minimum, preferred, maximum);
    }
}

#define TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_handle_starttls(void)
{
}

static inline void trace_nbd_negotiate_handle_starttls(void)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_handle_starttls();
    }
}

#define TRACE_NBD_NEGOTIATE_HANDLE_STARTTLS_HANDSHAKE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_handle_starttls_handshake(void)
{
}

static inline void trace_nbd_negotiate_handle_starttls_handshake(void)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_handle_starttls_handshake();
    }
}

#define TRACE_NBD_NEGOTIATE_META_CONTEXT_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_meta_context(const char * optname, const char * export, uint32_t queries)
{
}

static inline void trace_nbd_negotiate_meta_context(const char * optname, const char * export, uint32_t queries)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_meta_context(optname, export, queries);
    }
}

#define TRACE_NBD_NEGOTIATE_META_QUERY_SKIP_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_meta_query_skip(const char * reason)
{
}

static inline void trace_nbd_negotiate_meta_query_skip(const char * reason)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_meta_query_skip(reason);
    }
}

#define TRACE_NBD_NEGOTIATE_META_QUERY_PARSE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_meta_query_parse(const char * query)
{
}

static inline void trace_nbd_negotiate_meta_query_parse(const char * query)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_meta_query_parse(query);
    }
}

#define TRACE_NBD_NEGOTIATE_META_QUERY_REPLY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_meta_query_reply(const char * context, uint32_t id)
{
}

static inline void trace_nbd_negotiate_meta_query_reply(const char * context, uint32_t id)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_meta_query_reply(context, id);
    }
}

#define TRACE_NBD_NEGOTIATE_OPTIONS_FLAGS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_options_flags(uint32_t flags)
{
}

static inline void trace_nbd_negotiate_options_flags(uint32_t flags)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_options_flags(flags);
    }
}

#define TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_MAGIC_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_options_check_magic(uint64_t magic)
{
}

static inline void trace_nbd_negotiate_options_check_magic(uint64_t magic)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_options_check_magic(magic);
    }
}

#define TRACE_NBD_NEGOTIATE_OPTIONS_CHECK_OPTION_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_options_check_option(uint32_t option, const char * name)
{
}

static inline void trace_nbd_negotiate_options_check_option(uint32_t option, const char * name)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_options_check_option(option, name);
    }
}

#define TRACE_NBD_NEGOTIATE_BEGIN_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_begin(void)
{
}

static inline void trace_nbd_negotiate_begin(void)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_begin();
    }
}

#define TRACE_NBD_NEGOTIATE_OLD_STYLE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_old_style(uint64_t size, unsigned flags)
{
}

static inline void trace_nbd_negotiate_old_style(uint64_t size, unsigned flags)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_old_style(size, flags);
    }
}

#define TRACE_NBD_NEGOTIATE_NEW_STYLE_SIZE_FLAGS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_new_style_size_flags(uint64_t size, unsigned flags)
{
}

static inline void trace_nbd_negotiate_new_style_size_flags(uint64_t size, unsigned flags)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_new_style_size_flags(size, flags);
    }
}

#define TRACE_NBD_NEGOTIATE_SUCCESS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_negotiate_success(void)
{
}

static inline void trace_nbd_negotiate_success(void)
{
    if (true) {
        _nocheck__trace_nbd_negotiate_success();
    }
}

#define TRACE_NBD_RECEIVE_REQUEST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_receive_request(uint32_t magic, uint16_t flags, uint16_t type, uint64_t from, uint32_t len)
{
}

static inline void trace_nbd_receive_request(uint32_t magic, uint16_t flags, uint16_t type, uint64_t from, uint32_t len)
{
    if (true) {
        _nocheck__trace_nbd_receive_request(magic, flags, type, from, len);
    }
}

#define TRACE_NBD_BLK_AIO_ATTACHED_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_blk_aio_attached(const char * name, void * ctx)
{
}

static inline void trace_nbd_blk_aio_attached(const char * name, void * ctx)
{
    if (true) {
        _nocheck__trace_nbd_blk_aio_attached(name, ctx);
    }
}

#define TRACE_NBD_BLK_AIO_DETACH_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_blk_aio_detach(const char * name, void * ctx)
{
}

static inline void trace_nbd_blk_aio_detach(const char * name, void * ctx)
{
    if (true) {
        _nocheck__trace_nbd_blk_aio_detach(name, ctx);
    }
}

#define TRACE_NBD_CO_SEND_SIMPLE_REPLY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_send_simple_reply(uint64_t handle, uint32_t error, const char * errname, int len)
{
}

static inline void trace_nbd_co_send_simple_reply(uint64_t handle, uint32_t error, const char * errname, int len)
{
    if (true) {
        _nocheck__trace_nbd_co_send_simple_reply(handle, error, errname, len);
    }
}

#define TRACE_NBD_CO_SEND_STRUCTURED_DONE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_send_structured_done(uint64_t handle)
{
}

static inline void trace_nbd_co_send_structured_done(uint64_t handle)
{
    if (true) {
        _nocheck__trace_nbd_co_send_structured_done(handle);
    }
}

#define TRACE_NBD_CO_SEND_STRUCTURED_READ_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_send_structured_read(uint64_t handle, uint64_t offset, void * data, size_t size)
{
}

static inline void trace_nbd_co_send_structured_read(uint64_t handle, uint64_t offset, void * data, size_t size)
{
    if (true) {
        _nocheck__trace_nbd_co_send_structured_read(handle, offset, data, size);
    }
}

#define TRACE_NBD_CO_SEND_STRUCTURED_READ_HOLE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_send_structured_read_hole(uint64_t handle, uint64_t offset, size_t size)
{
}

static inline void trace_nbd_co_send_structured_read_hole(uint64_t handle, uint64_t offset, size_t size)
{
    if (true) {
        _nocheck__trace_nbd_co_send_structured_read_hole(handle, offset, size);
    }
}

#define TRACE_NBD_CO_SEND_STRUCTURED_ERROR_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_send_structured_error(uint64_t handle, int err, const char * errname, const char * msg)
{
}

static inline void trace_nbd_co_send_structured_error(uint64_t handle, int err, const char * errname, const char * msg)
{
    if (true) {
        _nocheck__trace_nbd_co_send_structured_error(handle, err, errname, msg);
    }
}

#define TRACE_NBD_CO_RECEIVE_REQUEST_DECODE_TYPE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_receive_request_decode_type(uint64_t handle, uint16_t type, const char * name)
{
}

static inline void trace_nbd_co_receive_request_decode_type(uint64_t handle, uint16_t type, const char * name)
{
    if (true) {
        _nocheck__trace_nbd_co_receive_request_decode_type(handle, type, name);
    }
}

#define TRACE_NBD_CO_RECEIVE_REQUEST_PAYLOAD_RECEIVED_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_receive_request_payload_received(uint64_t handle, uint32_t len)
{
}

static inline void trace_nbd_co_receive_request_payload_received(uint64_t handle, uint32_t len)
{
    if (true) {
        _nocheck__trace_nbd_co_receive_request_payload_received(handle, len);
    }
}

#define TRACE_NBD_CO_RECEIVE_REQUEST_CMD_WRITE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_co_receive_request_cmd_write(uint32_t len)
{
}

static inline void trace_nbd_co_receive_request_cmd_write(uint32_t len)
{
    if (true) {
        _nocheck__trace_nbd_co_receive_request_cmd_write(len);
    }
}

#define TRACE_NBD_TRIP_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_nbd_trip(void)
{
}

static inline void trace_nbd_trip(void)
{
    if (true) {
        _nocheck__trace_nbd_trip();
    }
}
#endif /* TRACE_NBD_GENERATED_TRACERS_H */
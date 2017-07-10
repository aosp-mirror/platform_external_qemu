/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// TODO: add guard in header
extern "C" {
#include <stdlib.h>
}

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <err.h>
#include <trusty_std.h>

#include <interface/keymaster/keymaster.h>

#include <UniquePtr.h>

#include "trusty_keymaster.h"
#include "trusty_logger.h"
#include "keymaster_ipc.h"

using namespace keymaster;

uuid_t gatekeeper_uuid = {
    0x38ba0cdc, 0xdf0e, 0x11e4, {0x98, 0x69, 0x23, 0x3f, 0xb6, 0xae, 0x47, 0x95}};

typedef void (*event_handler_proc_t)(const uevent_t* ev, void* ctx);
struct tipc_event_handler {
    event_handler_proc_t proc;
    void* priv;
};

struct keymaster_chan_ctx {
    struct tipc_event_handler handler;
    uuid_t uuid;
    handle_t chan;
    long (*dispatch)(keymaster_chan_ctx*, keymaster_message*, uint32_t, UniquePtr<uint8_t[]>*,
                     uint32_t*);
};

struct keymaster_srv_ctx {
    handle_t port_secure;
    handle_t port_non_secure;
};

static void keymaster_port_handler_secure(const uevent_t* ev, void* priv);
static void keymaster_port_handler_non_secure(const uevent_t* ev, void* priv);

static tipc_event_handler keymaster_port_evt_handler_secure = {
    .proc = keymaster_port_handler_secure, .priv = NULL,
};

static tipc_event_handler keymaster_port_evt_handler_non_secure = {
    .proc = keymaster_port_handler_non_secure, .priv = NULL,
};

static void keymaster_chan_handler(const uevent_t* ev, void* priv);

TrustyKeymaster* device;
int32_t message_version = -1;

class MessageDeleter {
  public:
    explicit MessageDeleter(handle_t chan, int id) {
        chan_ = chan;
        id_ = id;
    }

    ~MessageDeleter() { put_msg(chan_, id_); }

  private:
    handle_t chan_;
    int id_;
};

static long handle_port_errors(const uevent_t* ev) {
    if ((ev->event & IPC_HANDLE_POLL_ERROR) || (ev->event & IPC_HANDLE_POLL_HUP) ||
        (ev->event & IPC_HANDLE_POLL_MSG) || (ev->event & IPC_HANDLE_POLL_SEND_UNBLOCKED)) {
        /* should never happen with port handles */
        LOG_E("error event (0x%x) for port (%d)", ev->event, ev->handle);
        return ERR_BAD_STATE;
    }

    return NO_ERROR;
}

static long send_response(handle_t chan, uint32_t cmd, uint8_t* out_buf, uint32_t out_buf_size) {
    struct keymaster_message km_msg;
    km_msg.cmd = cmd | KM_RESP_BIT;
    iovec_t iov[2] = {{&km_msg, sizeof(km_msg)}, {out_buf, out_buf_size}};
    ipc_msg_t msg = {2, iov, 0, NULL};

    long rc = send_msg(chan, &msg);

    // fatal error
    if (rc < 0) {
        LOG_E("failed (%d) to send_msg for chan (%d)", rc, chan);
        return rc;
    }

    return NO_ERROR;
}

static long send_error_response(handle_t chan, uint32_t cmd, keymaster_error_t err) {
    return send_response(chan, cmd, reinterpret_cast<uint8_t*>(&err), sizeof(err));
}

template <typename Request, typename Response>
static long do_dispatch(void (AndroidKeymaster::*operation)(const Request&, Response*),
                        struct keymaster_message* msg, uint32_t payload_size,
                        UniquePtr<uint8_t[]>* out, uint32_t* out_size) {
    const uint8_t* payload = msg->payload;
    Request req;
    req.message_version = message_version;
    if (!req.Deserialize(&payload, msg->payload + payload_size))
        return ERR_NOT_VALID;

    Response rsp;
    rsp.message_version = message_version;
    (device->*operation)(req, &rsp);

    *out_size = rsp.SerializedSize();
    if (*out_size > KEYMASTER_MAX_BUFFER_LENGTH) {
        *out_size = 0;
        return ERR_NOT_ENOUGH_BUFFER;
    }

    out->reset(new uint8_t[*out_size]);
    if (out->get() == NULL) {
        *out_size = 0;
        return ERR_NO_MEMORY;
    }

    rsp.Serialize(out->get(), out->get() + *out_size);

    return NO_ERROR;
}

static long get_auth_token_key(UniquePtr<uint8_t[]>* key_buf, uint32_t* key_size) {
    keymaster_key_blob_t key;
    long rc = device->GetAuthTokenKey(&key);

    if (rc != NO_ERROR) {
        return rc;
    }

    if (key.key_material_size > KEYMASTER_MAX_BUFFER_LENGTH) {
        return ERR_NOT_ENOUGH_BUFFER;
    }

    key_buf->reset(new uint8_t[key.key_material_size]);
    if (key_buf->get() == NULL) {
        return ERR_NO_MEMORY;
    }

    *key_size = key.key_material_size;

    memcpy(key_buf->get(), key.key_material, key.key_material_size);
    return NO_ERROR;
}

static long keymaster_dispatch_secure(keymaster_chan_ctx* ctx, keymaster_message* msg,
                                      uint32_t payload_size, UniquePtr<uint8_t[]>* out,
                                      uint32_t* out_size) {
    switch (msg->cmd) {
    case KM_GET_AUTH_TOKEN_KEY:
        return get_auth_token_key(out, out_size);
    default:
        return ERR_NOT_IMPLEMENTED;
    }
}

static long keymaster_dispatch_non_secure(keymaster_chan_ctx* ctx, keymaster_message* msg,
                                          uint32_t payload_size, UniquePtr<uint8_t[]>* out,
                                          uint32_t* out_size) {
    LOG_D("Dispatching command %d", msg->cmd);
    switch (msg->cmd) {
    case KM_GENERATE_KEY:
        LOG_D("Dispatching GENERATE_KEY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::GenerateKey, msg, payload_size, out, out_size);

    case KM_BEGIN_OPERATION:
        LOG_D("Dispatching BEGIN_OPERATION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::BeginOperation, msg, payload_size, out, out_size);

    case KM_UPDATE_OPERATION:
        LOG_D("Dispatching UPDATE_OPERATION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::UpdateOperation, msg, payload_size, out, out_size);

    case KM_FINISH_OPERATION:
        LOG_D("Dispatching FINISH_OPERATION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::FinishOperation, msg, payload_size, out, out_size);

    case KM_IMPORT_KEY:
        LOG_D("Dispatching IMPORT_KEY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::ImportKey, msg, payload_size, out, out_size);

    case KM_EXPORT_KEY:
        LOG_D("Dispatching EXPORT_KEY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::ExportKey, msg, payload_size, out, out_size);

    case KM_GET_VERSION:
        LOG_D("Dispatching GET_VERSION, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::GetVersion, msg, payload_size, out, out_size);

    case KM_ADD_RNG_ENTROPY:
        LOG_D("Dispatching ADD_RNG_ENTROPY, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::AddRngEntropy, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_ALGORITHMS:
        LOG_D("Dispatching GET_SUPPORTED_ALGORITHMS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedAlgorithms, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_BLOCK_MODES:
        LOG_D("Dispatching GET_SUPPORTED_BLOCK_MODES, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedBlockModes, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_PADDING_MODES:
        LOG_D("Dispatching GET_SUPPORTED_PADDING_MODES, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedPaddingModes, msg, payload_size, out,
                           out_size);

    case KM_GET_SUPPORTED_DIGESTS:
        LOG_D("Dispatching GET_SUPPORTED_DIGESTS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedDigests, msg, payload_size, out, out_size);

    case KM_GET_SUPPORTED_IMPORT_FORMATS:
        LOG_D("Dispatching GET_SUPPORTED_IMPORT_FORMATS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedImportFormats, msg, payload_size, out,
                           out_size);

    case KM_GET_SUPPORTED_EXPORT_FORMATS:
        LOG_D("Dispatching GET_SUPPORTED_EXPORT_FORMATS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::SupportedExportFormats, msg, payload_size, out,
                           out_size);

    case KM_GET_KEY_CHARACTERISTICS:
        LOG_D("Dispatching GET_KEY_CHARACTERISTICS, size: %d", payload_size);
        return do_dispatch(&TrustyKeymaster::GetKeyCharacteristics, msg, payload_size, out,
                           out_size);

    case KM_ABORT_OPERATION:
        LOG_D("Dispatching ABORT_OPERATION, size %d", payload_size);
        return do_dispatch(&TrustyKeymaster::AbortOperation, msg, payload_size, out, out_size);
    default:
        return ERR_NOT_IMPLEMENTED;
    }
}

static bool keymaster_port_accessible(uuid_t* uuid, bool secure) {
    return !secure || memcmp(uuid, &gatekeeper_uuid, sizeof(gatekeeper_uuid)) == 0;
}

static keymaster_chan_ctx* keymaster_ctx_open(handle_t chan, uuid_t* uuid, bool secure) {
    if (!keymaster_port_accessible(uuid, secure)) {
        LOG_E("access denied for client uuid", 0);
        return NULL;
    }

    keymaster_chan_ctx* ctx = new keymaster_chan_ctx;
    if (ctx == NULL) {
        return ctx;
    }

    ctx->handler.proc = &keymaster_chan_handler;
    ctx->handler.priv = ctx;
    ctx->uuid = *uuid;
    ctx->chan = chan;
    ctx->dispatch = secure ? &keymaster_dispatch_secure : &keymaster_dispatch_non_secure;
    return ctx;
}

static void keymaster_ctx_close(keymaster_chan_ctx* ctx) {
    close(ctx->chan);
    delete ctx;
}

static long handle_msg(keymaster_chan_ctx* ctx) {
    handle_t chan = ctx->chan;

    /* get message info */
    ipc_msg_info_t msg_inf;
    int rc = get_msg(chan, &msg_inf);
    if (rc == ERR_NO_MSG)
        return NO_ERROR; /* no new messages */

    // fatal error
    if (rc != NO_ERROR) {
        LOG_E("failed (%d) to get_msg for chan (%d), closing connection", rc, chan);
        return rc;
    }

    MessageDeleter md(chan, msg_inf.id);

    // allocate msg_buf, with one extra byte for null-terminator
    UniquePtr<uint8_t[]> msg_buf(new uint8_t[msg_inf.len + 1]);
    msg_buf[msg_inf.len] = 0;

    /* read msg content */
    iovec_t iov = {msg_buf.get(), msg_inf.len};
    ipc_msg_t msg = {1, &iov, 0, NULL};

    rc = read_msg(chan, msg_inf.id, 0, &msg);

    // fatal error
    if (rc < 0) {
        LOG_E("failed to read msg (%d)", rc, chan);
        return rc;
    }
    LOG_D("Read %d-byte message", rc);

    if (((unsigned long)rc) < sizeof(keymaster_message)) {
        LOG_E("invalid message of size (%d)", rc, chan);
        return ERR_NOT_VALID;
    }

    UniquePtr<uint8_t[]> out_buf;
    uint32_t out_buf_size = 0;
    keymaster_message* in_msg = reinterpret_cast<keymaster_message*>(msg_buf.get());

    rc = ctx->dispatch(ctx, in_msg, msg_inf.len - sizeof(*in_msg), &out_buf, &out_buf_size);
    if (rc < 0) {
        LOG_E("error handling message (%d)", rc);
        return send_error_response(chan, in_msg->cmd, KM_ERROR_UNKNOWN_ERROR);
    }

    LOG_D("Sending %d-byte response", out_buf_size);
    return send_response(chan, in_msg->cmd, out_buf.get(), out_buf_size);
}

static void keymaster_chan_handler(const uevent_t* ev, void* priv) {
    keymaster_chan_ctx* ctx = reinterpret_cast<keymaster_chan_ctx*>(priv);
    if (ctx == NULL) {
        LOG_E("error: no context on channel %d", ev->handle);
        close(ev->handle);
        return;
    }

    if ((ev->event & IPC_HANDLE_POLL_ERROR) || (ev->event & IPC_HANDLE_POLL_READY)) {
        /* close it as it is in an error state */
        LOG_E("error event (0x%x) for chan (%d)", ev->event, ev->handle);
        close(ev->handle);
        return;
    }

    if (ev->event & IPC_HANDLE_POLL_MSG) {
        long rc = handle_msg(ctx);
        if (rc != NO_ERROR) {
            /* report an error and close channel */
            LOG_E("failed (%d) to handle event on channel %d", rc, ev->handle);
            keymaster_ctx_close(ctx);
            return;
        }
    }

    if (ev->event & IPC_HANDLE_POLL_HUP) {
        /* closed by peer. */
        keymaster_ctx_close(ctx);
        return;
    }
}

static void keymaster_port_handler(const uevent_t* ev, void* priv, bool secure) {
    long rc = handle_port_errors(ev);
    if (rc != NO_ERROR) {
        abort();
    }

    uuid_t peer_uuid;
    if (ev->event & IPC_HANDLE_POLL_READY) {
        /* incoming connection: accept it */
        int rc = accept(ev->handle, &peer_uuid);
        if (rc < 0) {
            LOG_E("failed (%d) to accept on port %d", rc, ev->handle);
            return;
        }

        handle_t chan = (handle_t)rc;
        keymaster_chan_ctx* ctx = keymaster_ctx_open(chan, &peer_uuid, secure);
        if (ctx == NULL) {
            LOG_E("failed to allocate context on chan %d", chan);
            close(chan);
            return;
        }

        rc = set_cookie(chan, ctx);
        if (rc < 0) {
            LOG_E("failed (%d) to set_cookie on chan %d", rc, chan);
            keymaster_ctx_close(ctx);
            return;
        }
    }
}

static void keymaster_port_handler_secure(const uevent_t* ev, void* priv) {
    keymaster_port_handler(ev, priv, true);
}

static void keymaster_port_handler_non_secure(const uevent_t* ev, void* priv) {
    keymaster_port_handler(ev, priv, false);
}

static void dispatch_event(const uevent_t* ev) {
    if (ev == NULL)
        return;

    if (ev->event == IPC_HANDLE_POLL_NONE) {
        /* not really an event, do nothing */
        LOG_E("got an empty event", 0);
        return;
    }

    /* check if we have handler */
    tipc_event_handler* handler = reinterpret_cast<tipc_event_handler*>(ev->cookie);
    if (handler && handler->proc) {
        /* invoke it */
        handler->proc(ev, handler->priv);
        return;
    }

    /* no handler? close it */
    LOG_E("no handler for event (0x%x) with handle %d", ev->event, ev->handle);

    close(ev->handle);

    return;
}

static long keymaster_ipc_init(keymaster_srv_ctx* ctx) {
    int rc;

    /* Initialize secure-side service */
    rc = port_create(KEYMASTER_SECURE_PORT, 1, KEYMASTER_MAX_BUFFER_LENGTH,
                     IPC_PORT_ALLOW_TA_CONNECT);
    if (rc < 0) {
        LOG_E("Failed (%d) to create port %s", rc, KEYMASTER_SECURE_PORT);
        return rc;
    }

    ctx->port_secure = (handle_t)rc;

    rc = set_cookie(ctx->port_secure, &keymaster_port_evt_handler_secure);
    if (rc) {
        LOG_E("failed (%d) to set_cookie on port %d", rc, ctx->port_secure);
        close(ctx->port_secure);
        return rc;
    }

    /* initialize non-secure side service */
    rc = port_create(KEYMASTER_PORT, 1, KEYMASTER_MAX_BUFFER_LENGTH, IPC_PORT_ALLOW_NS_CONNECT);
    if (rc < 0) {
        LOG_E("Failed (%d) to create port %s", rc, KEYMASTER_PORT);
        return rc;
    }

    ctx->port_non_secure = (handle_t)rc;

    rc = set_cookie(ctx->port_non_secure, &keymaster_port_evt_handler_non_secure);
    if (rc) {
        LOG_E("failed (%d) to set_cookie on port %d", rc, ctx->port_non_secure);
        close(ctx->port_non_secure);
        return rc;
    }

    return NO_ERROR;
}

int main(void) {
    long rc;
    uevent_t event;

    device = new TrustyKeymaster(new TrustyKeymasterContext, 16);

    TrustyLogger::initialize();

    LOG_I("Initializing", 0);

    GetVersionRequest request;
    GetVersionResponse response;
    device->GetVersion(request, &response);
    if (response.error == KM_ERROR_OK) {
      message_version = MessageVersion(
          response.major_ver, response.minor_ver, response.subminor_ver);
    } else {
      LOG_E("Error %d determining AndroidKeymaster version.", response.error);
      return ERR_GENERIC;
    }

    keymaster_srv_ctx ctx;
    rc = keymaster_ipc_init(&ctx);
    if (rc < 0) {
        LOG_E("failed (%d) to initialize keymaster", rc);
        return rc;
    }

    /* enter main event loop */
    while (true) {
        event.handle = INVALID_IPC_HANDLE;
        event.event = 0;
        event.cookie = NULL;

        rc = wait_any(&event, -1);
        if (rc < 0) {
            LOG_E("wait_any failed (%d)", rc);
            break;
        }

        if (rc == NO_ERROR) { /* got an event */
            dispatch_event(&event);
        }
    }

    return 0;
}

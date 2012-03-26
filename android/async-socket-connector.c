/*
 * Copyright (C) 2011 The Android Open Source Project
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

/*
 * Encapsulates exchange protocol between the emulator, and an Android device
 * that is connected to the host via USB. The communication is established over
 * a TCP port forwarding, enabled by ADB.
 */

#include "qemu-common.h"
#include "android/async-utils.h"
#include "android/utils/debug.h"
#include "android/async-socket-connector.h"
#include "utils/panic.h"
#include "iolooper.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(asconnector,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(asconnector)

/********************************************************************************
 *                             Internals
 *******************************************************************************/

struct AsyncSocketConnector {
    /* TCP address for the connection. */
    SockAddress     address;
    /* I/O looper for asynchronous I/O. */
    Looper*         looper;
    /* I/O port for asynchronous connection. */
    LoopIo          connector_io[1];
    /* Timer that is used to retry asynchronous connections. */
    LoopTimer       connector_timer[1];
    /* Asynchronous connector to the socket. */
    AsyncConnector  connector[1];
    /* Callback to invoke on connection / connection error. */
    asc_event_cb    on_connected_cb;
    /* An opaque parameter to pass to the connection callback. */
    void*           on_connected_cb_opaque;
    /* Retry timeout in milliseconds. */
    int             retry_to;
    /* Socket descriptor for the connection. */
    int             fd;
};

/* Asynchronous I/O looper callback invoked by the connector.
 * Param:
 *  opaque - AsyncSocketConnector instance.
 *  fd, events - Standard I/O callback parameters.
 */
static void _on_async_socket_connector_io(void* opaque, int fd, unsigned events);

/* Gets socket's address string. */
AINLINED const char*
_asc_socket_string(AsyncSocketConnector* connector)
{
    return sock_address_to_string(&connector->address);
}

/* Destroys AsyncSocketConnector instance.
 * Param:
 *  connector - Initialized AsyncSocketConnector instance.
 */
static void
_async_socket_connector_free(AsyncSocketConnector* connector)
{
    if (connector != NULL) {
        if (connector->fd >= 0) {
            socket_close(connector->fd);
        }

        if (connector->looper != NULL) {
            loopTimer_done(connector->connector_timer);
            looper_free(connector->looper);
        }

        sock_address_done(&connector->address);

        AFREE(connector);
    }
}

/* Opens connection socket.
 * Param:
 *  connector - Initialized AsyncSocketConnector instance.
 * Return:
 *  0 on success, or -1 on failure.
 */
static int
_async_socket_connector_open_socket(AsyncSocketConnector* connector)
{
    /* Open socket. */
    connector->fd = socket_create_inet(SOCKET_STREAM);
    if (connector->fd < 0) {
        D("Unable to create connector socket for %s. Error: %s",
          _asc_socket_string(connector), strerror(errno));
        return -1;
    }

    /* Prepare for async I/O on the connector. */
    socket_set_nonblock(connector->fd);
    loopIo_init(connector->connector_io, connector->looper, connector->fd,
                _on_async_socket_connector_io, connector);

    return 0;
}

/* Closes connection socket.
 * Param:
 *  connector - Initialized AsyncSocketConnector instance.
 * Return:
 *  0 on success, or -1 on failure.
 */
static void
_async_socket_connector_close_socket(AsyncSocketConnector* connector)
{
    if (connector->fd >= 0) {
        socket_close(connector->fd);
        connector->fd = -1;
    }
}

/* Asynchronous connector (AsyncConnector instance) has completed connection
 *  attempt.
 *
 * NOTE: Upon exit from this routine AsyncSocketConnector instance might be
 * destroyed. So, once this routine is called, there must be no further
 * references to AsyncSocketConnector instance passed to this routine.
 * Param:
 *  connector - Initialized AsyncSocketConnector instance.
 *  status - Status of the connection attempt.
 */
static void
_on_async_socket_connector_connecting(AsyncSocketConnector* connector,
                                      AsyncStatus status)
{
    ASCCbRes action;
    int do_retry = 0;

    switch (status) {
        case ASYNC_COMPLETE:
            loopIo_done(connector->connector_io);
            D("Socket %s is connected", _asc_socket_string(connector));
            /* Invoke "on connected" callback */
            action = connector->on_connected_cb(connector->on_connected_cb_opaque,
                                                connector, ASC_CONNECTION_SUCCEEDED);
            if (action == ASC_CB_RETRY) {
                do_retry = 1;
            } else if (action == ASC_CB_ABORT) {
                _async_socket_connector_close_socket(connector);
            }
            break;

        case ASYNC_ERROR:
            loopIo_done(connector->connector_io);
            D("Error %d while connecting to socket %s: %s",
              errno, _asc_socket_string(connector), strerror(errno));
            /* Invoke "on connected" callback */
            action = connector->on_connected_cb(connector->on_connected_cb_opaque,
                                                connector, ASC_CONNECTION_FAILED);
            if (action == ASC_CB_RETRY) {
                do_retry = 1;
            } else if (action == ASC_CB_ABORT) {
                _async_socket_connector_close_socket(connector);
            }
            break;

        case ASYNC_NEED_MORE:
            return;
    }

    if (do_retry) {
        D("Retrying connection to socket %s", _asc_socket_string(connector));
        loopTimer_startRelative(connector->connector_timer, connector->retry_to);
    } else {
        _async_socket_connector_free(connector);
    }
}

static void
_on_async_socket_connector_io(void* opaque, int fd, unsigned events)
{
    AsyncSocketConnector* const connector = (AsyncSocketConnector*)opaque;

    /* Complete socket connection. */
    const AsyncStatus status = asyncConnector_run(connector->connector);
    _on_async_socket_connector_connecting(connector, status);
}

/* Retry connection timer callback.
 * Param:
 *  opaque - AsyncSocketConnector instance.
 */
static void
_on_async_socket_connector_retry(void* opaque)
{
    AsyncSocketConnector* const connector = (AsyncSocketConnector*)opaque;

    /* Invoke the callback to notify about a connection retry attempt. */
    const ASCCbRes action =
        connector->on_connected_cb(connector->on_connected_cb_opaque,
                                   connector, ASC_CONNECTION_RETRY);

    if (action == ASC_CB_RETRY) {
        AsyncStatus status;

        /* Close handle opened for the previous (failed) attempt. */
        _async_socket_connector_close_socket(connector);

        /* Retry connection attempt. */
        if (_async_socket_connector_open_socket(connector) == 0) {
            status = asyncConnector_init(connector->connector, &connector->address,
                                         connector->connector_io);
        } else {
            status = ASYNC_ERROR;
        }

        _on_async_socket_connector_connecting(connector, status);
    } else {
        _async_socket_connector_free(connector);
    }
}

/********************************************************************************
 *                       Async connector implementation
 *******************************************************************************/

AsyncSocketConnector*
async_socket_connector_new(const SockAddress* address,
                           int retry_to,
                           asc_event_cb cb,
                           void* cb_opaque)
{
    AsyncSocketConnector* connector;

    if (cb == NULL) {
        W("No callback for AsyncSocketConnector for %s",
          sock_address_to_string(address));
        errno = EINVAL;
        return NULL;
    }

    ANEW0(connector);

    connector->fd = -1;
    connector->retry_to = retry_to;
    connector->on_connected_cb = cb;
    connector->on_connected_cb_opaque = cb_opaque;

    /* Copy socket address. */
    if (sock_address_get_family(address) == SOCKET_UNIX) {
        sock_address_init_unix(&connector->address, sock_address_get_path(address));
    } else {
        connector->address = *address;
    }

    /* Create a looper for asynchronous I/O. */
    connector->looper = looper_newCore();
    if (connector->looper != NULL) {
        /* Create a timer that will be used for connection retries. */
        loopTimer_init(connector->connector_timer, connector->looper,
                       _on_async_socket_connector_retry, connector);
    } else {
        E("Unable to create I/O looper for asynchronous connector to socket %s",
          _asc_socket_string(connector));
        _async_socket_connector_free(connector);
        return NULL;
    }

    return connector;
}

ASCConnectRes
async_socket_connector_connect(AsyncSocketConnector* connector)
{
    AsyncStatus status;

    if (_async_socket_connector_open_socket(connector) == 0) {
        status = asyncConnector_init(connector->connector, &connector->address,
                                     connector->connector_io);
    } else {
        status = ASYNC_ERROR;
    }

    _on_async_socket_connector_connecting(connector, status);

    switch (status) {
        case ASYNC_COMPLETE:
            return ASC_CONNECT_SUCCEEDED;

        case ASYNC_ERROR:
            return ASC_CONNECT_FAILED;

        case ASYNC_NEED_MORE:
        default:
            return ASC_CONNECT_IN_PROGRESS;
    }
}

int
async_socket_connector_pull_fd(AsyncSocketConnector* connector)
{
    const int fd = connector->fd;
    if (fd >= 0) {
        connector->fd = -1;
    }
    return fd;
}

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
    /* Callback to invoke on connection events. */
    asc_event_cb    on_connected_cb;
    /* An opaque parameter to pass to the connection callback. */
    void*           on_connected_cb_opaque;
    /* Retry timeout in milliseconds. */
    int             retry_to;
    /* Socket descriptor for the connection. */
    int             fd;
    /* Number of outstanding references to the connector. */
    int             ref_count;
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
        D("%s: Connector is destroyed.", _asc_socket_string(connector));
        if (asyncConnector_stop(connector->connector) == 0) {
            /* Connection was in progress. We need to destroy I/O descriptor for
             * that connection. */
            D("%s: Stopped async connection in progress.",
              _asc_socket_string(connector));
            loopIo_done(connector->connector_io);
        }
        if (connector->fd >= 0) {
            socket_close(connector->fd);
        }

        if (connector->looper != NULL) {
            loopTimer_stop(connector->connector_timer);
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
        D("%s: Unable to create socket: %d -> %s",
          _asc_socket_string(connector), errno, strerror(errno));
        return -1;
    }

    /* Prepare for async I/O on the connector. */
    socket_set_nonblock(connector->fd);

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
 * Param:
 *  connector - Initialized AsyncSocketConnector instance. Note: When this
 *      callback is called, the caller has referenced passed connector object,
 *      So, it's guaranteed that this connector is not going to be destroyed
 *      while this routine executes.
 *  status - Status of the connection attempt.
 */
static void
_on_async_socket_connector_connecting(AsyncSocketConnector* connector,
                                      AsyncStatus status)
{
    AsyncIOAction action = ASIO_ACTION_DONE;

    switch (status) {
        case ASYNC_COMPLETE:
            loopIo_done(connector->connector_io);
            D("Socket '%s' is connected", _asc_socket_string(connector));
            /* Invoke "on connected" callback */
            action = connector->on_connected_cb(connector->on_connected_cb_opaque,
                                                connector, ASIO_STATE_SUCCEEDED);
            break;

        case ASYNC_ERROR:
            loopIo_done(connector->connector_io);
            D("Error while connecting to socket '%s': %d -> %s",
              _asc_socket_string(connector), errno, strerror(errno));
            /* Invoke "on connected" callback */
            action = connector->on_connected_cb(connector->on_connected_cb_opaque,
                                                connector, ASIO_STATE_FAILED);
            break;

        case ASYNC_NEED_MORE:
            return;
    }

    if (action == ASIO_ACTION_RETRY) {
        D("%s: Retrying connection", _asc_socket_string(connector));
        loopTimer_startRelative(connector->connector_timer, connector->retry_to);
    } else if (action == ASIO_ACTION_ABORT) {
        D("%s: AsyncSocketConnector client for socket '%s' has aborted connection",
          __FUNCTION__, _asc_socket_string(connector));
    }
}

static void
_on_async_socket_connector_io(void* opaque, int fd, unsigned events)
{
    AsyncSocketConnector* const connector = (AsyncSocketConnector*)opaque;

    /* Reference the connector while we're handing I/O. */
    async_socket_connector_reference(connector);

    /* Notify the client that another connection attempt is about to start. */
    const AsyncIOAction action =
        connector->on_connected_cb(connector->on_connected_cb_opaque,
                                   connector, ASIO_STATE_CONTINUES);
    if (action != ASIO_ACTION_ABORT) {
        /* Complete socket connection. */
        const AsyncStatus status = asyncConnector_run(connector->connector);
        _on_async_socket_connector_connecting(connector, status);
    } else {
        D("%s: AsyncSocketConnector client for socket '%s' has aborted connection",
          __FUNCTION__, _asc_socket_string(connector));
    }

    /* Release the connector after we're done with handing I/O. */
    async_socket_connector_release(connector);
}

/* Retry connection timer callback.
 * Param:
 *  opaque - AsyncSocketConnector instance.
 */
static void
_on_async_socket_connector_retry(void* opaque)
{
    AsyncStatus status;
    AsyncSocketConnector* const connector = (AsyncSocketConnector*)opaque;

    /* Reference the connector while we're in callback. */
    async_socket_connector_reference(connector);

    /* Invoke the callback to notify about a connection retry attempt. */
    AsyncIOAction action =
        connector->on_connected_cb(connector->on_connected_cb_opaque,
                                   connector, ASIO_STATE_RETRYING);

    if (action != ASIO_ACTION_ABORT) {
        /* Close handle opened for the previous (failed) attempt. */
        _async_socket_connector_close_socket(connector);

        /* Retry connection attempt. */
        if (_async_socket_connector_open_socket(connector) == 0) {
            loopIo_init(connector->connector_io, connector->looper,
                        connector->fd, _on_async_socket_connector_io, connector);
            status = asyncConnector_init(connector->connector,
                                         &connector->address,
                                         connector->connector_io);
        } else {
            status = ASYNC_ERROR;
        }

        _on_async_socket_connector_connecting(connector, status);
    } else {
        D("%s: AsyncSocketConnector client for socket '%s' has aborted connection",
          __FUNCTION__, _asc_socket_string(connector));
    }

    /* Release the connector after we're done with the callback. */
    async_socket_connector_release(connector);
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
        W("No callback for AsyncSocketConnector for socket '%s'",
          sock_address_to_string(address));
        errno = EINVAL;
        return NULL;
    }

    ANEW0(connector);

    connector->fd = -1;
    connector->retry_to = retry_to;
    connector->on_connected_cb = cb;
    connector->on_connected_cb_opaque = cb_opaque;
    connector->ref_count = 1;

    /* Copy socket address. */
    if (sock_address_get_family(address) == SOCKET_UNIX) {
        sock_address_init_unix(&connector->address, sock_address_get_path(address));
    } else {
        connector->address = *address;
    }

    /* Create a looper for asynchronous I/O. */
    connector->looper = looper_newCore();
    if (connector->looper == NULL) {
        E("Unable to create I/O looper for AsyncSocketConnector for socket '%s'",
          _asc_socket_string(connector));
        cb(cb_opaque, connector, ASIO_STATE_FAILED);
        _async_socket_connector_free(connector);
        return NULL;
    }

    /* Create a timer that will be used for connection retries. */
    loopTimer_init(connector->connector_timer, connector->looper,
                   _on_async_socket_connector_retry, connector);

    return connector;
}

int
async_socket_connector_reference(AsyncSocketConnector* connector)
{
    assert(connector->ref_count > 0);
    connector->ref_count++;
    return connector->ref_count;
}

int
async_socket_connector_release(AsyncSocketConnector* connector)
{
    assert(connector->ref_count > 0);
    connector->ref_count--;
    if (connector->ref_count == 0) {
        /* Last reference has been dropped. Destroy this object. */
        _async_socket_connector_free(connector);
        return 0;
    }
    return connector->ref_count;
}

void
async_socket_connector_connect(AsyncSocketConnector* connector)
{
    AsyncStatus status;

    if (_async_socket_connector_open_socket(connector) == 0) {
        const AsyncIOAction action =
            connector->on_connected_cb(connector->on_connected_cb_opaque,
                                       connector, ASIO_STATE_STARTED);
        if (action == ASIO_ACTION_ABORT) {
            D("%s: AsyncSocketConnector client for socket '%s' has aborted connection",
              __FUNCTION__, _asc_socket_string(connector));
            return;
        } else {
            loopIo_init(connector->connector_io, connector->looper,
                        connector->fd, _on_async_socket_connector_io, connector);
            status = asyncConnector_init(connector->connector,
                                         &connector->address,
                                         connector->connector_io);
        }
    } else {
        status = ASYNC_ERROR;
    }

    _on_async_socket_connector_connecting(connector, status);
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

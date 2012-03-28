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

#ifndef ANDROID_ASYNC_SOCKET_CONNECTOR_H_
#define ANDROID_ASYNC_SOCKET_CONNECTOR_H_

/*
 * Contains declaration of an API that allows asynchronous connection to a
 * socket with retries.
 *
 * The typical usage of this API is as such:
 *
 * 1. The client creates an async connector instance by calling async_socket_connector_new
 *    routine, supplying there address of the socket to connect, and a callback
 *    to invoke on connection events.
 * 2. The client then proceeds with calling async_socket_connector_connect that
 *    would initiate connection attempts.
 *
 * The main job on the client side falls on the client's callback routine that
 * serves the connection events. Once connection has been initiated, the connector
 * will invoke that callback to report current connection status.
 *
 * In general, there are three connection events passed to the callback:
 * 1. Success.
 * 2. Failure.
 * 3. Retry.
 *
 * Typically, when client's callback is called for successful connection, the
 * client will pull connected socket's FD from the connector, and then this FD
 * will be used by the client for I/O on the connected socket. If socket's FD
 * is pulled by the client, it must return ASC_CB_KEEP from the callback.
 *
 * When client's callback is invoked with an error (ASC_CONNECTION_FAILED event),
 * the client has an opportunity to review the error (available in 'errno'), and
 * either abort the connection by returning ASC_CB_ABORT, or schedule a retry
 * by returning ASC_CB_RETRY from the callback. If client returns ASC_CB_ABORT
 * from the callback, the connector will stop connection attempts, and will
 * self-destruct. If ASC_CB_RETRY is returned from the callback, the connector
 * will retry connection attempt after timeout that was set by the caller in the
 * call to async_socket_connector_new routine.
 *
 * When client's callback is invoked with ASC_CONNECTION_RETRY, the client has an
 * opportunity to cancel further connection attempts by returning ASC_CB_ABORT,
 * or it can allow another connection attempt by returning ASC_CB_RETRY.
 *
 * The client has no control over the lifespan of initialized connector instance.
 * It always self-destructs after client's cllback returns with a status other
 * than ASC_CB_RETRY.
 */

/* Declares async socket connector descriptor. */
typedef struct AsyncSocketConnector AsyncSocketConnector;

/* Enumerates connection events.
 * Values from this enum are passed to the callback that connector's client uses
 * to monitor connection status / progress.
 */
typedef enum ASCEvent {
    /* Connection with the socket has been successfuly established. */
    ASC_CONNECTION_SUCCEEDED,
    /* A failure has occured while establising connection, with errno containing
     * the actual error. */
    ASC_CONNECTION_FAILED,
    /* Async socket connector is about to retry the connection. */
    ASC_CONNECTION_RETRY,
} ASCEvent;

/* Enumerates return values from the callback to the connector's client.
 */
typedef enum ASCCbRes {
    /* Keep established connection. */
    ASC_CB_KEEP,
    /* Abort connection attempts. */
    ASC_CB_ABORT,
    /* Retry connection attempt. */
    ASC_CB_RETRY,
} ASCCbRes;

/* Enumerates values returned from the connector routine.
 */
typedef enum ASCConnectRes {
    /* Connection has succeeded in the connector routine. */
    ASC_CONNECT_SUCCEEDED,
    /* Connection has failed in the connector routine. */
    ASC_CONNECT_FAILED,
    /* Connection is in progress, and will be completed asynchronously. */
    ASC_CONNECT_IN_PROGRESS,
} ASCConnectRes;

/* Declares callback that connector's client uses to monitor connection
 * status / progress.
 * Param:
 *  opaque - An opaque pointer associated with the client.
 *  connector - Connector instance for thecallback.
 *  event - Event that has occured. If event is set to ASC_CONNECTION_FAILED,
 *      errno contains connection error.
 * Return:
 *  One of ASCCbRes values.
 */
typedef ASCCbRes (*asc_event_cb)(void* opaque,
                                 AsyncSocketConnector* connector,
                                 ASCEvent event);

/* Creates and initializes AsyncSocketConnector instance.
 * Param:
 *  address - Initialized socket address to connect to.
 *  retry_to - Retry timeout in milliseconds.
 *  cb, cb_opaque - Callback to invoke on connection events. This callback is
 *      required, and must not be NULL.
 * Return:
 *  Initialized AsyncSocketConnector instance. Note that AsyncSocketConnector
 *  instance returned from this routine will be destroyed by the connector itself,
 *  when its work on connecting to the socket is completed. Typically, the
 * connector wil destroy the descriptor after client's callback routine returns
 * with the status other than ASC_CB_RETRY.
 */
extern AsyncSocketConnector* async_socket_connector_new(const SockAddress* address,
                                                        int retry_to,
                                                        asc_event_cb cb,
                                                        void* cb_opaque);

/* Initiates asynchronous connection.
 * Param:
 *  connector - Initialized AsyncSocketConnector instance.
 * Return:
 *  Status indicating state of the connection: completed, failed, or in progress.
 *  Note that the connector will always invoke a callback passed to the
 *  async_socket_connector_new routine prior to exiting from this routine with
 *  statuses other ASC_CONNECT_IN_PROGRESS.
 */
extern ASCConnectRes async_socket_connector_connect(AsyncSocketConnector* connector);

/* Pulls socket's file descriptor from the connector.
 * This routine should be called from the connection callback on successful
 * connection status. This will provide the connector's client with operational
 * socket FD, and at the same time this will tell the connector not to close
 * the FD when connector descriptor gets destroyed.
 * Param:
 *  connector - Initialized AsyncSocketConnector instance.
 * Return:
 *  File descriptor for the connected socket.
 */
extern int async_socket_connector_pull_fd(AsyncSocketConnector* connector);

#endif  /* ANDROID_ASYNC_SOCKET_CONNECTOR_H_ */

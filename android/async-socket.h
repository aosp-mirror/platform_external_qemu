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

#ifndef ANDROID_ASYNC_SOCKET_H_
#define ANDROID_ASYNC_SOCKET_H_

/*
 * Contains declaration of an API that encapsulates communication via
 * asynchronous socket.
 *
 * This is pretty basic API that allows to asynchronously connect to a socket,
 * and perform asynchronous read from / write to the connected socket.
 *
 * Since all the operations (including connection) are asynchronous, all the
 * operation results are reported back to the client of this API via set of
 * callbacks that client supplied to this API.
 */

/* Declares asynchronous socket descriptor. */
typedef struct AsyncSocket AsyncSocket;

/* Enumerates asynchronous socket connection statuses.
 * Values enumerated here are passed to the client's callback that was set to
 * monitor socket connection.
 */
typedef enum ASConnectStatus {
    /* Socket has been connected. */
    ASCS_CONNECTED,
    /* Socket has been disconnected. */
    ASCS_DISCONNECTED,
    /* An error has occured while connecting to the socket. */
    ASCS_FAILURE,
    /* An attempt to retry connection is about to begin. */
    ASCS_RETRY,
} ASConnectStatus;

/* Enumerates values returned from the client's callback that was set to
 * monitor socket connection.
 */
typedef enum ASConnectAction {
    /* Keep the connection. */
    ASCA_KEEP,
    /* Retry connection attempt. */
    ASCA_RETRY,
    /* Abort the connection. */
    ASCA_ABORT,
} ASConnectAction;

/* Defines client's callback set to monitor socket connection.
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  as - Initialized AsyncSocket instance.
 *  status - Socket connection status.
 * Return:
 *  One of the ASConnectAction values.
 */
typedef ASConnectAction (*on_as_connection_cb)(void* client_opaque,
                                               AsyncSocket* as,
                                               ASConnectStatus status);

/* Defines client's callback set to monitor socket I/O failures.
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  as - Initialized AsyncSocket instance.
 *  is_io_read - I/O type selector: 1 - read, 0 - write.
 *  io_opaque - An opaque pointer associated with the I/O that has failed.
 *  buffer - Address of the I/O buffer.
 *  transferred - Number of bytes that were transferred before I/O has failed.
 *  to_transfer - Number of bytes initially requested to transfer with the
 *      failed I/O.
 *  failure - Error that occured (errno value)
 */
typedef void (*on_as_io_failure_cb)(void* client_opaque,
                                    AsyncSocket* as,
                                    int is_io_read,
                                    void* io_opaque,
                                    void* buffer,
                                    uint32_t transferred,
                                    uint32_t to_transfer,
                                    int failure);

/* Defines client's callback invoked when I/O has been completed.
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  as - Initialized AsyncSocket instance.
 *  is_io_read - I/O type selector: 1 - read, 0 - write.
 *  io_opaque - An opaque pointer associated with the I/O that has been
 *      completed.
 *  buffer - Address of the I/O buffer.
 *  transferred - Number of bytes that were transferred.
 */
typedef void (*on_as_io_completed_cb)(void* client_opaque,
                                      AsyncSocket* as,
                                      int is_io_read,
                                      void* io_opaque,
                                      void* buffer,
                                      uint32_t transferred);

/* Defines client's callback invoked when an I/O gets cancelled (due to a
 * disconnection, for instance).
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  as - Initialized AsyncSocket instance.
 *  is_io_read - I/O type selector: 1 - read, 0 - write.
 *  io_opaque - An opaque pointer associated with the I/O that has been
 *      cancelled.
 *  buffer - Address of the I/O buffer.
 *  transferred - Number of bytes that were transferred before I/O has been
 *      cancelled.
 *  to_transfer - Number of bytes initially requested to transfer with the
 *  cancelled I/O.
 */
typedef void (*on_as_io_cancelled_cb)(void* client_opaque,
                                      AsyncSocket* as,
                                      int is_io_read,
                                      void* io_opaque,
                                      void* buffer,
                                      uint32_t transferred,
                                      uint32_t to_transfer);

/* Defines client's callback invoked when an I/O gets timed out.
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  as - Initialized AsyncSocket instance.
 *  is_io_read - I/O type selector: 1 - read, 0 - write.
 *  io_opaque - An opaque pointer associated with the I/O that has timed out.
 *  buffer - Address of the I/O buffer.
 *  transferred - Number of bytes that were transferred before I/O has timed out.
 *  to_transfer - Number of bytes initially requested to transfer with the timed
 *  out I/O.
 */
typedef void (*on_as_io_timed_out_cb)(void* client_opaque,
                                      AsyncSocket* as,
                                      int is_io_read,
                                      void* io_opaque,
                                      void* buffer,
                                      uint32_t transferred,
                                      uint32_t to_transfer);

/* Lists asynchronous socket I/O callbacks. */
typedef struct ASIOCb {
    on_as_io_completed_cb   on_completed;
    on_as_io_cancelled_cb   on_cancelled;
    on_as_io_timed_out_cb   on_timed_out;
    on_as_io_failure_cb     on_io_failure;
} ASIOCb;


/* Lists asynchronous socket client callbacks. */
typedef struct ASClientCb {
    /* Connection callback (client must have one) */
    on_as_connection_cb     on_connection;
    /* Optional client-level I/O callbacks. */
    const ASIOCb*           io_cb;
} ASClientCb;

/* Creates an asynchronous socket descriptor.
 * Param:
 *  port - TCP port to connect the socket to.
 *  reconnect_to - Timeout before retrying to reconnect after disconnection.
 *      0 means "don't try to reconnect".
 *  client_callbacks - Lists socket client callbacks.
 *  client_opaque - An opaque pointer to associate with the socket client.
 * Return:
 *  Initialized AsyncSocket instance on success, or NULL on failure.
 */
extern AsyncSocket* async_socket_new(int port,
                                     int reconnect_to,
                                     const ASClientCb* client_callbacks,
                                     void* client_opaque);

/* Asynchronously connects to an asynchronous socket.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  retry_to - Number of milliseconds to wait before retrying a failed
 *      connection.
 * Return:
 *  0 on success, or -1 on failure. If this routine returns a failure, I/O
 *  failure callback has not been invoked.
 */
extern int async_socket_connect(AsyncSocket* as, int retry_to);

/* Disconnects from an asynchronous socket.
 * Param:
 *  as - Initialized and connected AsyncSocket instance.
 */
extern void async_socket_disconnect(AsyncSocket* as);

/* Asynchronously reconnects to an asynchronous socket.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  retry_to - Number of milliseconds to wait before retrying to reconnect.
 * Return:
 *  0 on success, or -1 on failure. If this routine returns a failure, I/O
 *  failure callback has not been invoked.
 */
extern int async_socket_reconnect(AsyncSocket* as, int retry_to);

/* Asynchronously reads data from an asynchronous socket with a deadline.
 * Param:
 *  as - Initialized and connected AsyncSocket instance.
 *  buffer, len - Buffer where to read data.
 *  reader_cb - Lists reader's callbacks.
 *  reader_opaque - An opaque pointer associated with the reader.
 *  deadline - Deadline to complete the read.
 * Return:
 *  0 on success, or -1 on failure. If this routine returns a failure, I/O
 *  failure callback has not been invoked.
 */
extern int async_socket_read_abs(AsyncSocket* as,
                                 void* buffer, uint32_t len,
                                 const ASIOCb* reader_cb,
                                 void* reader_opaque,
                                 Duration deadline);

/* Asynchronously reads data from an asynchronous socket with a relative timeout.
 * Param:
 *  as - Initialized and connected AsyncSocket instance.
 *  buffer, len - Buffer where to read data.
 *  reader_cb - Lists reader's callbacks.
 *  reader_opaque - An opaque pointer associated with the reader.
 *  to - Milliseconds to complete the read. to < 0 indicates "no timeout"
 * Return:
 *  0 on success, or -1 on failure. If this routine returns a failure, I/O
 *  failure callback has not been invoked.
 */
extern int async_socket_read_rel(AsyncSocket* as,
                                 void* buffer, uint32_t len,
                                 const ASIOCb* reader_cb,
                                 void* reader_opaque,
                                 int to);

/* Asynchronously writes data to an asynchronous socket with a deadline.
 * Param:
 *  as - Initialized and connected AsyncSocket instance.
 *  buffer, len - Buffer with writing data.
 *  writer_cb - Lists writer's callbacks.
 *  writer_opaque - An opaque pointer associated with the writer.
 *  deadline - Deadline to complete the write.
 * Return:
 *  0 on success, or -1 on failure. If this routine returns a failure, I/O
 *  failure callback has not been invoked.
 */
extern int async_socket_write_abs(AsyncSocket* as,
                                 const void* buffer, uint32_t len,
                                 const ASIOCb* writer_cb,
                                 void* writer_opaque,
                                 Duration deadline);

/* Asynchronously writes data to an asynchronous socket with a relative timeout.
 * Param:
 *  as - Initialized and connected AsyncSocket instance.
 *  buffer, len - Buffer with writing data.
 *  writer_cb - Lists writer's callbacks.
 *  writer_opaque - An opaque pointer associated with the writer.
 *  to - Milliseconds to complete the write. to < 0 indicates "no timeout"
 * Return:
 *  0 on success, or -1 on failure. If this routine returns a failure, I/O
 *  failure callback has not been invoked.
 */
extern int async_socket_write_rel(AsyncSocket* as,
                                 const void* buffer, uint32_t len,
                                 const ASIOCb* writer_cb,
                                 void* writer_opaque,
                                 int to);

#endif  /* ANDROID_ASYNC_SOCKET_H_ */

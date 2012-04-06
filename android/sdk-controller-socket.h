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

#ifndef ANDROID_SDKCONTROL_SOCKET_H_
#define ANDROID_SDKCONTROL_SOCKET_H_

#include "android/async-socket.h"

/*
 * Contains declaration of an API that encapsulates communication protocol with
 * SdkController running on an Android device.
 *
 * SdkController is used to provide:
 *
 * - Realistic sensor emulation.
 * - Multi-touch emulation.
 * - Open for other types of emulation.
 *
 * The idea behind this type of emulation is such that there is an actual
 * Android device that is connected via USB to the host running the emulator.
 * On the device there is an SdkController service running that enables
 * communication between an Android application that gathers information required
 * by the emulator, and transmits that info to the emulator.
 *
 * SdkController service on the device, and SDKCtlSocket API implemented here
 * implement the exchange protocol between an Android application, and emulation
 * engine running inside the emulator.
 *
 * In turn, the exchange protocol is implemented on top of asynchronous socket
 * communication (abstracted in AsyncSocket protocol implemented in
 * android/async-socket.*). It means that connection, and all data transfer
 * (both, in, and out) are completely asynchronous, and results of each operation
 * are reported through callbacks.
 *
 * Essentially, this entire API implements two types of protocols:
 *
 * - Connection protocol.
 * - Data exchange protocol.
 *
 * 1. Connection protocol.
 *
 * Connecting to SdkController service on the attached device can be broken down
 * into two phases:
 * - Connecting to a TCP socket.
 * - Sending a "handshake" query to the SdkController.
 *
 * 1.1. Connecting to the socket.
 *
 * TCP socket connection with SdkController is enabled by using adb port
 * forwarding. SdkController is always listening to a local abstract socket named
 * 'android.sdk.controller', so to enable connecting to it from the host, run
 *
 *   adb forward tcp:<port> localabstract: android.sdk.controller
 *
 * After that TCP socket for the requested port can be used to connect to
 * SdkController, and connecting to it is no different than connecting to any
 * socket server. Except for one thing: adb port forwarding is implemented in
 * such a way, that socket_connect will always succeed, even if there is no
 * server listening to that port on the other side of connection. Moreover,
 * even socked_send will succeed in this case, so the only way to ensure that
 * SdkController in deed is listening is to exchange a handshake with it:
 * Fortunatelly, an attempt to read from forwarded TCP port on condition that
 * there is no listener on the oher side will fail.
 *
 * 1.2. Handshake query.
 *
 * Handshake query is a special type of query that SDKCtlSocket sends to the
 * SdkController upon successful socket connection. This query served two
 * purposes:
 * - Informs the SdkController about host endianness. This information is
 *   important, because SdkController needs it in order to format its messages
 *   with proper endianness.
 * - Ensures that SdkController is in deed listening on the other side of the
 *   connected socket.
 *
 * Connection with SdkController is considered to be successfuly established when
 * SdkController responds to the handshake query, thus, completing the connection.
 *
 * 2. Data exchange protocol.
 *
 * As it was mentioned above, all data transfer in this API is completely
 * asynchronous, and result of each data transfer is reported via callbacks.
 * This also complicates destruction of data involved in exchange, since in an
 * asynchronous environment it's hard to control the lifespan of an object, its
 * owner, and who and when is responsible to free resources allocated for the
 * transfer. To address this issue, all the descriptors that this API operates
 * with are referenced on use / released after use, and get freed when reference
 * counter for them drops to zero, indicating that there is no component that is
 * interested in that particular descriptor.
 *
 * There are three types of data in the exchange protocol:
 * - A packet - the simplest type of data that doesn't require any replies.
 * - A query - A message that require a reply, and
 * - A query reply - A message that delivers query reply.
 */

/* Declares SDK controller socket descriptor. */
typedef struct SDKCtlSocket SDKCtlSocket;

/* Declares SDK controller data packet descriptor. */
typedef struct SDKCtlPacket SDKCtlPacket;

/* Declares SDK controller query descriptor. */
typedef struct SDKCtlQuery SDKCtlQuery;

/* Defines client's callback set to monitor SDK controller socket connection.
 *
 * SDKCtlSocket will invoke this callback when connection to TCP port is
 * established, but before handshake query is processed. The client should use
 * on_sdkctl_handshake_cb to receive notification about an operational connection
 * with SdkController.
 *
 * The main purpose of this callback for the client is to monitor connection
 * state: in addition to TCP port connection, this callback will be invoked when
 * connection with the port is lost.
 *
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  sdkctl - Initialized SDKCtlSocket instance.
 *  status - Socket connection status.  Can be one of these:
 *    - ASIO_STATE_SUCCEEDED : Socket is connected to the port.
 *    - ASIO_STATE_FAILED    : Connection attempt has failed, or connection with
 *                             the port is lost.
 * Return:
 *  One of the AsyncIOAction values.
 */
typedef AsyncIOAction (*on_sdkctl_connection_cb)(void* client_opaque,
                                                 SDKCtlSocket* sdkctl,
                                                 AsyncIOState status);

/* Defines client's callback set to receive handshake reply from the SdkController
 * service running on the device.
 *
 * Successful handshake means that connection between the client and SDK
 * controller service has been established.
 *
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  sdkctl - Initialized SDKCtlSocket instance.
 *  handshake - Handshake message received from the SDK controller service.
 *  handshake_size - Size of the fandshake message received from the SDK
 *      controller service.
 *  status - Handshake status. Note that handshake, and handshake_size are valid
 *      only if this parameter is set to ASIO_STATE_SUCCEEDED.
 */
typedef void (*on_sdkctl_handshake_cb)(void* client_opaque,
                                       SDKCtlSocket* sdkctl,
                                       void* handshake,
                                       uint32_t handshake_size,
                                       AsyncIOState status);

/* Defines a message notification callback.
 * Param:
 *  client_opaque - An opaque pointer associated with the client.
 *  sdkctl - Initialized SDKCtlSocket instance.
 *  message - Descriptor for received message. Note that message descriptor will
 *      be released upon exit from this callback (thus, could be freed along
 *      with message data). If the client is interested in working with that
 *      message after the callback returns, it should reference the message
 *      descriptor in this callback.
 *  msg_type - Message type.
 *  msg_data, msg_size - Message data.
 */
typedef void (*on_sdkctl_message_cb)(void* client_opaque,
                                     SDKCtlSocket* sdkctl,
                                     SDKCtlPacket* message,
                                     int msg_type,
                                     void* msg_data,
                                     int msg_size);

/* Defines query completion callback.
 * Param:
 *  query_opaque - An opaque pointer associated with the query by the client.
 *  query - Query descriptor.  Note that query descriptor will be released upon
 *      exit from this callback (thus, could be freed along with query data). If
 *      the client is interested in working with that query after the callback
 *      returns, it should reference the query descriptor in this callback.
 *  status - Query status. Can be one of these:
 *    - ASIO_STATE_CONTINUES : Query data has been transmitted to the service,
 *                             and query is now waiting for response.
 *    - ASIO_STATE_SUCCEEDED : Query has been successfully completed.
 *    - ASIO_STATE_FAILED    : Query has failed on an I/O.
 *    - ASIO_STATE_TIMED_OUT : Deadline set for the query has expired.
 *    - ASIO_STATE_CANCELLED : Query has been cancelled due to socket
 *                             disconnection.
 * Return:
 *  One of the AsyncIOAction values.
 */
typedef AsyncIOAction (*on_sdkctl_query_cb)(void* query_opaque,
                                            SDKCtlQuery* query,
                                            AsyncIOState status);

/********************************************************************************
 *                         SDKCtlPacket API
 ********************************************************************************/

/* References SDKCtlPacket object.
 * Param:
 *  packet - Initialized SDKCtlPacket instance.
 * Return:
 *  Number of outstanding references to the object.
 */
extern int sdkctl_packet_reference(SDKCtlPacket* packet);

/* Releases SDKCtlPacket object.
 * Note that upon exiting from this routine the object might be destroyed, even
 * if this routine returns value other than zero.
 * Param:
 *  packet - Initialized SDKCtlPacket instance.
 * Return:
 *  Number of outstanding references to the object.
 */
extern int sdkctl_packet_release(SDKCtlPacket* packet);

/********************************************************************************
 *                          SDKCtlQuery API
 ********************************************************************************/

/* Creates, and partially initializes query descriptor.
 * Note that returned descriptor is referenced, and it must be eventually
 * released with a call to sdkctl_query_release.
 * Param:
 *  sdkctl - SDKCtlSocket instance for the query.
 *  query_type - Defines query type.
 *  in_data_size Size of the query's input buffer (data to be sent with this
 *      query). Note that buffer for query data will be allocated along with the
 *      query descriptor. Use sdkctl_query_get_buffer_in to get address of data
 *      buffer for this query.
 * Return:
 *  Referenced SDKCtlQuery descriptor.
 */
extern SDKCtlQuery* sdkctl_query_new(SDKCtlSocket* sdkctl,
                                     int query_type,
                                     uint32_t in_data_size);

/* Creates, and fully initializes query descriptor.
 * Note that returned descriptor is referenced, and it must be eventually
 * released with a call to sdkctl_query_release.
 * Param:
 *  sdkctl - SDKCtlSocket instance for the query.
 *  query_type - Defines query type.
 *  in_data_size Size of the query's input buffer (data to be sent with this
 *      query). Note that buffer for query data will be allocated along with the
 *      query descriptor. Use sdkctl_query_get_buffer_in to get address of data
 *      buffer for this query.
 *  in_data - Data to initialize query's input buffer with.
 *  response_buffer - Contains address of the buffer addressing preallocated
 *      response buffer on the way in, and address of the buffer containing query
 *      response on query completion. If this parameter is NULL, the API will
 *      allocate its own query response buffer, which is going to be freed after
 *      query completion.
 *  response_size - Contains size of preallocated response buffer on the way in,
 *      and size of the received response on query completion. Can be NULL.
 *  query_cb - A callback to monitor query state.
 *  query_opaque - An opaque pointer associated with the query.
 * Return:
 *  Referenced SDKCtlQuery descriptor.
 */
extern SDKCtlQuery* sdkctl_query_new_ex(SDKCtlSocket* sdkctl,
                                        int query_type,
                                        uint32_t in_data_size,
                                        const void* in_data,
                                        void** response_buffer,
                                        uint32_t* response_size,
                                        on_sdkctl_query_cb query_cb,
                                        void* query_opaque);

/* Sends query to SDK controller.
 * Param:
 *  query - Query to send. Note that this must be a fully initialized query
 *      descriptor.
 *  to - Milliseconds to allow for the query to complete. Negative value means
 *  "forever".
 */
extern void sdkctl_query_send(SDKCtlQuery* query, int to);

/* Creates, fully initializes query descriptor, and sends the query to SDK
 * controller.
 * Note that returned descriptor is referenced, and it must be eventually
 * released with a call to sdkctl_query_release.
 * Param:
 *  sdkctl - SDKCtlSocket instance for the query.
 *  query_type - Defines query type.
 *  in_data_size Size of the query's input buffer (data to be sent with this
 *      query). Note that buffer for query data will be allocated along with the
 *      query descriptor. Use sdkctl_query_get_buffer_in to get address of data
 *      buffer for this query.
 *  in_data - Data to initialize query's input buffer with.
 *  response_buffer - Contains address of the buffer addressing preallocated
 *      response buffer on the way in, and address of the buffer containing query
 *      response on query completion. If this parameter is NULL, the API will
 *      allocate its own query response buffer, which is going to be freed after
 *      query completion.
 *  response_size - Contains size of preallocated response buffer on the way in,
 *      and size of the received response on query completion. Can be NULL.
 *  query_cb - A callback to monitor query state.
 *  query_opaque - An opaque pointer associated with the query.
 *  to - Milliseconds to allow for the query to complete. Negative value means
 *  "forever".
 * Return:
 *  Referenced SDKCtlQuery descriptor for the query that has been sent.
 */
extern SDKCtlQuery* sdkctl_query_build_and_send(SDKCtlSocket* sdkctl,
                                                int query_type,
                                                uint32_t in_data_size,
                                                const void* in_data,
                                                void** response_buffer,
                                                uint32_t* response_size,
                                                on_sdkctl_query_cb query_cb,
                                                void* query_opaque,
                                                int to);

/* References SDKCtlQuery object.
 * Param:
 *  query - Initialized SDKCtlQuery instance.
 * Return:
 *  Number of outstanding references to the object.
 */
extern int sdkctl_query_reference(SDKCtlQuery* query);

/* Releases SDKCtlQuery object.
 * Note that upon exit from this routine the object might be destroyed, even if
 * this routine returns value other than zero.
 * Param:
 *  query - Initialized SDKCtlQuery instance.
 * Return:
 *  Number of outstanding references to the object.
 */
extern int sdkctl_query_release(SDKCtlQuery* query);

/* Gets address of query's input data buffer (data to be send).
 * Param:
 *  query - Query to get data buffer for.
 * Return:
 *  Address of query's input data buffer.
 */
extern void* sdkctl_query_get_buffer_in(SDKCtlQuery* query);

/********************************************************************************
 *                          SDKCtlSocket API
 ********************************************************************************/

/* Creates an SDK controller socket descriptor.
 * Param:
 *  reconnect_to - Timeout before trying to reconnect, or retry connection
 *      attempts after disconnection, or on connection failures.
 *  service_name - Name of the SdkController service for this socket (such as
 *      'sensors', 'milti-touch', etc.)
 *  on_connection - A callback to invoke on socket connection events.
 *  on_handshake - A callback to invoke on handshake events.
 *  on_message - A callback to invoke when a message is received from the SDK
 *      controller.
 *  opaque - An opaque pointer to associate with the socket.
 * Return:
 *  Initialized SDKCtlSocket instance on success, or NULL on failure.
 */
extern SDKCtlSocket* sdkctl_socket_new(int reconnect_to,
                                       const char* service_name,
                                       on_sdkctl_connection_cb on_connection,
                                       on_sdkctl_handshake_cb on_handshake,
                                       on_sdkctl_message_cb on_message,
                                       void* opaque);

/* Improves throughput by recycling memory allocated for buffers transferred via
 * this API.
 *
 * In many cases data exchanged between SDK controller sides are small, and,
 * although may come quite often, are coming in a sequential manner. For instance,
 * sensor service on the device may send us sensor updates every 20ms, one after
 * another. For such data traffic it makes perfect sense to recycle memory
 * allocated for the previous sensor update, rather than to free it just to
 * reallocate same buffer in 20ms. This routine sets properties of the recycler
 * for the given SDK controller connection. Recycling includes memory allocated
 * for all types of data transferred in this API: packets, and queries.
 * Param:
 *  sdkctl - Initialized SDKCtlSocket instance.
 *  data_size - Size of buffer to allocate for each data block.
 *  max_recycled_num - Maximum number of allocated buffers to keep in the
 *      recycler.
 */
extern void sdkctl_init_recycler(SDKCtlSocket* sdkctl,
                                 uint32_t data_size,
                                 int max_recycled_num);

/* References SDKCtlSocket object.
 * Param:
 *  sdkctl - Initialized SDKCtlSocket instance.
 * Return:
 *  Number of outstanding references to the object.
 */
extern int sdkctl_socket_reference(SDKCtlSocket* sdkctl);

/* Releases SDKCtlSocket object.
 * Note that upon exit from this routine the object might be destroyed, even if
 * this routine returns value other than zero.
 * Param:
 *  sdkctl - Initialized SDKCtlSocket instance.
 * Return:
 *  Number of outstanding references to the object.
 */
extern int sdkctl_socket_release(SDKCtlSocket* sdkctl);

/* Asynchronously connects to SDK controller.
 * Param:
 *  sdkctl - Initialized SDKCtlSocket instance.
 *  port - TCP port to connect the socket to.
 *  retry_to - Number of milliseconds to wait before retrying a failed
 *      connection attempt.
 */
extern void sdkctl_socket_connect(SDKCtlSocket* sdkctl, int port, int retry_to);

/* Asynchronously reconnects to SDK controller.
 * Param:
 *  sdkctl - Initialized SDKCtlSocket instance.
 *  port - TCP port to reconnect the socket to.
 *  retry_to - Number of milliseconds to wait before reconnecting. Same timeout
 *      will be used for retrying a failed connection attempt.
 */
extern void sdkctl_socket_reconnect(SDKCtlSocket* sdkctl, int port, int retry_to);

/* Disconnects from SDK controller.
 * Param:
 *  sdkctl - Initialized SDKCtlSocket instance.
 */
extern void sdkctl_socket_disconnect(SDKCtlSocket* sdkctl);

#endif  /* ANDROID_SDKCONTROL_SOCKET_H_ */

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

/*
 * Encapsulates exchange protocol between the emulator, and an Android device
 * that is connected to the host via USB. The communication is established over
 * a TCP port forwarding, enabled by ADB.
 */

#include "qemu-common.h"
#include "android/async-utils.h"
#include "android/utils/debug.h"
#include "android/async-socket-connector.h"
#include "android/async-socket.h"
#include "android/sdk-controller-socket.h"
#include "utils/panic.h"
#include "iolooper.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(sdkctlsocket,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(sdkctlsocket)

#define TRACE_ON    1

#if TRACE_ON
#define  T(...)    VERBOSE_PRINT(sdkctlsocket,__VA_ARGS__)
#else
#define  T(...)
#endif

/* Recycling memory descriptor. */
typedef struct SDKCtlRecycled SDKCtlRecycled;
struct SDKCtlRecycled {
    union {
        /* Next recycled descriptor (while listed in recycler). */
        SDKCtlRecycled* next;
        /* Allocated memory size (while outside of the recycler). */
        uint32_t        size;
    };
};

/********************************************************************************
 *                      SDKCtlPacket declarations
 *******************************************************************************/

/*
 * Types of the packets of data sent via SDK controller socket.
 */

/* The packet is a message. */
#define SDKCTL_PACKET_MESSAGE           1
/* The packet is a query. */
#define SDKCTL_PACKET_QUERY             2
/* The packet is a response to a query. */
#define SDKCTL_PACKET_QUERY_RESPONSE    3

/* Data packet descriptor.
 *
 * All packets, sent and received via SDK controller socket begin with this
 * header, with packet data immediately following this header.
 */
typedef struct SDKCtlPacketHeader {
    /* Total size of the data to transfer with this packet, including this
     * header. The transferring data should immediatelly follow this header. */
    int     size;
    /* Encodes packet type. See SDKCTL_PACKET_XXX for the list of packet types
     * used by SDK controller. */
    int     type;
} SDKCtlPacketHeader;

/* Packet descriptor, allocated by this API for data packets to be sent to SDK
 * controller service on the device.
 *
 * When packet descriptors are allocated by this API, they are allocated large
 * enough to contain this header, and packet data to send to the service,
 * immediately following this descriptor.
 */
struct SDKCtlPacket {
    /* Supports recycling. Don't put anything in front: recycler expects this
     * to be the first field in recyclable descriptor. */
    SDKCtlRecycled      recycling;

    /* Next packet in the list of packets to send. */
    SDKCtlPacket*       next;
    /* SDK controller socket that transmits this packet. */
    SDKCtlSocket*       sdkctl;
    /* Number of outstanding references to the packet. */
    int                 ref_count;

    /* Common packet header. Packet data immediately follows this header, so it
     * must be last field in SDKCtlPacket descriptor. */
    SDKCtlPacketHeader  header;
};

/********************************************************************************
 *                      SDKCtlQuery declarations
 *******************************************************************************/

/*
 * Types of queries sent via SDK controller socket.
 */

/* Handshake query.
 * This query is sent to SDK controller service as part of the connection
 * protocol implementation.
 */
#define SDKCTL_QUERY_HANDSHAKE          -1

/* Query packet descriptor.
 *
 * All queries, sent and received via SDK controller socket begin with this
 * header, with query data immediately following this header.
 */
typedef struct SDKCtlQueryHeader {
    /* Data packet header for this query. */
    SDKCtlPacketHeader  packet;
    /* A unique query identifier. This ID is used to track the query in the
     * asynchronous environment in whcih SDK controller socket operates. */
    int                 query_id;
    /* Query type. See SDKCTL_QUERY_XXX for the list of query types used by SDK
     * controller. */
    int                 query_type;
} SDKCtlQueryHeader;

/* Query descriptor, allocated by this API for queries to be sent to SDK
 * controller service on the device.
 *
 * When query descriptors are allocated by this API, they are allocated large
 * enough to contain this header, and query data to send to the service,
 * immediately following this descriptor.
 */
struct SDKCtlQuery {
    /* Supports recycling. Don't put anything in front: recycler expects this
     * to be the first field in recyclable descriptor. */
    SDKCtlRecycled          recycling;

    /* Next query in the list of active, or recycled queries. */
    SDKCtlQuery*            next;
    /* A timer to run time out on this query after it has been sent. */
    LoopTimer               timer[1];
    /* Absolute time for this query's deadline. This is the value that query's
     * timer is set for after query has been transmitted to the service. */
    Duration                deadline;
    /* SDK controller socket that owns the query. */
    SDKCtlSocket*           sdkctl;
    /* A callback to invoke on query state changes. */
    on_sdkctl_query_cb      query_cb;
    /* An opaque pointer associated with this query. */
    void*                   query_opaque;
    /* Points to an address of a buffer where to save query response. */
    void**                  response_buffer;
    /* Points to a variable containing size of the response buffer (on the way in),
     * or actual query response size (when query is completed). */
    uint32_t*               response_size;
    /* Internal response buffer, allocated if query creator didn't provide its
     * own. This field is valid only if response_buffer field is NULL, or is
     * pointing to this field. */
    void*                   internal_resp_buffer;
    /* Internal response buffer size This field is valid only if response_size
     * field is NULL, or is pointing to this field. */
    uint32_t                internal_resp_size;
    /* Number of outstanding references to the query. */
    int                     ref_count;

    /* Common packet header. Query data immediately follows this header, so it
     * must be last field in SDKCtlQuery descriptor. */
    SDKCtlQueryHeader       header;
};

/* Query reply descriptor.
 *
 * All replies to a query, sent and received via SDK controller socket begin with
 * this header, with query reply data immediately following this header.
 */
typedef struct SDKCtlQueryReplyHeader {
    /* Data packet header for this reply. */
    SDKCtlPacketHeader  packet;

    /* An identifier for the query that is addressed with this reply. */
    int                 query_id;
} SDKCtlQueryReplyHeader;

/********************************************************************************
 *                      SDK Control Socket declarations
 *******************************************************************************/

/* Enumerates SDKCtlSocket states. */
typedef enum SDKCtlSocketState {
    /* Socket is disconnected from SDK controller. */
    SDKCTL_SOCKET_DISCONNECTED,
    /* Connection to SDK controller is in progress. */
    SDKCTL_SOCKET_CONNECTING,
    /* Socket is connected to an SDK controller service. */
    SDKCTL_SOCKET_CONNECTED
} SDKCtlSocketState;

/* Enumerates SDKCtlSocket I/O dispatcher states. */
typedef enum SDKCtlIODispatcherState {
    /* I/O dispatcher expects a packet header. */
    SDKCTL_IODISP_EXPECT_HEADER,
    /* I/O dispatcher expects packet data. */
    SDKCTL_IODISP_EXPECT_DATA,
    /* I/O dispatcher expects query response header. */
    SDKCTL_IODISP_EXPECT_QUERY_REPLY_HEADER,
    /* I/O dispatcher expects query response data. */
    SDKCTL_IODISP_EXPECT_QUERY_REPLY_DATA,
} SDKCtlIODispatcherState;

/* SDKCtlSocket I/O dispatcher descriptor. */
typedef struct SDKCtlIODispatcher {
    /* SDKCtlSocket instance for this dispatcher. */
    SDKCtlSocket*               sdkctl;
    /* Dispatcher state. */
    SDKCtlIODispatcherState     state;
    /* Unites all types of headers used in SDK controller data exchange. */
    union {
        /* Common packet header. */
        SDKCtlPacketHeader      header;
        /* Header for a query packet. */
        SDKCtlQueryHeader       query_header;
        /* Header for a query response packet. */
        SDKCtlQueryReplyHeader  query_reply_header;
    };
    /* Descriptor of a packet packet received from SDK controller. */
    SDKCtlPacket*               packet;
    /* A query for which a reply is currently being received. */
    SDKCtlQuery*                current_query;
} SDKCtlIODispatcher;

/* SDK controller socket descriptor. */
struct SDKCtlSocket {
    /* SDK controller socket state */
    SDKCtlSocketState           state;
    /* I/O dispatcher for the socket. */
    SDKCtlIODispatcher          io_dispatcher;
    /* Asynchronous socket connected to SDK Controller on the device. */
    AsyncSocket*                as;
    /* Client callback that monitors this socket connection. */
    on_sdkctl_connection_cb     on_connection;
    /* A callback to invoke when handshake message is received from the
     * SDK controller. */
    on_sdkctl_handshake_cb      on_handshake;
    /* A callback to invoke when a message is received from the SDK controller. */
    on_sdkctl_message_cb        on_message;
    /* An opaque pointer associated with this socket. */
    void*                       opaque;
    /* Name of an SDK controller service this socket is connected to. */
    char*                       service_name;
    /* I/O looper for timers. */
    Looper*                     looper;
    /* Head of the active query list. */
    SDKCtlQuery*                query_head;
    /* Tail of the active query list. */
    SDKCtlQuery*                query_tail;
    /* Query ID generator that gets incremented for each new query. */
    int                         next_query_id;
    /* Timeout before trying to reconnect after disconnection. */
    int                         reconnect_to;
    /* Number of outstanding references to this descriptor. */
    int                         ref_count;
    /* Head of the recycled memory */
    SDKCtlRecycled*             recycler;
    /* Recyclable block size. */
    uint32_t                    recycler_block_size;
    /* Maximum number of blocks to recycle. */
    int                         recycler_max;
    /* Number of blocs in the recycler. */
    int                         recycler_count;
};

/********************************************************************************
 *                      SDKCtlSocket recycling management
 *******************************************************************************/

/* Gets a recycled block for a given SDKCtlSocket, or allocates new memory
 * block. */
static void*
_sdkctl_socket_alloc_recycler(SDKCtlSocket* sdkctl, uint32_t size)
{
    SDKCtlRecycled* block = NULL;

    if (sdkctl->recycler != NULL && size <= sdkctl->recycler_block_size) {
        /* There are blocks in the recycler, and requested size fits. */
        block = sdkctl->recycler;
        sdkctl->recycler = block->next;
        block->size = sdkctl->recycler_block_size;
        sdkctl->recycler_count--;
    } else if (size <= sdkctl->recycler_block_size) {
        /* There are no blocks in the recycler, but requested size fits. */
        block = malloc(sdkctl->recycler_block_size);
        if (block == NULL) {
            APANIC("SDKCtl %s: Unable to allocate %d bytes block",
                   sdkctl->service_name, sdkctl->recycler_block_size);
        }
        block->size = sdkctl->recycler_block_size;
    } else {
        /* Requested size doesn't fit the recycler. */
        block = malloc(size);
        if (block == NULL) {
            APANIC("SDKCtl %s: Unable to allocate %d bytes block",
                   sdkctl->service_name, size);
        }
        block->size = size;
    }

    return block;
}

/* Recycles, or frees a block of memory for a given SDKCtlSocket. */
static void
_sdkctl_socket_free_recycler(SDKCtlSocket* sdkctl, void* mem)
{
    SDKCtlRecycled* block = (SDKCtlRecycled*)mem;

    if (sdkctl->recycler_count == sdkctl->recycler_max ||
        block->size != sdkctl->recycler_block_size) {
        /* Recycler is full, or block cannot be recycled. */
        free(mem);
        return;
    }

    block->next = sdkctl->recycler;
    sdkctl->recycler = block;
    sdkctl->recycler_count++;
}

/* Empties the recycler for a given SDKCtlSocket. */
static void
_sdkctl_socket_empty_recycler(SDKCtlSocket* sdkctl)
{
    SDKCtlRecycled* block = sdkctl->recycler;
    while (block != NULL) {
        void* to_free = block;
        block = block->next;
        free(to_free);
    }
    sdkctl->recycler = NULL;
    sdkctl->recycler_count = 0;
}

/********************************************************************************
 *                      SDKCtlSocket query list management
 *******************************************************************************/

/* Adds a query to the list of active queries.
 * Param:
 *  sdkctl - SDKCtlSocket instance for the query.
 *  query - Query to add to the list.
 */
static void
_sdkctl_socket_add_query(SDKCtlQuery* query)
{
    SDKCtlSocket* const sdkctl = query->sdkctl;
    if (sdkctl->query_head == NULL) {
        sdkctl->query_head = sdkctl->query_tail = query;
    } else {
        sdkctl->query_tail->next = query;
        sdkctl->query_tail = query;
    }

    /* Keep the query referenced while it's in the list. */
    sdkctl_query_reference(query);
}

/* Removes a query from the list of active queries.
 * Param:
 *  query - Query to remove from the list of active queries.
 * Return:
 *  Boolean: 1 if query has been removed, or 0 if query has not been found in the
 *  list of active queries.
 */
static int
_sdkctl_socket_remove_query(SDKCtlQuery* query)
{
    SDKCtlSocket* const sdkctl = query->sdkctl;
    SDKCtlQuery* prev = NULL;
    SDKCtlQuery* head = sdkctl->query_head;

    /* Quick check: the query could be currently handled by dispatcher. */
    if (sdkctl->io_dispatcher.current_query == query) {
        /* Release the query from dispatcher. */
        sdkctl_query_release(query);
        sdkctl->io_dispatcher.current_query = NULL;
        return 1;
    }

    /* Remove query from the list. */
    while (head != NULL && query != head) {
        prev = head;
        head = head->next;
    }
    if (head != NULL) {
        if (prev == NULL) {
            /* Query is at the head of the list. */
            assert(query == sdkctl->query_head);
            sdkctl->query_head = query->next;
        } else {
            /* Query is in the middle / at the end of the list. */
            assert(query != sdkctl->query_head);
            prev->next = query->next;
        }
        if (sdkctl->query_tail == query) {
            /* Query is at the tail of the list. */
            assert(query->next == NULL);
            sdkctl->query_tail = prev;
        }
        query->next = NULL;

        /* Release query that is now removed from the list. Note that query
         * passed to this routine should hold an extra reference, owned by the
         * caller. */
        sdkctl_query_release(query);
        return 1;
    } else {
        D("%s: Query %p is not found in the list.", sdkctl->service_name, query);
        return 0;
    }
}

/* Removes a query (based on query ID) from the list of active queries.
 * Param:
 *  sdkctl - SDKCtlSocket instance that owns the query.
 *  query_id - Identifies the query to remove.
 * Return:
 *  A query removed from the list of active queries, or NULL if query with the
 *  given ID has not been found in the list.
 */
static SDKCtlQuery*
_sdkctl_socket_remove_query_id(SDKCtlSocket* sdkctl, int query_id)
{
    SDKCtlQuery* prev = NULL;
    SDKCtlQuery* head = sdkctl->query_head;

    /* Quick check: the query could be currently handled by dispatcher. */
    if (sdkctl->io_dispatcher.current_query != NULL &&
        sdkctl->io_dispatcher.current_query->header.query_id == query_id) {
        /* Release the query from dispatcher. */
        SDKCtlQuery* const query = sdkctl->io_dispatcher.current_query;
        sdkctl->io_dispatcher.current_query = NULL;
        return query;
    }

    /* Remove query from the list. */
    while (head != NULL && head->header.query_id != query_id) {
        prev = head;
        head = head->next;
    }
    if (head != NULL) {
        /* Query is found in the list. */
        SDKCtlQuery* const query = head;
        if (prev == NULL) {
            /* Query is at the head of the list. */
            assert(query == sdkctl->query_head);
            sdkctl->query_head = query->next;
        } else {
            /* Query is in the middle, or at the end of the list. */
            assert(query != sdkctl->query_head);
            prev->next = query->next;
        }
        if (sdkctl->query_tail == query) {
            /* Query is at the tail of the list. */
            assert(query->next == NULL);
            sdkctl->query_tail = prev;
        }
        query->next = NULL;
        return query;
    } else {
        D("%s: Query ID %d is not found in the list.",
          sdkctl->service_name, query_id);
        return NULL;
    }
}

/* Pulls the first query from the list of active queries.
 * Param:
 *  sdkctl - SDKCtlSocket instance that owns the query.
 * Return:
 *  A query removed pulled from the list of active queries, or NULL if query
 *  list is empty.
 */
static SDKCtlQuery*
_sdkctl_socket_pull_first_query(SDKCtlSocket* sdkctl)
{
    SDKCtlQuery* const query = sdkctl->query_head;

    if (query != NULL) {
        sdkctl->query_head = query->next;
        if (sdkctl->query_head == NULL) {
            sdkctl->query_tail = NULL;
        }
    }
    return query;
}

/* Generates new query ID for the given SDKCtl. */
static int
_sdkctl_socket_next_query_id(SDKCtlSocket* sdkctl)
{
    return ++sdkctl->next_query_id;
}

/********************************************************************************
 *                      SDKCtlPacket implementation
 *******************************************************************************/

/* Alocates a packet. */
static SDKCtlPacket*
_sdkctl_packet_new(SDKCtlSocket* sdkctl, int size, int type)
{
    const uint32_t total_size = sizeof(SDKCtlPacket) + size;
    SDKCtlPacket* const packet = _sdkctl_socket_alloc_recycler(sdkctl, total_size);

    packet->sdkctl      = sdkctl;
    packet->ref_count   = 1;
    packet->header.size = size;
    packet->header.type = type;

    /* Refence SDKCTlSocket that owns this packet. */
    sdkctl_socket_reference(sdkctl);

    return packet;
}

/* Frees a packet. */
static void
_sdkctl_packet_free(SDKCtlPacket* packet)
{
    SDKCtlSocket* const sdkctl = packet->sdkctl;

    /* Free allocated resources. */
    _sdkctl_socket_free_recycler(packet->sdkctl, packet);

    /* Release SDKCTlSocket that owned this packet. */
    sdkctl_socket_release(sdkctl);
}

int
sdkctl_packet_reference(SDKCtlPacket* packet)
{
    assert(packet->ref_count > 0);
    packet->ref_count++;
    return packet->ref_count;
}

int
sdkctl_packet_release(SDKCtlPacket* packet)
{
    assert(packet->ref_count > 0);
    packet->ref_count--;
    if (packet->ref_count == 0) {
        /* Last reference has been dropped. Destroy this object. */
        _sdkctl_packet_free(packet);
        return 0;
    }
    return packet->ref_count;
}

/********************************************************************************
 *                    SDKCtlQuery implementation
 *******************************************************************************/

/* Frees query descriptor. */
static void
_sdkctl_query_free(SDKCtlQuery* query)
{
    if (query != NULL) {
        SDKCtlSocket* const sdkctl = query->sdkctl;
        T("SDKCtl %s: Query %p ID %d is freed.",
          sdkctl->service_name, query, query->header.query_id);

        /* Free allocated resources. */
        if (query->internal_resp_buffer != NULL &&
            (query->response_buffer == NULL ||
             query->response_buffer == &query->internal_resp_buffer)) {
            free(query->internal_resp_buffer);
        }

        loopTimer_done(query->timer);
        _sdkctl_socket_free_recycler(sdkctl, query);

        /* Release socket that owned this query. */
        sdkctl_socket_release(sdkctl);
    }
}

/* Cancels timeout for the query.
 *
 * For the simplicity of implementation, the dispatcher will cancel query timer
 * when query response data begins to flow in. If we let the timer to expire at
 * that stage, we will end up with data flowing in without real place to
 * accomodate it.
 */
static void
_sdkctl_query_cancel_timeout(SDKCtlQuery* query)
{
    loopTimer_stop(query->timer);

    T("SDKCtl %s: Query %p ID %d deadline is cancelled.",
      query->sdkctl->service_name, query, query->header.query_id);
}

/*
 * Query I/O callbacks.
 */

/* Callback that is invoked by the I/O dispatcher when query is successfuly
 * completed (i.e. response to the query is received).
 */
static void
_on_sdkctl_query_completed(SDKCtlQuery* query)
{
    T("SDKCtl %s: Query %p ID %d is completed.",
      query->sdkctl->service_name, query, query->header.query_id);

    /* Cancel deadline, and inform the client about query completion. */
    _sdkctl_query_cancel_timeout(query);
    query->query_cb(query->query_opaque, query, ASIO_STATE_SUCCEEDED);
}

/* A callback that is invoked on query cancellation. */
static void
_on_sdkctl_query_cancelled(SDKCtlQuery* query)
{
    /*
     * Query cancellation means that SDK controller is disconnected. In turn,
     * this means that SDK controller socket will handle disconnection in its
     * connection callback. So, at this point all we need to do here is to inform
     * the client, and then unlist the query.
     */

    /* Cancel deadline, and inform the client about query cancellation. */
    _sdkctl_query_cancel_timeout(query);
    query->query_cb(query->query_opaque, query, ASIO_STATE_CANCELLED);
}

/* A timer callback that is invoked on query timeout.
 * Param:
 *  opaque - SDKCtlQuery instance.
 */
static void
_on_skdctl_query_timeout(void* opaque)
{
    SDKCtlQuery* const query = (SDKCtlQuery*)opaque;

    D("SDKCtl %s: Query %p ID %d with deadline %lld has timed out at %lld",
      query->sdkctl->service_name, query, query->header.query_id,
      query->deadline, async_socket_deadline(query->sdkctl->as, 0));

    /* Reference the query while we're in this callback. */
    sdkctl_query_reference(query);

    /* Inform the client about deadline expiration. Note that client may
     * extend the deadline, and retry the query. */
    const AsyncIOAction action =
        query->query_cb(query->query_opaque, query, ASIO_STATE_TIMED_OUT);

    /* For actions other than retry we will destroy the query. */
    if (action != ASIO_ACTION_RETRY) {
        _sdkctl_socket_remove_query(query);
    }

    sdkctl_query_release(query);
}

/* A callback that is invoked when query has been sent to the SDK controller
 * service. */
static void
_on_sdkctl_query_sent(SDKCtlQuery* query)
{
    T("SDKCtl %s: sent %d bytes of query %p ID %d of type %d",
      query->sdkctl->service_name, query->header.packet.size, query,
      query->header.query_id, query->header.query_type);

    /* Inform the client about the event. */
    query->query_cb(query->query_opaque, query, ASIO_STATE_CONTINUES);

    /* Set a timer to expire at query's deadline, and let the response to come
     * through the dispatcher loop. */
    loopTimer_startAbsolute(query->timer, query->deadline);
}

/* An I/O callback invoked on query transmission.
 * Param:
 *  io_opaque SDKCtlQuery instance of the query that's being sent with this I/O.
 *  asio - Write I/O descriptor.
 *  status - I/O status.
 */
static AsyncIOAction
_on_sdkctl_query_send_io(void* io_opaque,
                         AsyncSocketIO* asio,
                         AsyncIOState status)
{
    SDKCtlQuery* const query = (SDKCtlQuery*)io_opaque;
    AsyncIOAction action = ASIO_ACTION_DONE;

    /* Reference the query while we're in this callback. */
    sdkctl_query_reference(query);

    if (status == ASIO_STATE_SUCCEEDED) {
        /* Query has been sent to the service. */
        _on_sdkctl_query_sent(query);

        sdkctl_query_release(query);

        return ASIO_ACTION_DONE;
    }

    /* Lets see what's going on with query transmission. */
    switch (status) {
        case ASIO_STATE_CANCELLED:
            T("SDKCtl %s: Query %p ID %d is cancelled in %s I/O.",
              query->sdkctl->service_name, query, query->header.query_id,
              async_socket_io_is_read(asio) ? "READ" : "WRITE");
            /* Remove the query from the list of active queries. */
            _sdkctl_socket_remove_query(query);
            _on_sdkctl_query_cancelled(query);
            break;

        case ASIO_STATE_TIMED_OUT:
            D("SDKCtl %s: Query %p ID %d with deadline %lld has timed out in %s I/O at %lld",
              query->sdkctl->service_name, query, query->header.query_id,
              query->deadline, async_socket_io_is_read(asio) ? "READ" : "WRITE",
              async_socket_deadline(query->sdkctl->as, 0));
            /* Invoke query's callback. */
            action = query->query_cb(query->query_opaque, query, status);
            /* For actions other than retry we need to stop the query. */
            if (action != ASIO_ACTION_RETRY) {
                _sdkctl_socket_remove_query(query);
            }
            break;

        case ASIO_STATE_FAILED:
            T("SDKCtl %s: Query %p ID %d failed in %s I/O: %d -> %s",
              query->sdkctl->service_name, query, query->header.query_id,
              async_socket_io_is_read(asio) ? "READ" : "WRITE",
              errno, strerror(errno));
            /* Invoke query's callback. Note that we will let the client to
             * decide what to do on I/O failure. */
            action = query->query_cb(query->query_opaque, query, status);
            /* For actions other than retry we need to stop the query. */
            if (action != ASIO_ACTION_RETRY) {
                _sdkctl_socket_remove_query(query);
            }
            break;

        case ASIO_STATE_FINISHED:
            /* Time to disassociate with the I/O. */
            sdkctl_query_release(query);
            break;

        default:
            /* Transitional state. */
            break;
    }

    sdkctl_query_release(query);

    return action;
}

/********************************************************************************
 *                    SDKCtlQuery public API implementation
 ********************************************************************************/

SDKCtlQuery*
sdkctl_query_new(SDKCtlSocket* sdkctl, int query_type, uint32_t in_data_size)
{
    const uint32_t total_size = sizeof(SDKCtlQuery) + in_data_size;

    SDKCtlQuery* const query = _sdkctl_socket_alloc_recycler(sdkctl, total_size);
    query->next                 = NULL;
    query->sdkctl               = sdkctl;
    query->response_buffer      = NULL;
    query->response_size        = NULL;
    query->internal_resp_buffer = NULL;
    query->internal_resp_size   = 0;
    query->query_cb             = NULL;
    query->query_opaque         = NULL;
    query->deadline             = DURATION_INFINITE;
    query->ref_count            = 1;
    query->header.packet.size   = sizeof(SDKCtlQueryHeader) + in_data_size;
    query->header.packet.type   = SDKCTL_PACKET_QUERY;
    query->header.query_id      = _sdkctl_socket_next_query_id(sdkctl);
    query->header.query_type    = query_type;

    /* Initialize timer to fire up on query deadline expiration. */
    loopTimer_init(query->timer, sdkctl->looper, _on_skdctl_query_timeout, query);

    /* Reference socket that owns this query. */
    sdkctl_socket_reference(sdkctl);

    T("SDKCtl %s: Query %p ID %d type %d is created for %d bytes of data.",
      query->sdkctl->service_name, query, query->header.query_id,
      query_type, in_data_size);

    return query;
}

SDKCtlQuery*
sdkctl_query_new_ex(SDKCtlSocket* sdkctl,
                    int query_type,
                    uint32_t in_data_size,
                    const void* in_data,
                    void** response_buffer,
                    uint32_t* response_size,
                    on_sdkctl_query_cb query_cb,
                    void* query_opaque)
{
    SDKCtlQuery* const query = sdkctl_query_new(sdkctl, query_type, in_data_size);

    query->response_buffer  = response_buffer;
    if (query->response_buffer == NULL) {
        /* Creator didn't supply a buffer. Use internal one instead. */
        query->response_buffer = &query->internal_resp_buffer;
        query->internal_resp_buffer = NULL;
    }
    query->response_size    = response_size;
    if (query->response_size == NULL) {
        /* Creator didn't supply a buffer for response size. Use internal one
         * instead. */
        query->response_size = &query->internal_resp_size;
        query->internal_resp_size = 0;
    }
    query->query_cb         = query_cb;
    query->query_opaque     = query_opaque;
    /* Init query's input buffer. */
    if (in_data_size != 0 && in_data != NULL) {
        memcpy(query + 1, in_data, in_data_size);
    }

    return query;
}

void
sdkctl_query_send(SDKCtlQuery* query, int to)
{
    SDKCtlSocket* const sdkctl = query->sdkctl;

    /* Initialize the deadline. */
    query->deadline = async_socket_deadline(query->sdkctl->as, to);

    /* List the query in the list of active queries. */
    _sdkctl_socket_add_query(query);

    /* Reference query associated with write I/O. */
    sdkctl_query_reference(query);

    /* Transmit the query to SDK controller. */
    async_socket_write_abs(sdkctl->as, &query->header, query->header.packet.size,
                           _on_sdkctl_query_send_io, query, query->deadline);

    T("SDKCtl %s: Query %p ID %d type %d is sent with deadline at %lld",
      query->sdkctl->service_name, query, query->header.query_id,
      query->header.query_type, query->deadline);
}

SDKCtlQuery*
sdkctl_query_build_and_send(SDKCtlSocket* sdkctl,
                            int query_type,
                            uint32_t in_data_size,
                            const void* in_data,
                            void** response_buffer,
                            uint32_t* response_size,
                            on_sdkctl_query_cb query_cb,
                            void* query_opaque,
                            int to)
{
    SDKCtlQuery* const query =
        sdkctl_query_new_ex(sdkctl, query_type, in_data_size, in_data,
                            response_buffer, response_size, query_cb,
                            query_opaque);
    sdkctl_query_send(query, to);
    return query;
}

int
sdkctl_query_reference(SDKCtlQuery* query)
{
    assert(query->ref_count > 0);
    query->ref_count++;
    return query->ref_count;
}

int
sdkctl_query_release(SDKCtlQuery* query)
{
    assert(query->ref_count > 0);
    query->ref_count--;
    if (query->ref_count == 0) {
        /* Last reference has been dropped. Destroy this object. */
        _sdkctl_query_free(query);
        return 0;
    }
    return query->ref_count;
}

/********************************************************************************
 *                      SDKCtlPacket implementation
 *******************************************************************************/

/* A packet has been received from SDK controller. */
static void
_on_sdkctl_packet_received(SDKCtlSocket* sdkctl, SDKCtlPacket* packet)
{
    T("SDKCtl %s: Received packet size: %d, type: %d",
      sdkctl->service_name, packet->header.size, packet->header.type);

    /* Dispatch received packet to the client. */
    sdkctl->on_message(sdkctl->opaque, sdkctl, packet, packet->header.type,
                       packet + 1, packet->header.size - sizeof(SDKCtlPacketHeader));
}

/********************************************************************************
 *                      SDKCtlIODispatcher implementation
 *******************************************************************************/

/* An I/O callback invoked when data gets received from the socket.
 * Param:
 *  io_opaque SDKCtlIODispatcher instance associated with the reader.
 *  asio - Read I/O descriptor.
 *  status - I/O status.
 */
static AsyncIOAction _on_sdkctl_io_dispatcher_io(void* io_opaque,
                                                 AsyncSocketIO* asio,
                                                 AsyncIOState status);

/* Starts I/O dispatcher for SDK controller socket. */
static void
_sdkctl_io_dispatcher_start(SDKCtlSocket* sdkctl) {
    SDKCtlIODispatcher* const dispatcher = &sdkctl->io_dispatcher;

    dispatcher->state           = SDKCTL_IODISP_EXPECT_HEADER;
    dispatcher->sdkctl          = sdkctl;
    dispatcher->packet          = NULL;
    dispatcher->current_query   = NULL;

    /* Register a packet header reader with the socket. */
    async_socket_read_rel(dispatcher->sdkctl->as, &dispatcher->header,
                          sizeof(SDKCtlPacketHeader), _on_sdkctl_io_dispatcher_io,
                          dispatcher, -1);
}

/* Resets I/O dispatcher for SDK controller socket. */
static void
_sdkctl_io_dispatcher_reset(SDKCtlSocket* sdkctl) {
    SDKCtlIODispatcher* const dispatcher = &sdkctl->io_dispatcher;

    /* Cancel current query. */
    if (dispatcher->current_query != NULL) {
        SDKCtlQuery* const query = dispatcher->current_query;
        dispatcher->current_query = NULL;
        _on_sdkctl_query_cancelled(query);
        sdkctl_query_release(query);
    }

    /* Free packet data buffer. */
    if (dispatcher->packet != NULL) {
        sdkctl_packet_release(dispatcher->packet);
        dispatcher->packet = NULL;
    }

    /* Reset dispatcher state. */
    dispatcher->state = SDKCTL_IODISP_EXPECT_HEADER;
}

/*
 * I/O dispatcher callbacks.
 */

/* A callback that is invoked when a failure occurred while dispatcher was
 * reading data from the socket.
 */
static void
_on_io_dispatcher_io_failure(SDKCtlIODispatcher* dispatcher,
                             AsyncSocketIO* asio)
{
    SDKCtlSocket* const sdkctl = dispatcher->sdkctl;

    D("SDKCtl %s: Dispatcher I/O failure: %d -> %s",
      sdkctl->service_name, errno, strerror(errno));

    /* We treat all I/O failures same way we treat disconnection. Just cancel
     * everything, disconnect, and let the client to decide what to do next. */
    sdkctl_socket_disconnect(sdkctl);

    /* Report disconnection to the client, and let it restore connection in this
     * callback. */
    sdkctl->on_connection(sdkctl->opaque, sdkctl, ASIO_STATE_FAILED);
}

/* A callback that is invoked when dispatcher's reader has been cancelled. */
static void
_on_io_dispatcher_io_cancelled(SDKCtlIODispatcher* dispatcher,
                               AsyncSocketIO* asio)
{
    T("SDKCtl %s: Dispatcher I/O cancelled.", dispatcher->sdkctl->service_name);

    /* If we're in the middle of receiving query reply we need to cancel the
     * query. */
    if (dispatcher->current_query != NULL) {
        SDKCtlQuery* const query = dispatcher->current_query;
        dispatcher->current_query = NULL;
        _on_sdkctl_query_cancelled(query);
        sdkctl_query_release(query);
    }

    /* Discard packet data we've received so far. */
    if (dispatcher->packet != NULL) {
        sdkctl_packet_release(dispatcher->packet);
        dispatcher->packet = NULL;
    }
}

/* A generic packet header has been received by I/O dispatcher. */
static AsyncIOAction
_on_io_dispatcher_packet_header(SDKCtlIODispatcher* dispatcher,
                                AsyncSocketIO* asio)
{
    SDKCtlSocket* const sdkctl = dispatcher->sdkctl;

    T("SDKCtl %s: Packet header type %d, size %d is received.",
      dispatcher->sdkctl->service_name, dispatcher->header.type,
      dispatcher->header.size);

    /* Here we have three choices for the packet, that define the rest of
     * the data that follow it:
     * - Regular packet,
     * - Response to a query that has been sent to SDK controller,
     * - A query from SDK controller.
     * Update the state accordingly, and initiate reading of the
     * remaining of the packet.
     */
     if (dispatcher->header.type == SDKCTL_PACKET_QUERY_RESPONSE) {
        /* This is a response to the query. Before receiving response data we
         * need to locate the relevant query, and use its response buffer to read
         * the data. For that we need to obtain query ID firts. So, initiate
         * reading of the remaining part of SDKCtlQueryReplyHeader. */
        dispatcher->state = SDKCTL_IODISP_EXPECT_QUERY_REPLY_HEADER;
        async_socket_read_rel(sdkctl->as, &dispatcher->query_reply_header.query_id,
                              sizeof(SDKCtlQueryReplyHeader) - sizeof(SDKCtlPacketHeader),
                             _on_sdkctl_io_dispatcher_io, dispatcher, -1);
    } else {
        /* For regular packets, as well as queries, we simply allocate buffer,
         * that fits the entire packet, and read the remainder of the data in
         * there. */
        dispatcher->state = SDKCTL_IODISP_EXPECT_DATA;
        dispatcher->packet =
            _sdkctl_packet_new(sdkctl, dispatcher->header.size,
                               dispatcher->header.type);
        /* Initiate reading of the packet data. */
        async_socket_read_rel(sdkctl->as, dispatcher->packet + 1,
                              dispatcher->header.size - sizeof(SDKCtlPacketHeader),
                              _on_sdkctl_io_dispatcher_io, dispatcher, -1);
    }

    return ASIO_ACTION_DONE;
}

/* A generic packet has been received by I/O dispatcher. */
static AsyncIOAction
_on_io_dispatcher_packet(SDKCtlIODispatcher* dispatcher, AsyncSocketIO* asio)
{
    SDKCtlSocket* const sdkctl = dispatcher->sdkctl;

    T("SDKCtl %s: Packet type %d, size %d is received.",
      dispatcher->sdkctl->service_name, dispatcher->header.type,
      dispatcher->header.size);

    _on_sdkctl_packet_received(sdkctl, dispatcher->packet);
    sdkctl_packet_release(dispatcher->packet);
    dispatcher->packet = NULL;

    /* Get ready for the next I/O cycle. */
    dispatcher->state = SDKCTL_IODISP_EXPECT_HEADER;
    async_socket_read_rel(sdkctl->as, &dispatcher->header, sizeof(SDKCtlPacketHeader),
                          _on_sdkctl_io_dispatcher_io, dispatcher, -1);
    return ASIO_ACTION_DONE;
}

/* A query reply header has been received by I/O dispatcher. */
static AsyncIOAction
_on_io_dispatcher_query_reply_header(SDKCtlIODispatcher* dispatcher,
                                     AsyncSocketIO* asio)
{
    SDKCtlSocket* const sdkctl = dispatcher->sdkctl;
    SDKCtlQuery* query;

    T("SDKCtl %s: Query reply header is received for query ID %d",
      dispatcher->sdkctl->service_name, dispatcher->query_reply_header.query_id);

    /* Pull the query out of the list of active queries. It's the dispatcher that
     * owns this query now. */
    dispatcher->current_query =
        _sdkctl_socket_remove_query_id(sdkctl, dispatcher->query_reply_header.query_id);
    query = dispatcher->current_query;

    if (query == NULL) {
        D("%s: Query #%d is not found by dispatcher",
          dispatcher->sdkctl->service_name, dispatcher->query_reply_header.query_id);

        /* Query is not found. Just read the remainder of reply up in the air,
         * and then discard when it's over. */
        dispatcher->state = SDKCTL_IODISP_EXPECT_QUERY_REPLY_DATA;
        dispatcher->packet =
            _sdkctl_packet_new(sdkctl, dispatcher->header.size,
                               dispatcher->header.type);
        /* Copy query reply info to the packet. */
        memcpy(&dispatcher->packet->header, &dispatcher->query_reply_header,
               sizeof(SDKCtlQueryReplyHeader));
        async_socket_read_rel(sdkctl->as, &dispatcher->query_header + 1,
                              dispatcher->header.size - sizeof(SDKCtlQueryReplyHeader),
                             _on_sdkctl_io_dispatcher_io, dispatcher, -1);
    } else {
        /* Prepare to receive query reply. For the simplicity sake, cancel query
         * time out, so it doesn't expire on us while we're in the middle of
         * receiving query's reply. */
        _sdkctl_query_cancel_timeout(query);

        /* Adjust the reply buffer set for the query (if needed). */
        const uint32_t query_data_size =
            dispatcher->header.size - sizeof(SDKCtlQueryReplyHeader);
        if (*query->response_size < query_data_size) {
            *query->response_buffer = malloc(query_data_size);
            if (*query->response_buffer == NULL) {
                APANIC("%s: Unable to allocate %d bytes for query response",
                       sdkctl->service_name, query_data_size);
            }
        }
        /* Save the actual query response size. */
        *query->response_size = query_data_size;

        /* Start reading query response. */
        dispatcher->state = SDKCTL_IODISP_EXPECT_QUERY_REPLY_DATA;
        async_socket_read_rel(sdkctl->as, *query->response_buffer,
                              *query->response_size, _on_sdkctl_io_dispatcher_io,
                              dispatcher, -1);
    }

    return ASIO_ACTION_DONE;
}

/* A query reply header has been received by I/O dispatcher. */
static AsyncIOAction
_on_io_dispatcher_query_reply(SDKCtlIODispatcher* dispatcher, AsyncSocketIO* asio)
{
    SDKCtlSocket* const sdkctl = dispatcher->sdkctl;
    SDKCtlQuery* const query = dispatcher->current_query;
    dispatcher->current_query = NULL;

    if (query != NULL) {
        _ANDROID_ASSERT(query->header.query_id == dispatcher->query_reply_header.query_id,
                        "SDKCtl %s: Query ID mismatch in I/O dispatcher",
                        sdkctl->service_name);
        T("SDKCtl %s: Query reply is received for query %p ID %d. Reply size is %d",
          dispatcher->sdkctl->service_name, query, query->header.query_id,
          *query->response_size);

        /* Complete the query, and release it from the dispatcher. */
        _on_sdkctl_query_completed(query);
        sdkctl_query_release(query);
    } else {
        /* This was "read up in the air" for a cancelled query. Just discard the
         * read data. */
        if (dispatcher->packet != NULL) {
            sdkctl_packet_release(dispatcher->packet);
            dispatcher->packet = NULL;
        }
    }

    /* Get ready for the next I/O cycle. */
    dispatcher->state = SDKCTL_IODISP_EXPECT_HEADER;
    async_socket_read_rel(sdkctl->as, &dispatcher->header, sizeof(SDKCtlPacketHeader),
                          _on_sdkctl_io_dispatcher_io, dispatcher, -1);
    return ASIO_ACTION_DONE;
}

/* An I/O callback invoked when data gets received from the socket.
 * This is main I/O dispatcher loop.
 * Param:
 *  io_opaque SDKCtlIODispatcher instance associated with the reader.
 *  asio - Read I/O descriptor.
 *  status - I/O status.
 */
static AsyncIOAction
_on_sdkctl_io_dispatcher_io(void* io_opaque,
                            AsyncSocketIO* asio,
                            AsyncIOState status)
{
    AsyncIOAction action = ASIO_ACTION_DONE;
    SDKCtlIODispatcher* const dispatcher = (SDKCtlIODispatcher*)io_opaque;
    SDKCtlSocket* const sdkctl = dispatcher->sdkctl;

    /* Reference SDKCtlSocket while we're in this callback. */
    sdkctl_socket_reference(sdkctl);

    if (status != ASIO_STATE_SUCCEEDED) {
        /* Something going on with I/O other than receiving data.. */
        switch (status) {
            case ASIO_STATE_STARTED:
                /* Data has started flowing in. Cancel timeout on I/O that has
                 * started, so we can complete the current state of the
                 * dispatcher without interruptions other than I/O failures. */
                async_socket_io_cancel_time_out(asio);
                break;

            case ASIO_STATE_FAILED:
                /* I/O failure has occurred. Handle the failure. */
                _on_io_dispatcher_io_failure(dispatcher, asio);
                break;

            case ASIO_STATE_TIMED_OUT:
                 /* The way I/O dispatcher is implemented, this should never
                  * happen, because dispatcher doesn't set I/O expiration time
                  * when registering its readers. */
                _ANDROID_ASSERT(0,
                    "SDKCtl %s: We should never receive ASIO_STATE_TIMED_OUT in SDKCtl I/O dispatcher.",
                    sdkctl->service_name);
                break;

            case ASIO_STATE_CANCELLED:
                /* Cancellation means that we're in the middle of handling
                 * disconnection. Sooner or later, this dispatcher will be reset,
                 * so we don't really care about keeping its state at this point.
                 */
                _on_io_dispatcher_io_cancelled(dispatcher, asio);
                break;

            case ASIO_STATE_FINISHED:
                break;

            default:
                _ANDROID_ASSERT(0, "SDKCtl %s: Unexpected I/O status %d in the dispatcher",
                                sdkctl->service_name, status);
                /* Handle this as protocol failure. */
                errno = EINVAL;
                _on_io_dispatcher_io_failure(dispatcher, asio);
                action = ASIO_ACTION_ABORT;
                break;
        }

        sdkctl_socket_release(sdkctl);

        return action;
    }

    /* Requested data has been read. Handle the chunk depending on dispatcher's
     * state. */
    switch (dispatcher->state) {
        case SDKCTL_IODISP_EXPECT_HEADER:
            /* A generic packet header is received. */
            action = _on_io_dispatcher_packet_header(dispatcher, asio);
            break;

        case SDKCTL_IODISP_EXPECT_QUERY_REPLY_HEADER:
            /* Query reply header is received. */
            action = _on_io_dispatcher_query_reply_header(dispatcher, asio);
            break;

        case SDKCTL_IODISP_EXPECT_QUERY_REPLY_DATA:
            /* Query reply is received. Complete the query. */
            action = _on_io_dispatcher_query_reply(dispatcher, asio);
            break;

        case SDKCTL_IODISP_EXPECT_DATA:
            /* A generic packet is received. */
            action = _on_io_dispatcher_packet(dispatcher, asio);
            break;

        default:
            _ANDROID_ASSERT(0, "SDKCtl %s: Unexpected I/O dispacher state %d",
                            sdkctl->service_name, dispatcher->state);
            break;
    }

    sdkctl_socket_release(sdkctl);

    return action;
}

/********************************************************************************
 *                       SDKCtlSocket internals.
 *******************************************************************************/

/* Cancels all queries that is active on this socket. */
static void
_sdkctl_socket_cancel_all_queries(SDKCtlSocket* sdkctl)
{
    SDKCtlIODispatcher* const dispatcher = &sdkctl->io_dispatcher;
    SDKCtlQuery* query;

    /* Cancel query that is being completed in dispatcher. */
    if (dispatcher->current_query != NULL) {
        SDKCtlQuery* const query = dispatcher->current_query;
        dispatcher->current_query = NULL;
        _on_sdkctl_query_cancelled(query);
        sdkctl_query_release(query);
    }

    /* One by one empty query list cancelling pulled queries. */
    query = _sdkctl_socket_pull_first_query(sdkctl);
    while (query != NULL) {
        _sdkctl_query_cancel_timeout(query);
        query->query_cb(query->query_opaque, query, ASIO_STATE_CANCELLED);
        sdkctl_query_release(query);
        query = _sdkctl_socket_pull_first_query(sdkctl);
    }
}

/* Cancels all packets that is active on this socket. */
static void
_sdkctl_socket_cancel_all_packets(SDKCtlSocket* sdkctl)
{
}

/* Cancels all I/O that is active on this socket. */
static void
_sdkctl_socket_cancel_all_io(SDKCtlSocket* sdkctl)
{
    /* Cancel all queries, and packets that are active for this I/O. */
    _sdkctl_socket_cancel_all_queries(sdkctl);
    _sdkctl_socket_cancel_all_packets(sdkctl);
}

/* Disconnects AsyncSocket for SDKCtlSocket. */
static void
_sdkctl_socket_disconnect_socket(SDKCtlSocket* sdkctl)
{
    if (sdkctl->as != NULL) {
        /* Disconnect the socket. This will trigger I/O cancellation callbacks. */
        async_socket_disconnect(sdkctl->as);

        /* Cancel all I/O that is active on this socket. */
        _sdkctl_socket_cancel_all_io(sdkctl);

        /* Reset I/O dispatcher. */
        _sdkctl_io_dispatcher_reset(sdkctl);
    }

    sdkctl->state = SDKCTL_SOCKET_DISCONNECTED;
}

/* Frees SDKCtlSocket instance. */
static void
_sdkctl_socket_free(SDKCtlSocket* sdkctl)
{
    if (sdkctl != NULL) {
        /* Disconnect, and release the socket. */
        if (sdkctl->as != NULL) {
            async_socket_disconnect(sdkctl->as);
            async_socket_release(sdkctl->as);
        }

        /* Free allocated resources. */
        if (sdkctl->looper != NULL) {
            looper_free(sdkctl->looper);
        }
        if (sdkctl->service_name != NULL) {
            free(sdkctl->service_name);
        }
        _sdkctl_socket_empty_recycler(sdkctl);

        AFREE(sdkctl);
    }
}

/********************************************************************************
 *                    SDK Control Socket connection callbacks.
 *******************************************************************************/

/* Initiates handshake query when SDK controller socket is connected. */
static void _sdkctl_do_handshake(SDKCtlSocket* sdkctl);

/* A socket connection is established.
 * Here we will start I/O dispatcher, and will initiate a handshake with
 * the SdkController service for this socket. */
static AsyncIOAction
_on_async_socket_connected(SDKCtlSocket* sdkctl)
{
    D("SDKCtl %s: Socket is connected.", sdkctl->service_name);

    /* Notify the client that connection is established. */
    const AsyncIOAction action =
        sdkctl->on_connection(sdkctl->opaque, sdkctl, ASIO_STATE_SUCCEEDED);

    if (action == ASIO_ACTION_DONE) {
        /* Initialize, and start main I/O dispatcher. */
        _sdkctl_io_dispatcher_start(sdkctl);

        /* Initiate handshake. */
        _sdkctl_do_handshake(sdkctl);

        return action;
    } else {
        /* Client didn't like something about this connection. */
        return action;
    }
}

/* Handles lost connection with SdkController service. */
static AsyncIOAction
_on_async_socket_disconnected(SDKCtlSocket* sdkctl)
{
    D("SDKCtl %s: Socket has been disconnected.", sdkctl->service_name);

    _sdkctl_socket_disconnect_socket(sdkctl);

    AsyncIOAction action = sdkctl->on_connection(sdkctl->opaque, sdkctl,
                                                 ASIO_STATE_FAILED);
    if (action == ASIO_ACTION_DONE) {
        /* Default action for disconnect is to reestablish the connection. */
        action = ASIO_ACTION_RETRY;
    }
    if (action == ASIO_ACTION_RETRY) {
        sdkctl->state = SDKCTL_SOCKET_CONNECTING;
    }
    return action;
}

/* An entry point for all socket connection events.
 * Here we will dispatch connection events to appropriate handlers.
 * Param:
 *  client_opaque - SDKCtlSocket isntance.
 */
static AsyncIOAction
_on_async_socket_connection(void* client_opaque,
                            AsyncSocket* as,
                            AsyncIOState status)
{
    AsyncIOAction action = ASIO_ACTION_DONE;
    SDKCtlSocket* const sdkctl = (SDKCtlSocket*)client_opaque;

    /* Reference the socket while in this callback. */
    sdkctl_socket_reference(sdkctl);

    switch (status) {
        case ASIO_STATE_SUCCEEDED:
            sdkctl->state = SDKCTL_SOCKET_CONNECTED;
            _on_async_socket_connected(sdkctl);
            break;

        case ASIO_STATE_FAILED:
            if (sdkctl->state == SDKCTL_SOCKET_CONNECTED) {
                /* This is disconnection condition. */
                action = _on_async_socket_disconnected(sdkctl);
            } else {
                /* An error has occurred while attempting to connect to socket.
                 * Lets try again... */
                action = ASIO_ACTION_RETRY;
            }
            break;

        case ASIO_STATE_RETRYING:
        default:
            action = ASIO_ACTION_RETRY;
            break;
    }

    sdkctl_socket_release(sdkctl);

    return action;
}

/********************************************************************************
 *                      SDK Control Socket public API
 *******************************************************************************/

SDKCtlSocket*
sdkctl_socket_new(int reconnect_to,
                  const char* service_name,
                  on_sdkctl_connection_cb on_connection,
                  on_sdkctl_handshake_cb on_handshake,
                  on_sdkctl_message_cb on_message,
                  void* opaque)
{
    SDKCtlSocket* sdkctl;
    ANEW0(sdkctl);

    sdkctl->state               = SDKCTL_SOCKET_DISCONNECTED;
    sdkctl->opaque              = opaque;
    sdkctl->service_name        = ASTRDUP(service_name);
    sdkctl->on_connection       = on_connection;
    sdkctl->on_handshake        = on_handshake;
    sdkctl->on_message          = on_message;
    sdkctl->reconnect_to        = reconnect_to;
    sdkctl->as                  = NULL;
    sdkctl->next_query_id       = 0;
    sdkctl->query_head          = sdkctl->query_tail = NULL;
    sdkctl->ref_count           = 1;
    sdkctl->recycler            = NULL;
    sdkctl->recycler_block_size = 0;
    sdkctl->recycler_max        = 0;
    sdkctl->recycler_count      = 0;

    sdkctl->looper = looper_newCore();
    if (sdkctl->looper == NULL) {
        E("Unable to create I/O looper for SDKCtl socket '%s'",
          service_name);
        on_connection(opaque, sdkctl, ASIO_STATE_FAILED);
        _sdkctl_socket_free(sdkctl);
        return NULL;
    }

    return sdkctl;
}

int sdkctl_socket_reference(SDKCtlSocket* sdkctl)
{
    assert(sdkctl->ref_count > 0);
    sdkctl->ref_count++;
    return sdkctl->ref_count;
}

int
sdkctl_socket_release(SDKCtlSocket* sdkctl)
{
    assert(sdkctl->ref_count > 0);
    sdkctl->ref_count--;
    if (sdkctl->ref_count == 0) {
        /* Last reference has been dropped. Destroy this object. */
        _sdkctl_socket_free(sdkctl);
        return 0;
    }
    return sdkctl->ref_count;
}

void
sdkctl_init_recycler(SDKCtlSocket* sdkctl,
                     uint32_t data_size,
                     int max_recycled_num)
{
    if (sdkctl->recycler != NULL) {
        D("SDKCtl %s: Recycler is already initialized. Ignoring recycler init.",
          sdkctl->service_name);
        return;
    }

    /* SDKCtlQuery is max descriptor sizeof. */
    data_size += sizeof(SDKCtlQuery);

    sdkctl->recycler_block_size = data_size;
    sdkctl->recycler_max        = max_recycled_num;
    sdkctl->recycler_count      = 0;
}

void
sdkctl_socket_connect(SDKCtlSocket* sdkctl, int port, int retry_to)
{
    T("SDKCtl %s: Handling connect request to port %d, retrying in %dms...",
      sdkctl->service_name, port, retry_to);

    sdkctl->state = SDKCTL_SOCKET_CONNECTING;
    sdkctl->as = async_socket_new(port, sdkctl->reconnect_to,
                                  _on_async_socket_connection, sdkctl,
                                  sdkctl->looper);
    if (sdkctl->as == NULL) {
        E("Unable to allocate AsyncSocket for SDKCtl socket '%s'",
           sdkctl->service_name);
        sdkctl->on_connection(sdkctl->opaque, sdkctl, ASIO_STATE_FAILED);
    } else {
        async_socket_connect(sdkctl->as, retry_to);
    }
}

void
sdkctl_socket_reconnect(SDKCtlSocket* sdkctl, int port, int retry_to)
{
    T("SDKCtl %s: Handling reconnection request to port %d, retrying in %dms...",
      sdkctl->service_name, port, retry_to);

    _sdkctl_socket_disconnect_socket(sdkctl);

    if (sdkctl->as == NULL) {
        sdkctl_socket_connect(sdkctl, port, retry_to);
    } else {
        sdkctl->state = SDKCTL_SOCKET_CONNECTING;
        async_socket_reconnect(sdkctl->as, retry_to);
    }
}

void
sdkctl_socket_disconnect(SDKCtlSocket* sdkctl)
{
    T("SDKCtl %s: Handling disconnect request.", sdkctl->service_name);

    _sdkctl_socket_disconnect_socket(sdkctl);
}


/********************************************************************************
 *                       Handshake query
 *******************************************************************************/

/* A callback that is ivoked on handshake I/O events. */
static AsyncIOAction
_on_handshake_io(void* query_opaque,
                 SDKCtlQuery* query,
                 AsyncIOState status)
{
    SDKCtlSocket* const sdkctl = (SDKCtlSocket*)query_opaque;

    if (status == ASIO_STATE_SUCCEEDED) {
        D("SDKCtl %s: %d bytes of handshake reply is received.",
          sdkctl->service_name, *query->response_size);

        /* Handshake is received. Inform the client. */
        sdkctl->on_handshake(sdkctl->opaque, sdkctl, *query->response_buffer,
                             *query->response_size, status);
    } else {
        /* Something is going on with the handshake... */
        switch (status) {
            case ASIO_STATE_FAILED:
            case ASIO_STATE_TIMED_OUT:
            case ASIO_STATE_CANCELLED:
              D("SDKCtl %s: Handshake failed: I/O state %d. Error: %d -> %s",
                sdkctl->service_name, status, errno, strerror(errno));
                sdkctl->on_handshake(sdkctl->opaque, sdkctl,
                                     *query->response_buffer,
                                     *query->response_size, status);
                break;

            default:
                break;
        }
    }
    return ASIO_ACTION_DONE;
}

static AsyncIOAction
_on_sdkctl_endianness_io(void* io_opaque,
                         AsyncSocketIO* asio,
                         AsyncIOState status) {
    SDKCtlSocket* const sdkctl = (SDKCtlSocket*)io_opaque;

    if (status == ASIO_STATE_SUCCEEDED) {
        /* Now it's time to initiate handshake message. */
        D("SDKCtl %s: Sending handshake query...", sdkctl->service_name);
        SDKCtlQuery* query =
            sdkctl_query_build_and_send(sdkctl, SDKCTL_QUERY_HANDSHAKE,
                                        strlen(sdkctl->service_name),
                                        sdkctl->service_name, NULL, NULL,
                                        _on_handshake_io, sdkctl, 3000);
        sdkctl_query_release(query);
        return ASIO_ACTION_DONE;
    } else {
        /* Something is going on with the endianness... */
        switch (status) {
                case ASIO_STATE_FAILED:
                case ASIO_STATE_TIMED_OUT:
                case ASIO_STATE_CANCELLED:
                  D("SDKCtl %s: endianness failed: I/O state %d. Error: %d -> %s",
                    sdkctl->service_name, status, errno, strerror(errno));
                    sdkctl->on_handshake(sdkctl->opaque, sdkctl, NULL, 0, status);
                    break;

                default:
                    break;
        }
    }
    return ASIO_ACTION_DONE;
}

static void
_sdkctl_do_handshake(SDKCtlSocket* sdkctl)
{
#ifndef HOST_WORDS_BIGENDIAN
static const char _host_end = 0;
#else
static const char _host_end = 1;
#endif

    D("SDKCtl %s: Sending endianness: %d...", sdkctl->service_name, _host_end);

    /* Before we can send any structured data to the SDK controller we need to
     * report endianness of the host. */
    async_socket_write_rel(sdkctl->as, &_host_end, 1,
                           _on_sdkctl_endianness_io, sdkctl, 3000);
}

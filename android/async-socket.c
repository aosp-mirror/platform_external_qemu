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
#include "utils/panic.h"
#include "iolooper.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(asyncsocket,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(asyncsocket)

/********************************************************************************
 *                  Asynchronous Socket internal API declarations
 *******************************************************************************/

/* Asynchronous socket I/O (reader, or writer) descriptor. */
typedef struct AsyncSocketIO AsyncSocketIO;

/* Gets socket's address string. */
static const char* _async_socket_string(AsyncSocket* as);

/* Gets socket's looper. */
static Looper* _async_socket_get_looper(AsyncSocket* as);

/* Handler for the I/O time out.
 * Param:
 *  as - Asynchronous socket for the reader.
 *  asio - Desciptor for the timed out I/O.
 */
static void _async_socket_io_timed_out(AsyncSocket* as, AsyncSocketIO* asio);

/********************************************************************************
 *                  Asynchronous Socket Reader / Writer
 *******************************************************************************/

struct AsyncSocketIO {
    /* Next I/O in the reader, or writer list. */
    AsyncSocketIO*      next;
    /* Asynchronous socket for this I/O. */
    AsyncSocket*        as;
    /* Timer used for time outs on this I/O. */
    LoopTimer           timer[1];
    /* An opaque pointer associated with this I/O. */
    void*               io_opaque;
    /* Buffer where to read / write data. */
    uint8_t*            buffer;
    /* Bytes to transfer through the socket for this I/O. */
    uint32_t            to_transfer;
    /* Bytes thransferred through the socket in this I/O. */
    uint32_t            transferred;
    /* I/O callbacks for this I/O. */
    const ASIOCb*       io_cb;
    /* I/O type selector: 1 - read, 0 - write. */
    int                 is_io_read;
};

/*
 * Recycling I/O instances.
 * Since AsyncSocketIO instances are not that large, it makes sence to recycle
 * them for faster allocation, rather than allocating and freeing them for each
 * I/O on the socket.
 */

/* List of recycled I/O descriptors. */
static AsyncSocketIO* _asio_recycled    = NULL;
/* Number of I/O descriptors that are recycled in the _asio_recycled list. */
static int _recycled_asio_count         = 0;
/* Maximum number of I/O descriptors that can be recycled. */
static const int _max_recycled_asio_num = 32;

/* Handler for an I/O time-out timer event.
 * When this routine is invoked, it indicates that a time out has occurred on an
 * I/O.
 * Param:
 *  opaque - AsyncSocketIO instance representing the timed out I/O.
 */
static void _on_async_socket_io_timed_out(void* opaque);

/* Creates new I/O descriptor.
 * Param:
 *  as - Asynchronous socket for the I/O.
 *  is_io_read - I/O type selector: 1 - read, 0 - write.
 *  buffer, len - Reader / writer buffer address.
 *  io_cb - Callbacks for this reader / writer.
 *  io_opaque - An opaque pointer associated with the I/O.
 *  deadline - Deadline to complete the I/O.
 * Return:
 *  Initialized AsyncSocketIO instance.
 */
static AsyncSocketIO*
_async_socket_rw_new(AsyncSocket* as,
                     int is_io_read,
                     void* buffer,
                     uint32_t len,
                     const ASIOCb* io_cb,
                     void* io_opaque,
                     Duration deadline)
{
    /* Lookup in the recycler first. */
    AsyncSocketIO* asio = _asio_recycled;
    if (asio != NULL) {
        /* Pull the descriptor from recycler. */
        _asio_recycled = asio->next;
        _recycled_asio_count--;
    } else {
        /* No recycled descriptors. Allocate new one. */
        ANEW0(asio);
    }

    asio->next          = NULL;
    asio->as            = as;
    asio->is_io_read    = is_io_read;
    asio->buffer        = (uint8_t*)buffer;
    asio->to_transfer   = len;
    asio->transferred   = 0;
    asio->io_cb         = io_cb;
    asio->io_opaque     = io_opaque;

    loopTimer_init(asio->timer, _async_socket_get_looper(as),
                   _on_async_socket_io_timed_out, asio);
    loopTimer_startAbsolute(asio->timer, deadline);

    return asio;
}

/* Destroys and frees I/O descriptor. */
static void
_async_socket_io_destroy(AsyncSocketIO* asio)
{
    loopTimer_stop(asio->timer);
    loopTimer_done(asio->timer);

    /* Try to recycle it first, and free the memory if recycler is full. */
    if (_recycled_asio_count < _max_recycled_asio_num) {
        asio->next = _asio_recycled;
        _asio_recycled = asio;
        _recycled_asio_count++;
    } else {
        AFREE(asio);
    }
}

/* Creates new asynchronous socket reader.
 * Param:
 *  as - Asynchronous socket for the reader.
 *  buffer, len - Reader's buffer.
 *  io_cb - Lists reader's callbacks.
 *  reader_opaque - An opaque pointer associated with the reader.
 *  deadline - Deadline to complete the operation.
 * Return:
 *  An initialized AsyncSocketIO intance.
 */
static AsyncSocketIO*
_async_socket_reader_new(AsyncSocket* as,
                         void* buffer,
                         uint32_t len,
                         const ASIOCb* io_cb,
                         void* reader_opaque,
                         Duration deadline)
{
    AsyncSocketIO* const asio = _async_socket_rw_new(as, 1, buffer, len, io_cb,
                                                     reader_opaque, deadline);
    return asio;
}

/* Creates new asynchronous socket writer.
 * Param:
 *  as - Asynchronous socket for the writer.
 *  buffer, len - Writer's buffer.
 *  io_cb - Lists writer's callbacks.
 *  writer_opaque - An opaque pointer associated with the writer.
 *  deadline - Deadline to complete the operation.
 * Return:
 *  An initialized AsyncSocketIO intance.
 */
static AsyncSocketIO*
_async_socket_writer_new(AsyncSocket* as,
                         const void* buffer,
                         uint32_t len,
                         const ASIOCb* io_cb,
                         void* writer_opaque,
                         Duration deadline)
{
    AsyncSocketIO* const asio = _async_socket_rw_new(as, 0, (void*)buffer, len,
                                                     io_cb, writer_opaque,
                                                     deadline);
    return asio;
}

/* I/O timed out. */
static void
_on_async_socket_io_timed_out(void* opaque)
{
    AsyncSocketIO* const asio = (AsyncSocketIO*)opaque;
    _async_socket_io_timed_out(asio->as, asio);
    _async_socket_io_destroy(asio);
}

/********************************************************************************
 *                      Asynchronous Socket internals
 *******************************************************************************/

struct AsyncSocket {
    /* TCP address for the socket. */
    SockAddress         address;
    /* Client callbacks for this socket. */
    const ASClientCb*   client_cb;
    /* An opaque pointer associated with this socket by the client. */
    void*               client_opaque;
    /* I/O looper for asynchronous I/O on the socket. */
    Looper*             looper;
    /* I/O descriptor for asynchronous I/O on the socket. */
    LoopIo              io[1];
    /* Timer to use for reconnection attempts. */
    LoopTimer           reconnect_timer[1];
    /* Head of the list of the active readers. */
    AsyncSocketIO*      readers_head;
    /* Tail of the list of the active readers. */
    AsyncSocketIO*      readers_tail;
    /* Head of the list of the active writers. */
    AsyncSocketIO*      writers_head;
    /* Tail of the list of the active writers. */
    AsyncSocketIO*      writers_tail;
    /* Socket's file descriptor. */
    int                 fd;
    /* Timeout to use for reconnection attempts. */
    int                 reconnect_to;
};

static const char*
_async_socket_string(AsyncSocket* as)
{
    return sock_address_to_string(&as->address);
}

static Looper*
_async_socket_get_looper(AsyncSocket* as)
{
    return as->looper;
}

/* Pulls first reader out of the list.
 * Param:
 *  as - Initialized AsyncSocket instance.
 * Return:
 *  First I/O pulled out of the list, or NULL if there are no I/O in the list.
 */
static AsyncSocketIO*
_async_socket_pull_first_io(AsyncSocket* as,
                            AsyncSocketIO** list_head,
                            AsyncSocketIO** list_tail)
{
    AsyncSocketIO* const ret = *list_head;
    if (ret != NULL) {
        *list_head = ret->next;
        ret->next = NULL;
        if (*list_head == NULL) {
            *list_tail = NULL;
        }
    }
    return ret;
}

/* Pulls first reader out of the list.
 * Param:
 *  as - Initialized AsyncSocket instance.
 * Return:
 *  First reader pulled out of the list, or NULL if there are no readers in the
 *  list.
 */
static AsyncSocketIO*
_async_socket_pull_first_reader(AsyncSocket* as)
{
    return _async_socket_pull_first_io(as, &as->readers_head, &as->readers_tail);
}

/* Pulls first writer out of the list.
 * Param:
 *  as - Initialized AsyncSocket instance.
 * Return:
 *  First writer pulled out of the list, or NULL if there are no writers in the
 *  list.
 */
static AsyncSocketIO*
_async_socket_pull_first_writer(AsyncSocket* as)
{
    return _async_socket_pull_first_io(as, &as->writers_head, &as->writers_tail);
}

/* Removes an I/O descriptor from a list of active I/O.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  list_head, list_tail - Pointers to the list head and tail.
 *  io - I/O to remove.
 * Return:
 *  Boolean: 1 if I/O has been removed, or 0 if I/O has not been found in the list.
 */
static int
_async_socket_remove_io(AsyncSocket* as,
                        AsyncSocketIO** list_head,
                        AsyncSocketIO** list_tail,
                        AsyncSocketIO* io)
{
    AsyncSocketIO* prev = NULL;

    while (*list_head != NULL && io != *list_head) {
        prev = *list_head;
        list_head = &((*list_head)->next);
    }
    if (*list_head == NULL) {
        D("%s: I/O %p is not found in the list for socket '%s'",
          __FUNCTION__, io, _async_socket_string(as));
        return 0;
    }

    *list_head = io->next;
    if (prev != NULL) {
        prev->next = io->next;
    }
    if (*list_tail == io) {
        *list_tail = prev;
    }

    return 1;
}

/* Advances to the next I/O in the list.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  list_head, list_tail - Pointers to the list head and tail.
 * Return:
 *  Next I/O at the head of the list, or NULL if I/O list become empty.
 */
static AsyncSocketIO*
_async_socket_advance_io(AsyncSocket* as,
                         AsyncSocketIO** list_head,
                         AsyncSocketIO** list_tail)
{
    AsyncSocketIO* first_io = *list_head;
    if (first_io != NULL) {
        *list_head = first_io->next;
        first_io->next = NULL;
    }
    if (*list_head == NULL) {
        *list_tail = NULL;
    }
    return *list_head;
}

/* Advances to the next reader in the list.
 * Param:
 *  as - Initialized AsyncSocket instance.
 * Return:
 *  Next reader at the head of the list, or NULL if reader list become empty.
 */
static AsyncSocketIO*
_async_socket_advance_reader(AsyncSocket* as)
{
    return _async_socket_advance_io(as, &as->readers_head, &as->readers_tail);
}

/* Advances to the next writer in the list.
 * Param:
 *  as - Initialized AsyncSocket instance.
 * Return:
 *  Next writer at the head of the list, or NULL if writer list become empty.
 */
static AsyncSocketIO*
_async_socket_advance_writer(AsyncSocket* as)
{
    return _async_socket_advance_io(as, &as->writers_head, &as->writers_tail);
}

/* Completes an I/O.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  asio - I/O to complete.
 */
static void
_async_socket_complete_io(AsyncSocket* as, AsyncSocketIO* asio)
{
    /* Stop the timer. */
    loopTimer_stop(asio->timer);

    /* Report I/O completion. First report via I/O callback, and only if it is
     * not set, report via client callback. */
    if (asio->io_cb && asio->io_cb->on_completed) {
        asio->io_cb->on_completed(as->client_opaque, as, asio->is_io_read,
                                  asio->io_opaque, asio->buffer, asio->transferred);
    } else if (as->client_cb->io_cb && as->client_cb->io_cb->on_completed) {
        as->client_cb->io_cb->on_completed(as->client_opaque, as, asio->is_io_read,
                                           asio->io_opaque, asio->buffer,
                                           asio->transferred);
    }
}

/* Timeouts an I/O.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  asio - An I/O that has timed out.
 */
static void
_async_socket_io_timed_out(AsyncSocket* as, AsyncSocketIO* asio)
{
    /* Remove the I/O from a list of active I/O. */
    if (asio->is_io_read) {
        _async_socket_remove_io(as, &as->readers_head, &as->readers_tail, asio);
    } else {
        _async_socket_remove_io(as, &as->writers_head, &as->writers_tail, asio);
    }

    /* Report I/O time out. First report it via I/O callbacks, and only if it is
     * not set, report it via client callbacks. */
    if (asio->io_cb && asio->io_cb->on_timed_out) {
        asio->io_cb->on_timed_out(as->client_opaque, as, asio->is_io_read,
                                  asio->io_opaque, asio->buffer,
                                  asio->transferred, asio->to_transfer);
    } else if (as->client_cb->io_cb && as->client_cb->io_cb->on_timed_out) {
        as->client_cb->io_cb->on_timed_out(as->client_opaque, as, asio->is_io_read,
                                           asio->io_opaque, asio->buffer,
                                           asio->transferred, asio->to_transfer);
    }
}

/* Cancels an I/O.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  asio - An I/O to cancel.
 */
static void
_async_socket_cancel_io(AsyncSocket* as, AsyncSocketIO* asio)
{
    /* Stop the timer. */
    loopTimer_stop(asio->timer);

    /* Report I/O cancellation. First report it via I/O callbacks, and only if it
     * is not set, report it via client callbacks. */
    if (asio->io_cb && asio->io_cb->on_cancelled) {
        asio->io_cb->on_cancelled(as->client_opaque, as, asio->is_io_read,
                                  asio->io_opaque, asio->buffer,
                                  asio->transferred, asio->to_transfer);
    } else if (as->client_cb->io_cb && as->client_cb->io_cb->on_cancelled) {
        as->client_cb->io_cb->on_cancelled(as->client_opaque, as, asio->is_io_read,
                                           asio->io_opaque, asio->buffer,
                                           asio->transferred, asio->to_transfer);
    }
}

/* Reports an I/O failure.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  asio - An I/O that has failed. Can be NULL for general failures.
 *  failure - Failure (errno) that has occurred.
 */
static void
_async_socket_io_failure(AsyncSocket* as, AsyncSocketIO* asio, int failure)
{
    /* Stop the timer. */
    loopTimer_stop(asio->timer);

    /* Report I/O failure. First report it via I/O callbacks, and only if it
     * is not set, report it via client callbacks. */
    if (asio && asio->io_cb && asio->io_cb->on_io_failure) {
        asio->io_cb->on_io_failure(as->client_opaque, as, asio->is_io_read,
                                   asio->io_opaque, asio->buffer,
                                   asio->transferred, asio->to_transfer, failure);
    } else if (as->client_cb->io_cb && as->client_cb->io_cb->on_io_failure) {
        as->client_cb->io_cb->on_io_failure(as->client_opaque, as,
                                            asio->is_io_read, asio->io_opaque,
                                            asio->buffer, asio->transferred,
                                            asio->to_transfer, failure);
    }
}

/* Cancels all the active socket readers.
 * Param:
 *  as - Initialized AsyncSocket instance.
 */
static void
_async_socket_cancel_readers(AsyncSocket* as)
{
    while (as->readers_head != NULL) {
        AsyncSocketIO* const to_cancel = _async_socket_pull_first_reader(as);
        _async_socket_cancel_io(as, to_cancel);
        _async_socket_io_destroy(to_cancel);
    }
}

/* Cancels all the active socket writers.
 * Param:
 *  as - Initialized AsyncSocket instance.
 */
static void
_async_socket_cancel_writers(AsyncSocket* as)
{
    while (as->writers_head != NULL) {
        AsyncSocketIO* const to_cancel = _async_socket_pull_first_writer(as);
        _async_socket_cancel_io(as, to_cancel);
        _async_socket_io_destroy(to_cancel);
    }
}

/* Cancels all the I/O on the socket. */
static void
_async_socket_cancel_all_io(AsyncSocket* as)
{
    /* Stop the reconnection timer. */
    loopTimer_stop(as->reconnect_timer);

    /* Stop read / write on the socket. */
    loopIo_dontWantWrite(as->io);
    loopIo_dontWantRead(as->io);

    /* Cancel active readers and writers. */
    _async_socket_cancel_readers(as);
    _async_socket_cancel_writers(as);
}

/* Closes socket handle used by the async socket.
 * Param:
 *  as - Initialized AsyncSocket instance.
 */
static void
_async_socket_close_socket(AsyncSocket* as)
{
    if (as->fd >= 0) {
        loopIo_done(as->io);
        socket_close(as->fd);
        as->fd = -1;
    }
}

/* Destroys AsyncSocket instance.
 * Param:
 *  as - Initialized AsyncSocket instance.
 */
static void
_async_socket_destroy(AsyncSocket* as)
{
    if (as != NULL) {
        /* Cancel all the I/O */
        _async_socket_cancel_all_io(as);

        /* Close socket. */
        _async_socket_close_socket(as);

        /* Free allocated resources. */
        if (as->looper != NULL) {
            loopTimer_done(as->reconnect_timer);
            looper_free(as->looper);
        }
        sock_address_done(&as->address);
        AFREE(as);
    }
}

/* Starts reconnection attempts after connection has been lost.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  to - Milliseconds to wait before reconnection attempt.
 */
static void
_async_socket_reconnect(AsyncSocket* as, int to)
{
    /* Make sure that no I/O is active, and socket is closed before we
     * reconnect. */
    _async_socket_cancel_all_io(as);

    /* Set the timer for reconnection attempt. */
    loopTimer_startRelative(as->reconnect_timer, to);
}

/********************************************************************************
 *                      Asynchronous Socket callbacks
 *******************************************************************************/

/* A callback that is invoked when socket gets disconnected.
 * Param:
 *  as - Initialized AsyncSocket instance.
 */
static void
_on_async_socket_disconnected(AsyncSocket* as)
{
    /* Save error to restore it for the client's callback. */
    const int save_errno = errno;
    ASConnectAction action = ASCA_ABORT;

    D("Async socket '%s' is disconnected. Error %d -> %s",
      _async_socket_string(as), errno, strerror(errno));

    /* Cancel all the I/O on this socket. */
    _async_socket_cancel_all_io(as);

    /* Close the socket. */
    _async_socket_close_socket(as);

    /* Restore errno, and invoke client's callback. */
    errno = save_errno;
    action = as->client_cb->on_connection(as->client_opaque, as,
                                          ASCS_DISCONNECTED);

    if (action == ASCA_RETRY) {
        /* Client requested reconnection. */
        if (as->reconnect_to) {
            _async_socket_reconnect(as, as->reconnect_to);
        }
    }
}

/* A callback that is invoked on socket's I/O failure.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  asio - Descriptor for the failed I/O. Can be NULL for general failures.
 */
static void
_on_async_socket_failure(AsyncSocket* as, AsyncSocketIO* asio)
{
    D("Async socket '%s' I/O failure %d: %s",
      _async_socket_string(as), errno, strerror(errno));

    /* Report the failure. */
    _async_socket_io_failure(as, asio, errno);
}

/* A callback that is invoked when there is data available to read.
 * Param:
 *  as - Initialized AsyncSocket instance.
 * Return:
 *  0 on success, or -1 on failure. Failure returned from this routine will
 *  skip writes (if awailable) behind this read.
 */
static int
_on_async_socket_recv(AsyncSocket* as)
{
    /* Get current reader. */
    AsyncSocketIO* const asr = as->readers_head;
    if (asr == NULL) {
        D("No async socket reader available on IO_READ for '%s'",
          _async_socket_string(as));
        loopIo_dontWantRead(as->io);
        return 0;
    }

    /* Read next chunk of data. */
    int res = socket_recv(as->fd, asr->buffer + asr->transferred,
                          asr->to_transfer - asr->transferred);
    while (res < 0 && errno == EINTR) {
        res = socket_recv(as->fd, asr->buffer + asr->transferred,
                          asr->to_transfer - asr->transferred);
    }

    if (res == 0) {
        /* Socket has been disconnected. */
        errno = ECONNRESET;
        _on_async_socket_disconnected(as);
        return -1;
    }

    if (res < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            /* Yield to writes behind this read. */
            loopIo_wantRead(as->io);
            return 0;
        }

        /* An I/O error. */
        _on_async_socket_failure(as, asr);
        return -1;
    }

    /* Update the reader's descriptor. */
    asr->transferred += res;
    if (asr->transferred == asr->to_transfer) {
        /* This read is completed. Move on to the next reader. */
        _async_socket_advance_reader(as);

        /* Notify reader completion. */
        _async_socket_complete_io(as, asr);
        _async_socket_io_destroy(asr);
    }

    /* Lets see if there are still active readers, and enable, or disable read
     * I/O callback accordingly. */
    if (as->readers_head != NULL) {
        loopIo_wantRead(as->io);
    } else {
        loopIo_dontWantRead(as->io);
    }

    return 0;
}

/* A callback that is invoked when there is data available to write.
 * Param:
 *  as - Initialized AsyncSocket instance.
 * Return:
 *  0 on success, or -1 on failure. Failure returned from this routine will
 *  skip reads (if awailable) behind this write.
 */
static int
_on_async_socket_send(AsyncSocket* as)
{
    /* Get current writer. */
    AsyncSocketIO* const asw = as->writers_head;
    if (asw == NULL) {
        D("No async socket writer available on IO_WRITE for '%s'",
          _async_socket_string(as));
        loopIo_dontWantWrite(as->io);
        return 0;
    }

    /* Write next chunk of data. */
    int res = socket_send(as->fd, asw->buffer + asw->transferred,
                          asw->to_transfer - asw->transferred);
    while (res < 0 && errno == EINTR) {
        res = socket_send(as->fd, asw->buffer + asw->transferred,
                          asw->to_transfer - asw->transferred);
    }

    if (res == 0) {
        /* Socket has been disconnected. */
        errno = ECONNRESET;
        _on_async_socket_disconnected(as);
        return -1;
    }

    if (res < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            /* Yield to reads behind this write. */
            loopIo_wantWrite(as->io);
            return 0;
        }

        /* An I/O error. */
        _on_async_socket_failure(as, asw);
        return -1;
    }

    /* Update the reader descriptor. */
    asw->transferred += res;
    if (asw->transferred == asw->to_transfer) {
        /* This write is completed. Move on to the next writer. */
        _async_socket_advance_writer(as);

        /* Notify writer completion. */
        _async_socket_complete_io(as, asw);
        _async_socket_io_destroy(asw);
    }

    /* Lets see if there are still active writers, and enable, or disable write
     * I/O callback accordingly. */
    if (as->writers_head != NULL) {
        loopIo_wantWrite(as->io);
    } else {
        loopIo_dontWantWrite(as->io);
    }

    return 0;
}

/* A callback that is invoked when an I/O is available on socket.
 * Param:
 *  as - Initialized AsyncSocket instance.
 *  fd - Socket's file descriptor.
 *  events - LOOP_IO_READ | LOOP_IO_WRITE bitmask.
 */
static void
_on_async_socket_io(void* opaque, int fd, unsigned events)
{
    AsyncSocket* const as = (AsyncSocket*)opaque;

    if ((events & LOOP_IO_READ) != 0) {
        if (_on_async_socket_recv(as) != 0) {
            return;
        }
    }

    if ((events & LOOP_IO_WRITE) != 0) {
        if (_on_async_socket_send(as) != 0) {
            return;
        }
    }
}

/* A callback that is invoked by asynchronous socket connector on connection
 *  events.
 * Param:
 *  opaque - Initialized AsyncSocket instance.
 *  connector - Connector that is used to connect this socket.
 *  event - Connection event.
 * Return:
 *  One of ASCCbRes values.
 */
static ASCCbRes
_on_connector_events(void* opaque,
                     AsyncSocketConnector* connector,
                     ASCEvent event)
{
    ASConnectAction action;
    ASConnectStatus adsc_status;
    AsyncSocket* const as = (AsyncSocket*)opaque;

    /* Convert connector event into async socket connection event. */
    switch (event) {
        case ASC_CONNECTION_SUCCEEDED:
            /* Accept the connection. */
            adsc_status = ASCS_CONNECTED;
            as->fd = async_socket_connector_pull_fd(connector);
            loopIo_init(as->io, as->looper, as->fd, _on_async_socket_io, as);
            break;

        case ASC_CONNECTION_RETRY:
            adsc_status = ASCS_RETRY;
            break;

        case ASC_CONNECTION_FAILED:
        default:
            adsc_status = ASCS_FAILURE;
            break;
    }

    /* Invoke client's callback. */
    action = as->client_cb->on_connection(as->client_opaque, as, adsc_status);
    if (event == ASC_CONNECTION_SUCCEEDED && action != ASCA_KEEP) {
        /* For whatever reason the client didn't want to keep this connection.
         * Close it. */
        _async_socket_close_socket(as);
    }

    if (action == ASCA_RETRY) {
        return ASC_CB_RETRY;
    } else if (action == ASCA_ABORT) {
        return ASC_CB_ABORT;
    } else {
        return ASC_CB_KEEP;
    }
}

/* Timer callback invoked to reconnect the lost connection.
 * Param:
 *  as - Initialized AsyncSocket instance.
 */
void
_on_async_socket_reconnect(void* opaque)
{
    AsyncSocket* as = (AsyncSocket*)opaque;
    async_socket_connect(as, as->reconnect_to);
}


/********************************************************************************
 *                  Android Device Socket public API
 *******************************************************************************/

AsyncSocket*
async_socket_new(int port,
                 int reconnect_to,
                 const ASClientCb* client_cb,
                 void* client_opaque)
{
    AsyncSocket* as;

    if (client_cb == NULL || client_cb->on_connection == NULL) {
        E("Invalid client_cb parameter");
        return NULL;
    }

    ANEW0(as);

    as->fd = -1;
    as->client_opaque = client_opaque;
    as->client_cb = client_cb;
    as->readers_head = as->readers_tail = NULL;
    as->reconnect_to = reconnect_to;
    sock_address_init_inet(&as->address, SOCK_ADDRESS_INET_LOOPBACK, port);
    as->looper = looper_newCore();
    if (as->looper == NULL) {
        E("Unable to create I/O looper for async socket '%s'",
          _async_socket_string(as));
        _async_socket_destroy(as);
        return NULL;
    }
    loopTimer_init(as->reconnect_timer, as->looper, _on_async_socket_reconnect, as);

    return as;
}

int
async_socket_connect(AsyncSocket* as, int retry_to)
{
    AsyncSocketConnector* const connector =
        async_socket_connector_new(&as->address, retry_to, _on_connector_events, as);
    if (connector == NULL) {
        return -1;
    }
    return (async_socket_connector_connect(connector) == ASC_CONNECT_FAILED) ? -1 : 0;
}

void
async_socket_disconnect(AsyncSocket* as)
{
    if (as != NULL) {
        _async_socket_cancel_all_io(as);
        _async_socket_close_socket(as);
        _async_socket_destroy(as);
    }
}

int
async_socket_reconnect(AsyncSocket* as, int retry_to)
{
    _async_socket_cancel_all_io(as);
    _async_socket_close_socket(as);
    return async_socket_connect(as, retry_to);
}

int
async_socket_read_abs(AsyncSocket* as,
                      void* buffer, uint32_t len,
                      const ASIOCb* reader_cb,
                      void* reader_opaque,
                      Duration deadline)
{
    AsyncSocketIO* const asr =
        _async_socket_reader_new(as, buffer, len, reader_cb, reader_opaque,
                                 deadline);
    if (as->readers_head == NULL) {
        as->readers_head = as->readers_tail = asr;
    } else {
        as->readers_tail->next = asr;
        as->readers_tail = asr;
    }
    loopIo_wantRead(as->io);
    return 0;
}

int
async_socket_read_rel(AsyncSocket* as,
                      void* buffer, uint32_t len,
                      const ASIOCb* reader_cb,
                      void* reader_opaque,
                      int to)
{
    const Duration dl = (to >= 0) ? looper_now(_async_socket_get_looper(as)) + to :
                                    DURATION_INFINITE;
    return async_socket_read_abs(as, buffer, len, reader_cb, reader_opaque, dl);
}

int
async_socket_write_abs(AsyncSocket* as,
                       const void* buffer, uint32_t len,
                       const ASIOCb* writer_cb,
                       void* writer_opaque,
                       Duration deadline)
{
    AsyncSocketIO* const asw =
        _async_socket_writer_new(as, buffer, len, writer_cb, writer_opaque,
                                 deadline);
    if (as->writers_head == NULL) {
        as->writers_head = as->writers_tail = asw;
    } else {
        as->writers_tail->next = asw;
        as->writers_tail = asw;
    }
    loopIo_wantWrite(as->io);
    return 0;
}

int async_socket_write_rel(AsyncSocket* as,
                           const void* buffer, uint32_t len,
                           const ASIOCb* writer_cb,
                           void* writer_opaque,
                           int to)
{
    const Duration dl = (to >= 0) ? looper_now(_async_socket_get_looper(as)) + to :
                                    DURATION_INFINITE;
    return async_socket_write_abs(as, buffer, len, writer_cb, writer_opaque, dl);
}

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

#include "android/android-device.h"
#include "utils/panic.h"
#include "iolooper.h"

#define  E(...)    derror(__VA_ARGS__)
#define  W(...)    dwarning(__VA_ARGS__)
#define  D(...)    VERBOSE_PRINT(adevice,__VA_ARGS__)
#define  D_ACTIVE  VERBOSE_CHECK(adevice)

/********************************************************************************
 *                       Common android device socket
 *******************************************************************************/

/* Milliseconds between retrying asynchronous connections to the device. */
#define ADS_RETRY_CONNECTION_TIMEOUT 3000

/* Socket type. */
typedef enum ADSType {
    /* Query socket. */
    ADS_TYPE_QUERY = 0,
    /* Events socket. */
    ADS_TYPE_EVENT = 1
} ADSType;

/* Status of the socket. */
typedef enum ADSStatus {
    /* Socket is disconnected. */
    ADS_DISCONNECTED,
    /* Connection process has been started. */
    ADS_CONNECTING,
    /* Connection has been established. */
    ADS_CONNECTED,
    /* Socket has been registered with the server. */
    ADS_REGISTERED,
} ADSStatus;

/* Identifies socket as a "query" socket with the server. */
static const char* _ads_query_socket_id = "query";
/* Identifies socket as an "event" socket with the server. */
static const char* _ads_event_socket_id = "event";

/* Android device socket descriptor. */
typedef struct AndroidDevSocket AndroidDevSocket;

/*
 * Callback routines.
 */

/* Callback routine that is called when a socket is connected.
 * Param:
 *  opaque - Opaque pointer associated with the socket. Typicaly it's the same
 *      pointer that is associated with AndroidDevice instance.
 *  ads - Connection socket.
 *  failure - If zero, indicates that socket has been successuly connected. If a
 *      connection error has occured, this parameter contains the error code (as
 *      in 'errno).
 */
typedef void (*ads_socket_connected_cb)(void* opaque,
                                        struct AndroidDevSocket* ads,
                                        int failure);

/* Android device socket descriptor. */
struct AndroidDevSocket {
    /* Socket type. */
    ADSType             type;
    /* Socket status. */
    ADSStatus           socket_status;
    /* TCP address for the socket. */
    SockAddress         address;
    /* Android device descriptor that owns the socket. */
    AndroidDevice*      ad;
    /* Opaque pointer associated with the socket. Typicaly it's the same
     * pointer that is associated with AndroidDevice instance.*/
    void*               opaque;
    /* Deadline for current I/O performed on the socket. */
    Duration            deadline;
    /* Socket's file descriptor. */
    int                 fd;
};

/* Query socket descriptor. */
typedef struct AndroidQuerySocket {
    /* Common device socket. */
    AndroidDevSocket    dev_socket;
} AndroidQuerySocket;

/* Describes data to send via an asynchronous socket. */
typedef struct AsyncSendBuffer {
    /* Next buffer to send. */
    struct AsyncSendBuffer* next;
    /* Callback to invoke when data transfer is completed. */
    async_send_cb           complete_cb;
    /* An opaque pointer to pass to the transfer completion callback. */
    void*                   complete_opaque;
    /* Data to send. */
    uint8_t*                data;
    /* Size of the entire data buffer. */
    int                     data_size;
    /* Remaining bytes to send. */
    int                     data_remaining;
    /* Boolean flag indicating whether to free data buffer upon completion. */
    int                     free_on_completion;
} AsyncSendBuffer;

/* Event socket descriptor. */
typedef struct AndroidEventSocket {
    /* Common socket descriptor. */
    AndroidDevSocket        dev_socket;
    /* Asynchronous connector to the device. */
    AsyncConnector          connector[1];
    /* I/O port for asynchronous I/O on this socket. */
    LoopIo                  io[1];
    /* Asynchronous string reader. */
    AsyncLineReader         alr;
    /* Callback to call at the end of the asynchronous connection to this socket.
     * Can be NULL. */
    ads_socket_connected_cb on_connected;
    /* Callback to call when an event is received on this socket. Can be NULL. */
    event_cb                on_event;
    /* Lists buffers that are pending to be sent. */
    AsyncSendBuffer*        send_pending;
} AndroidEventSocket;

/* Android device descriptor. */
struct AndroidDevice {
    /* Query socket for the device. */
    AndroidQuerySocket  query_socket;
    /* Event socket for the device. */
    AndroidEventSocket  event_socket;
    /* An opaque pointer associated with this descriptor. */
    void*               opaque;
    /* I/O looper for synchronous I/O on the sockets for this device. */
    IoLooper*           io_looper;
    /* Timer that is used to retry asynchronous connections. */
    LoopTimer           timer[1];
    /* I/O looper for asynchronous I/O. */
    Looper*             looper;
    /* Callback to call when device is fully connected. */
    device_connected_cb on_connected;
    /* I/O failure callback .*/
    io_failure_cb       on_io_failure;
};

/* Creates descriptor for a buffer to send asynchronously.
 * Param:
 *  data, size - Buffer to send.
 *  free_on_close - Boolean flag indicating whether to free data buffer upon
 *      completion.
 *  cb - Callback to invoke when data transfer is completed.
 *  opaque - An opaque pointer to pass to the transfer completion callback.
 */
static AsyncSendBuffer*
_async_send_buffer_create(void* data,
                          int size,
                          int free_on_close,
                          async_send_cb cb,
                          void* opaque)
{
    AsyncSendBuffer* desc = malloc(sizeof(AsyncSendBuffer));
    if (desc == NULL) {
        APANIC("Unable to allocate %d bytes for AsyncSendBuffer",
              sizeof(AsyncSendBuffer));
    }
    desc->next = NULL;
    desc->data = (uint8_t*)data;
    desc->data_size = desc->data_remaining = size;
    desc->free_on_completion = free_on_close;
    desc->complete_cb = cb;
    desc->complete_opaque = opaque;

    return desc;
}

/* Completes data transfer for the given descriptor.
 * Note that this routine will free the descriptor.
 * Param:
 *  desc - Asynchronous data transfer descriptor. Will be freed upon the exit
 *      from this routine.
 *  res - Data transfer result.
 */
static void
_async_send_buffer_complete(AsyncSendBuffer* desc, ATResult res)
{
    /* Invoke completion callback (if present) */
    if (desc->complete_cb) {
        desc->complete_cb(desc->complete_opaque, res, desc->data, desc->data_size,
                          desc->data_size - desc->data_remaining);
    }

    /* Free data buffer (if required) */
    if (desc->free_on_completion) {
        free(desc->data);
    }

    /* Free the descriptor itself. */
    free(desc);
}

/********************************************************************************
 *                        Common socket declarations
 *******************************************************************************/

/* Initializes common device socket.
 * Param:
 *  ads - Socket descriptor to initialize.
 *  opaque - An opaque pointer to associate with the socket. Typicaly it's the
 *      same pointer that is associated with AndroidDevice instance.
 *  ad - Android device descriptor that owns the socket.
 *  port - Socket's TCP port.
 *  type - Socket type (query, or event).
 */
static int _android_dev_socket_init(AndroidDevSocket* ads,
                                    void* opaque,
                                    AndroidDevice* ad,
                                    int port,
                                    ADSType type);

/* Destroys socket descriptor. */
static void _android_dev_socket_destroy(AndroidDevSocket* ads);

/* Callback that is ivoked from _android_dev_socket_connect when a file
 * descriptor has been created for a socket.
 * Param:
 *  ads - Socket descritor.
 *  opaque - An opaque pointer associated with the callback.
 */
typedef void (*on_socked_fd_created)(AndroidDevSocket* ads, void* opaque);

/* Synchronously connects to the socket, and registers it with the server.
 * Param:
 *  ads - Socket to connect. Must have 'deadline' field properly setup.
 *  cb, opaque - A callback to invoke (and opaque parameters to pass to the
 *      callback) when a file descriptor has been created for a socket. These
 *      parameters are optional and can be NULL.
 * Return:
 *  0 on success, -1 on failure with errno containing the reason for failure.
 */
static int _android_dev_socket_connect(AndroidDevSocket* ads,
                                       on_socked_fd_created cb,
                                       void* opaque);

/* Synchronously registers a connected socket with the server.
 * Param:
 *  ads - Socket to register. Must be connected, and must have 'deadline' field
 *      properly setup.
 * Return:
 *  0 on success, -1 on failure with errno containing the reason for failure.
 */
static int _android_dev_socket_register(AndroidDevSocket* ads);

/* Disconnects the socket (if it was connected) */
static void _android_dev_socket_disconnect(AndroidDevSocket* ads);

/* Synchronously sends data to the socket.
 * Param:
 *  ads - Socket to send the data to. Must be connected, and must have 'deadline'
 *      field properly setup.
 *  buff, buffsize - Buffer to send.
 * Return:
 *  Number of bytes sent on success, or -1 on failure with errno containing the
 *  reason for failure.
 */
static int _android_dev_socket_send(AndroidDevSocket* ads,
                                    const char* buff,
                                    int buffsize);

/* Synchronously receives data from the socket.
 * Param:
 *  ads - Socket to receive the data from. Must be connected, and must have
 *      'deadline' field properly setup.
 *  buff, buffsize - Buffer where to receive data.
 * Return:
 *  Number of bytes received on success, or -1 on failure with errno containing
 *  the reason for failure.
 */
static int _android_dev_socket_recv(AndroidDevSocket* ads,
                                    char* buf,
                                    int bufsize);

/* Synchronously reads zero-terminated string from the socket.
 * Param:
 *  ads - Socket to read the string from. Must be connected, and must have
 *      'deadline' field properly setup.
 *  str, strsize - Buffer where to read the string.
 * Return:
 *  Number of charactes read into the string buffer (including zero-terminator)
 *  on success, or -1 on failure with 'errno' containing the reason for failure.
 *  If this routine returns -1, and errno contains ENOMEM, this is an indicator
 *  that supplied string buffer was too small for the receiving string.
 */
static int _android_dev_socket_read_string(AndroidDevSocket* ads,
                                           char* str,
                                           int strsize);

/* Synchronously reads zero-terminated query response from the socket.
 * All queries respond with an 'ok', or 'ko' prefix, indicating a success, or
 * failure. Prefix can be followed by more query response data, separated from
 * the prefix with a ':' character. This routine helps separating prefix from the
 * data, by placing only the query response data into provided buffer. 'ko' or
 * 'ok' will be encoded in the return value.
 * Param:
 *  ads - Socket to read the response from. Must be connected, and must have
 *      'deadline' field properly setup.
 *  data, datasize - Buffer where to read the query response data.
 * Return:
 *  Number of charactes read into the data buffer (including zero-terminator) on
 *  success, or -1 on failure with errno containing the reason for failure.
 *  If the query has been completed with 'ko', this routine will return -1, with
 *  errno set to 0. If this routine returned -1, and errno is set to EINVAL, this
 *  indicates that reply string didn't match expected query reply format.
 */
static int _android_dev_socket_read_response(AndroidDevSocket* ads,
                                             char* str,
                                             int strsize);

/* Gets ID string for the channel. */
AINLINED const char*
_ads_id_str(AndroidDevSocket* ads)
{
    return (ads->type == ADS_TYPE_QUERY) ? _ads_query_socket_id :
                                           _ads_event_socket_id;
}

/* Gets socket's TCP port. */
AINLINED int
_ads_port(AndroidDevSocket* ads)
{
    return sock_address_get_port(&ads->address);
}

/* Gets synchronous I/O looper for the socket. */
AINLINED IoLooper*
_ads_io_looper(AndroidDevSocket* ads)
{
    return ads->ad->io_looper;
}

/* Sets deadline on a socket operation, given relative timeout.
 * Param:
 *  ads - Socket descriptor to set deadline for.
 *  to - Relative timeout (in millisec) for the operation.
 *      AD_INFINITE_WAIT passed in this parameter means "no deadline".
 */
AINLINED void
_ads_set_deadline(AndroidDevSocket* ads, int to)
{
    ads->deadline = (to == AD_INFINITE_WAIT) ? DURATION_INFINITE :
                                               iolooper_now() + to;
}

/********************************************************************************
 *                        Common socket implementation
 *******************************************************************************/

static int
_android_dev_socket_init(AndroidDevSocket* ads,
                         void* opaque,
                         AndroidDevice* ad,
                         int port,
                         ADSType type)
{
    ads->type = type;
    ads->socket_status = ADS_DISCONNECTED;
    ads->opaque = opaque;
    ads->ad = ad;
    ads->fd = -1;
    sock_address_init_inet(&ads->address, SOCK_ADDRESS_INET_LOOPBACK, port);

    return 0;
}

static void
_android_dev_socket_destroy(AndroidDevSocket* ads)
{
    /* Make sure it's disconnected. */
    _android_dev_socket_disconnect(ads);

    /* Finalize socket address. */
    sock_address_done(&ads->address);
    memset(&ads->address, 0, sizeof(ads->address));
}

static int
_android_dev_socket_connect(AndroidDevSocket* ads,
                            on_socked_fd_created cb,
                            void* opaque)
{
    int res;

    /* Create communication socket. */
    ads->fd = socket_create_inet(SOCKET_STREAM);
    if (ads->fd < 0) {
        D("Unable to create socket for channel '%s'@%d: %s",
          _ads_id_str(ads), _ads_port(ads), strerror(errno));
        return -1;
    }
    socket_set_nonblock(ads->fd);

    /* Invoke FD creation callback (if required) */
    if (cb != NULL) {
        cb(ads, opaque);
    }

    /* Synchronously connect to it. */
    ads->socket_status = ADS_CONNECTING;
    iolooper_add_write(_ads_io_looper(ads), ads->fd);
    res = socket_connect(ads->fd, &ads->address);
    while (res < 0 && errno == EINTR) {
        res = socket_connect(ads->fd, &ads->address);
    }

    if (res && (errno == EINPROGRESS || errno == EWOULDBLOCK || errno == EAGAIN)) {
        /* Connection is delayed. Wait for it until timeout expires. */
        res = iolooper_wait_absolute(_ads_io_looper(ads), ads->deadline);
        if (res > 0) {
            /* Pick up on possible connection error. */
            errno = socket_get_error(ads->fd);
            res = (errno == 0) ? 0 : -1;
        } else {
            res = -1;
        }
    }
    iolooper_del_write(_ads_io_looper(ads), ads->fd);

    if (res == 0) {
        D("Channel '%s'@%d is connected", _ads_id_str(ads), _ads_port(ads));
        /* Socket is connected. Now register it with the server. */
        ads->socket_status = ADS_CONNECTED;
        res = _android_dev_socket_register(ads);
    } else {
        D("Unable to connect channel '%s' to port %d: %s",
          _ads_id_str(ads), _ads_port(ads), strerror(errno));
    }

    if (res) {
        _android_dev_socket_disconnect(ads);
    }

    return res;
}

static int
_android_dev_socket_register(AndroidDevSocket* ads)
{
    /* Make sure that socket is connected. */
    if (ads->socket_status < ADS_CONNECTED) {
        D("Attempt to register a disconnected channel '%s'@%d",
          _ads_id_str(ads),  _ads_port(ads));
        errno = ECONNRESET;
        return -1;
    }

    /* Register this socket accordingly to its type. */
    const char* reg_str = _ads_id_str(ads);
    int res = _android_dev_socket_send(ads, reg_str, strlen(reg_str) + 1);
    if (res > 0) {
        /* Receive reply. Note that according to the protocol, the server should
         * reply to channel registration with 'ok', or 'ko' (just like with queries),
         * so we can use query reply reader here. */
        char reply[256];
        res = _android_dev_socket_read_response(ads, reply, sizeof(reply));
        if (res >= 0) {
            /* Socket is now registered. */
            ads->socket_status = ADS_REGISTERED;
            D("Channel '%s'@%d is registered", _ads_id_str(ads), _ads_port(ads));
            res = 0;
        } else {
            if (errno == 0) {
                /* 'ko' condition */
                D("Device failed registration of channel '%s'@%d: %s",
                  _ads_id_str(ads), _ads_port(ads), reply);
                errno = EINVAL;
            } else {
                D("I/O failure while registering channel '%s'@%d: %s",
                  _ads_id_str(ads), _ads_port(ads), strerror(errno));
            }
            res = -1;
        }
    } else {
        D("Unable to send registration query for channel '%s'@%d: %s",
          _ads_id_str(ads), _ads_port(ads), strerror(errno));
        res = -1;
    }

    return res;
}

static void
_android_dev_socket_disconnect(AndroidDevSocket* ads)
{
    /* Preserve errno */
    const int save_error = errno;
    if (ads->socket_status != ADS_DISCONNECTED) {
        /* Reset I/O looper for this socket. */
        iolooper_modify(_ads_io_looper(ads), ads->fd,
                        IOLOOPER_READ | IOLOOPER_WRITE, 0);

        /* Mark as disconnected. */
        ads->socket_status = ADS_DISCONNECTED;

        /* Close socket. */
        if (ads->fd >= 0) {
            socket_close(ads->fd);
            ads->fd = -1;
        }
    }
    errno = save_error;
}

static int
_android_dev_socket_send(AndroidDevSocket* ads, const char* buff, int to_send)
{
    int sent = 0;

    /* Make sure that socket is connected. */
    if (ads->socket_status < ADS_CONNECTED) {
        D("Attempt to send via disconnected channel '%s'@%d",
          _ads_id_str(ads),  _ads_port(ads));
        errno = ECONNRESET;
        return -1;
    }

    iolooper_add_write(_ads_io_looper(ads), ads->fd);
    do {
        int res = socket_send(ads->fd, buff + sent, to_send - sent);
        if (res == 0) {
            /* Disconnection. */
            errno = ECONNRESET;
            sent = -1;
            break;
        }

        if (res < 0) {
            if (errno == EINTR) {
                /* loop on EINTR */
                continue;
            }

            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                res = iolooper_wait_absolute(_ads_io_looper(ads), ads->deadline);
                if (res > 0) {
                    /* Ready to write. */
                    continue;
                }
            }
            sent = -1;
            break;
        }
        sent += res;
    } while (sent < to_send);
    iolooper_del_write(_ads_io_looper(ads), ads->fd);

    /* In case of an I/O failure we have to invoke failure callback. Note that we
     * report I/O failures only on registered sockets. */
    if (sent < 0) {
        D("I/O error while sending data via channel '%s'@%d: %s",
          _ads_id_str(ads), _ads_port(ads), strerror(errno));

        if (ads->ad->on_io_failure != NULL && ads->socket_status > ADS_CONNECTED) {
            const char save_error = errno;
            ads->ad->on_io_failure(ads->opaque, ads->ad, save_error);
            errno = save_error;
        }
    }

    return sent;
}

static int
_android_dev_socket_recv(AndroidDevSocket* ads, char* buf, int bufsize)
{
    int recvd = 0;

    /* Make sure that socket is connected. */
    if (ads->socket_status < ADS_CONNECTED) {
        D("Attempt to receive from disconnected channel '%s'@%d",
          _ads_id_str(ads),  _ads_port(ads));
        errno = ECONNRESET;
        return -1;
    }

    iolooper_add_read(_ads_io_looper(ads), ads->fd);
    do {
        int res = socket_recv(ads->fd, buf + recvd, bufsize - recvd);
        if (res == 0) {
            /* Disconnection. */
            errno = ECONNRESET;
            recvd = -1;
            break;
        }

        if (res < 0) {
            if (errno == EINTR) {
                /* loop on EINTR */
                continue;
            }

            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                res = iolooper_wait_absolute(_ads_io_looper(ads), ads->deadline);
                if (res > 0) {
                    /* Ready to read. */
                    continue;
                }
            }
            recvd = -1;
            break;
        }
        recvd += res;
    } while (recvd < bufsize);
    iolooper_del_read(_ads_io_looper(ads), ads->fd);

    /* In case of an I/O failure we have to invoke failure callback. Note that we
     * report I/O failures only on registered sockets. */
    if (recvd < 0) {
        D("I/O error while receiving from channel '%s'@%d: %s",
          _ads_id_str(ads), _ads_port(ads), strerror(errno));

         if (ads->ad->on_io_failure != NULL && ads->socket_status > ADS_CONNECTED) {
            const char save_error = errno;
            ads->ad->on_io_failure(ads->opaque, ads->ad, save_error);
            errno = save_error;
         }
    }

    return recvd;
}

static int
_android_dev_socket_read_string(AndroidDevSocket* ads, char* str, int strsize)
{
    int n;

    /* Char by char read from the socket, until zero-terminator is read. */
    for (n = 0; n < strsize; n++) {
        if (_android_dev_socket_recv(ads, str + n, 1) > 0) {
            if (str[n] == '\0') {
                /* Done. */
                return n + 1;   /* Including zero-terminator. */
            }
        } else {
            /* I/O error. */
            return -1;
        }
    }

    /* Buffer was too small. Report that by setting errno to ENOMEM. */
    D("Buffer %d is too small to receive a string from channel '%s'@%d",
       strsize, _ads_id_str(ads), _ads_port(ads));
    errno = ENOMEM;
    return -1;
}

static int
_android_dev_socket_read_response(AndroidDevSocket* ads, char* data, int datasize)
{
    int n, res;
    int success = 0;
    int failure = 0;
    int bad_format = 0;
    char ok[4];

    *data = '\0';

    /* Char by char read from the socket, until ok/ko is read. */
    for (n = 0; n < 2; n++) {
        res = _android_dev_socket_recv(ads, ok + n, 1);
        if (res > 0) {
            if (ok[n] == '\0') {
                /* EOS is unexpected here! */
                D("Bad query reply format on channel '%s'@%d: '%s' is too short.",
                  _ads_id_str(ads), _ads_port(ads), ok);
                errno = EINVAL;
                return -1;
            }
        } else {
            /* I/O error. */
            return -1;
        }
    }

    /* Next character must be either ':', or '\0' */
    res = _android_dev_socket_recv(ads, ok + n, 1);
    if (res <= 0) {
        /* I/O error. */
        return -1;
    }

    /*
     * Verify format.
     */

    /* Check ok / ko */
    success = memcmp(ok, "ok", 2) == 0;
    failure = memcmp(ok, "ko", 2) == 0;

    /* Check the prefix: 'ok'|'ko' & ':'|'\0' */
    if ((success || failure) && (ok[n] == '\0' || ok[n] == ':')) {
        /* Format is good. */
        if (ok[n] == '\0') {
            /* We're done: no extra data in response. */
            errno = 0;
            return success ? 0 : -1;
        }
        /* Reset buffer offset, so we will start to read the remaining query
         * data to the beginning of the supplied buffer. */
        n = 0;
    } else {
        /* Bad format. Lets move what we've read to the main buffer, and
         * continue filling it in. */
        bad_format = 1;
        n++;
        memcpy(data, ok, n);
    }

    /* Read the remainder of reply to the supplied data buffer. */
    res = _android_dev_socket_read_string(ads, data + n, datasize - n);
    if (res < 0) {
        return res;
    }

    /* Lets see if format was bad */
    if (bad_format) {
        D("Bad query reply format on channel '%s'@%d: %s.",
          _ads_id_str(ads), _ads_port(ads), data);
        errno = EINVAL;
        return -1;
    } else {
        errno = 0;
        return success ? n : -1;
    }
}

/********************************************************************************
 *                        Query socket declarations
 *******************************************************************************/

/* Initializes query socket descriptor.
 * Param:
 *  adsquery - Socket descriptor to initialize.
 *  opaque - An opaque pointer to associate with the socket. Typicaly it's the
 *      same pointer that is associated with AndroidDevice instance.
 *  ad - Android device descriptor that owns the socket.
 *  port - TCP socket port.
 */
static int _android_query_socket_init(AndroidQuerySocket* adsquery,
                                      void* opaque,
                                      AndroidDevice* ad,
                                      int port);

/* Destroys query socket descriptor. */
static void _android_query_socket_destroy(AndroidQuerySocket* adsquery);

/* Synchronously connects the query socket, and registers it with the server.
 * Param:
 *  adsquery - Descriptor for the query socket to connect. Must have 'deadline'
 *      field properly setup.
 *  cb - Callback to invoke when socket connection is completed. Can be NULL.
 * Return:
 *  Zero on success, or non-zero on failure.
 */
static int _android_query_socket_connect(AndroidQuerySocket* adsquery);

/* Disconnects the query socket. */
static void _android_query_socket_disconnect(AndroidQuerySocket* adsquery);

/********************************************************************************
 *                        Query socket implementation
 *******************************************************************************/

static int
_android_query_socket_init(AndroidQuerySocket* adsquery,
                           void* opaque,
                           AndroidDevice* ad,
                           int port)
{
    return _android_dev_socket_init(&adsquery->dev_socket, opaque, ad, port,
                                    ADS_TYPE_QUERY);
}

static void
_android_query_socket_destroy(AndroidQuerySocket* adsquery)
{
    _android_query_socket_disconnect(adsquery);
    _android_dev_socket_destroy(&adsquery->dev_socket);
}

static int
_android_query_socket_connect(AndroidQuerySocket* adsquery)
{
    return _android_dev_socket_connect(&adsquery->dev_socket, NULL, NULL);
}

static void
_android_query_socket_disconnect(AndroidQuerySocket* adsquery)
{
    _android_dev_socket_disconnect(&adsquery->dev_socket);
}

/********************************************************************************
 *                       Events socket declarations
 *******************************************************************************/

/* Initializes event socket descriptor.
 * Param:
 *  adsevent - Socket descriptor to initialize.
 *  opaque - An opaque pointer to associate with the socket.  Typicaly it's the
 *      same pointer that is associated with AndroidDevice instance.
 *  ad - Android device descriptor that owns the socket.
 *  port - TCP socket port.
 */
static int _android_event_socket_init(AndroidEventSocket* adsevent,
                                      void* opaque,
                                      AndroidDevice* ad,
                                      int port);

/* Destroys the event socket descriptor. */
static void _android_event_socket_destroy(AndroidEventSocket* adsevent);

/* Synchronously connects event socket.
 * Param:
 *  adsevent - Descriptor for the event socket to connect. Must have 'deadline'
 *      field properly setup.
 * Return:
 *  Zero on success, or non-zero on failure.
 */
static int _android_event_socket_connect_sync(AndroidEventSocket* adsevent);

/* Initiates asynchronous event socket connection.
 * Param:
 *  adsevent - Descriptor for the event socket to connect.  Must have 'deadline'
 *      field properly setup.
 *  cb - Callback to invoke when socket connection is completed. Can be NULL.
 * Return:
 *  Zero on success, or non-zero on failure.
 */
static int _android_event_socket_connect_async(AndroidEventSocket* adsevent,
                                               ads_socket_connected_cb cb);

/* Disconnects the event socket. */
static void _android_event_socket_disconnect(AndroidEventSocket* adsevent);

/* Initiates listening on the event socket.
 * Param:
 *  adsevent - Descriptor for the event socket to listen on.
 *  str, strsize - Buffer where to read the string.
 *  cb - A callback to call when the event string is read. Can be NULL.
 * Return:
 *  Zero on success, or non-zero on failure.
 */
static int _android_event_socket_listen(AndroidEventSocket* adsevent,
                                        char* str,
                                        int strsize,
                                        event_cb cb);

/* Asynchronously sends data via event socket.
 * Param:
 *  adsevent - Descriptor for the event socket to send data to.
 *  data, size - Buffer containing data to send.
 *  free_on_close - A boolean flag indicating whether the data buffer should be
 *      freed upon data transfer completion.
 *  cb - Callback to invoke when data transfer is completed.
 *  opaque - An opaque pointer to pass to the transfer completion callback.
 */
static int _android_event_socket_send(AndroidEventSocket* adsevent,
                                      void* data,
                                      int size,
                                      int free_on_close,
                                      async_send_cb cb,
                                      void* opaque);

/* Cancels all asynchronous data transfers on the event socket.
 * Param:
 *  adsevent - Descriptor for the event socket to cancel data transfer.
 *  reason - Reason for the cancellation.
 */
static void _android_event_socket_cancel_send(AndroidEventSocket* adsevent,
                                              ATResult reason);

/* Event socket's asynchronous I/O looper callback.
 * Param:
 *  opaque - AndroidEventSocket instance.
 *  fd - Socket's FD.
 *  events - I/O type bitsmask (read | write).
 */
static void _on_event_socket_io(void* opaque, int fd, unsigned events);

/* Callback that is invoked when asynchronous event socket connection is
 * completed. */
static void _on_event_socket_connected(AndroidEventSocket* adsevent, int failure);

/* Callback that is invoked when an event is received from the device. */
static void _on_event_received(AndroidEventSocket* adsevent);

/* Gets I/O looper for asynchronous I/O on event socket. */
AINLINED Looper*
_aes_looper(AndroidEventSocket* adsevent)
{
    return adsevent->dev_socket.ad->looper;
}

/********************************************************************************
 *                       Events socket implementation
 *******************************************************************************/

static int
_android_event_socket_init(AndroidEventSocket* adsevent,
                           void* opaque,
                           AndroidDevice* ad,
                           int port)
{
    return _android_dev_socket_init(&adsevent->dev_socket, opaque, ad, port,
                                    ADS_TYPE_EVENT);
}

static void
_android_event_socket_destroy(AndroidEventSocket* adsevent)
{
    _android_event_socket_disconnect(adsevent);
    _android_dev_socket_destroy(&adsevent->dev_socket);
}

/* A callback invoked when file descriptor is created for the event socket.
 * We use this callback to initialize the event socket for async I/O right after
 * the FD has been created.
 */
static void
_on_event_fd_created(AndroidDevSocket* ads, void* opaque)
{
    AndroidEventSocket* adsevent = (AndroidEventSocket*)opaque;
   /* Prepare for async I/O on the event socket. */
   loopIo_init(adsevent->io, _aes_looper(adsevent), ads->fd,
               _on_event_socket_io, adsevent);
}

static int
_android_event_socket_connect_sync(AndroidEventSocket* adsevent)
{
    return _android_dev_socket_connect(&adsevent->dev_socket,
                                       _on_event_fd_created, adsevent);
}

static int
_android_event_socket_connect_async(AndroidEventSocket* adsevent,
                                    ads_socket_connected_cb cb)
{
    AsyncStatus status;
    AndroidDevSocket* ads = &adsevent->dev_socket;

    /* Create asynchronous socket. */
    ads->fd = socket_create_inet(SOCKET_STREAM);
    if (ads->fd < 0) {
        D("Unable to create socket for channel '%s'@%d: %s",
          _ads_id_str(ads), _ads_port(ads), strerror(errno));
        if (cb != NULL) {
            cb(ads->opaque, ads, errno);
        }
        return -1;
    }
    socket_set_nonblock(ads->fd);

    /* Prepare for async I/O on the event socket. */
    loopIo_init(adsevent->io, _aes_looper(adsevent), ads->fd,
                _on_event_socket_io, adsevent);

    /* Try to connect. */
    ads->socket_status = ADS_CONNECTING;
    adsevent->on_connected = cb;
    status = asyncConnector_init(adsevent->connector, &ads->address, adsevent->io);
    switch (status) {
        case ASYNC_COMPLETE:
            /* We're connected to the device socket. */
            ads->socket_status = ADS_CONNECTED;
            _on_event_socket_connected(adsevent, 0);
            break;
        case ASYNC_ERROR:
            _on_event_socket_connected(adsevent, errno);
            break;
        case ASYNC_NEED_MORE:
            /* Attempt to connect would block, so connection competion is
             * delegates to the looper's I/O routine. */
        default:
            break;
    }

    return 0;
}

static void
_android_event_socket_disconnect(AndroidEventSocket* adsevent)
{
    AndroidDevSocket* ads = &adsevent->dev_socket;

    if (ads->socket_status != ADS_DISCONNECTED) {
        /* Cancel data transfer. */
        _android_event_socket_cancel_send(adsevent, ATR_DISCONNECT);

        /* Stop all async I/O. */
        loopIo_done(adsevent->io);

        /* Disconnect common socket. */
        _android_dev_socket_disconnect(ads);
    }
}

static int
_android_event_socket_listen(AndroidEventSocket* adsevent,
                             char* str,
                             int strsize,
                             event_cb cb)
{
    AsyncStatus  status;
    AndroidDevSocket* ads = &adsevent->dev_socket;

    /* Make sure that device is connected. */
    if (ads->socket_status < ADS_CONNECTED) {
        D("Attempt to listen on a disconnected channel '%s'@%d",
          _ads_id_str(ads),  _ads_port(ads));
        errno = ECONNRESET;
        return -1;
    }

    /* NOTE: only one reader at any given time! */
    adsevent->on_event = cb;
    asyncLineReader_init(&adsevent->alr, str, strsize, adsevent->io);
    /* Default EOL for the line reader was '\n'. */
    asyncLineReader_setEOL(&adsevent->alr, '\0');
    status = asyncLineReader_read(&adsevent->alr);
    if (status == ASYNC_COMPLETE) {
        /* Data has been transferred immediately. Do the callback here. */
        _on_event_received(adsevent);
    } else if (status == ASYNC_ERROR) {
        D("Error while listening on channel '%s'@%d: %s",
          _ads_id_str(ads),  _ads_port(ads), strerror(errno));
        /* There is one special failure here, when buffer was too small to
         * contain the entire string. This is not an I/O, but rather a
         * protocol error. So we don't report it to the I/O failure
         * callback. */
        if (errno == ENOMEM) {
            _on_event_received(adsevent);
        } else {
            if (ads->ad->on_io_failure != NULL) {
                ads->ad->on_io_failure(ads->ad->opaque, ads->ad, errno);
            }
        }
        return -1;
    }
    return 0;
}

static int
_android_event_socket_send(AndroidEventSocket* adsevent,
                           void* data,
                           int size,
                           int free_on_close,
                           async_send_cb cb,
                           void* opaque)
{
    /* Create data transfer descriptor, and place it at the end of the list. */
    AsyncSendBuffer* const desc =
        _async_send_buffer_create(data, size, free_on_close, cb, opaque);
    AsyncSendBuffer** place = &adsevent->send_pending;
    while (*place != NULL) {
        place = &((*place)->next);
    }
    *place = desc;

    /* We're ready to transfer data. */
    loopIo_wantWrite(adsevent->io);

    return 0;
}

static void
_android_event_socket_cancel_send(AndroidEventSocket* adsevent, ATResult reason)
{
    while (adsevent->send_pending != NULL) {
        AsyncSendBuffer* const to_cancel = adsevent->send_pending;
        adsevent->send_pending = to_cancel->next;
        _async_send_buffer_complete(to_cancel, reason);
    }
    loopIo_dontWantWrite(adsevent->io);
}

static void
_on_event_socket_io(void* opaque, int fd, unsigned events)
{
    AsyncStatus status;
    AndroidEventSocket* adsevent = (AndroidEventSocket*)opaque;
    AndroidDevSocket* ads = &adsevent->dev_socket;

    /* Lets see if we're still wating on a connection to occur. */
    if (ads->socket_status == ADS_CONNECTING) {
        /* Complete socket connection. */
        status = asyncConnector_run(adsevent->connector);
        if (status == ASYNC_COMPLETE) {
            /* We're connected to the device socket. */
            ads->socket_status = ADS_CONNECTED;
            D("Channel '%s'@%d is connected asynchronously",
              _ads_id_str(ads), _ads_port(ads));
            _on_event_socket_connected(adsevent, 0);
        } else if (status == ASYNC_ERROR) {
            _on_event_socket_connected(adsevent, adsevent->connector->error);
        }
        return;
    }

    /*
     * Device is connected. Continue with the data transfer.
     */

    if ((events & LOOP_IO_READ) != 0) {
        /* Continue reading data. */
        status = asyncLineReader_read(&adsevent->alr);
        if (status == ASYNC_COMPLETE) {
            errno = 0;
            _on_event_received(adsevent);
        } else if (status == ASYNC_ERROR) {
            D("I/O failure while reading from channel '%s'@%d: %s",
              _ads_id_str(ads),  _ads_port(ads), strerror(errno));
            /* There is one special failure here, when buffer was too small to
             * contain the entire string. This is not an I/O, but rather a
             * protocol error. So we don't report it to the I/O failure
             * callback. */
            if (errno == ENOMEM) {
                _on_event_received(adsevent);
            } else {
                if (ads->ad->on_io_failure != NULL) {
                    ads->ad->on_io_failure(ads->ad->opaque, ads->ad, errno);
                }
            }
        }
    }

    if ((events & LOOP_IO_WRITE) != 0) {
        while (adsevent->send_pending != NULL) {
            AsyncSendBuffer* to_send = adsevent->send_pending;
            const int offset = to_send->data_size - to_send->data_remaining;
            const int sent = socket_send(ads->fd, to_send->data + offset,
                                         to_send->data_remaining);
            if (sent < 0) {
                if (errno == EWOULDBLOCK) {
                    /* Try again later. */
                    return;
                } else {
                    /* An error has occured. */
                    _android_event_socket_cancel_send(adsevent, ATR_IO_ERROR);
                    if (ads->ad->on_io_failure != NULL) {
                        ads->ad->on_io_failure(ads->ad->opaque, ads->ad, errno);
                    }
                    return;
                }
            } else if (sent == 0) {
                /* Disconnect condition. */
                _android_event_socket_cancel_send(adsevent, ATR_DISCONNECT);
                if (ads->ad->on_io_failure != NULL) {
                    ads->ad->on_io_failure(ads->ad->opaque, ads->ad, errno);
                }
                return;
            } else if (sent == to_send->data_remaining) {
                /* All data is sent. */
                errno = 0;
                adsevent->send_pending = to_send->next;
                _async_send_buffer_complete(to_send, ATR_SUCCESS);
            } else {
                /* Chunk is sent. */
                to_send->data_remaining -= sent;
                return;
            }
        }
        loopIo_dontWantWrite(adsevent->io);
    }
}

static void
_on_event_socket_connected(AndroidEventSocket* adsevent, int failure)
{
    int res;
    AndroidDevSocket* ads = &adsevent->dev_socket;

    if (failure) {
        _android_event_socket_disconnect(adsevent);
        if (adsevent->on_connected != NULL) {
            adsevent->on_connected(ads->opaque, ads, failure);
        }
        return;
    }

    /* Complete event socket connection by identifying it as "event" socket with
     * the application. */
    ads->socket_status = ADS_CONNECTED;
    res = _android_dev_socket_register(ads);

    if (res) {
        const int save_error = errno;
        _android_event_socket_disconnect(adsevent);
        errno = save_error;
    }

    /* Notify callback about connection completion. */
    if (adsevent->on_connected != NULL) {
        if (res) {
            adsevent->on_connected(ads->opaque, ads, errno);
        } else {
            adsevent->on_connected(ads->opaque, ads, 0);
        }
    }
}

static void
_on_event_received(AndroidEventSocket* adsevent)
{
    if (adsevent->on_event != NULL) {
        AndroidDevice* ad = adsevent->dev_socket.ad;
        adsevent->on_event(ad->opaque, ad, (char*)adsevent->alr.buffer,
                           adsevent->alr.pos);
    }
}

/********************************************************************************
 *                          Android device connection
 *******************************************************************************/

/* Callback that is invoked when event socket is connected and registered as part
 * of the _android_device_connect_async API.
 * Param:
 *  opaque - Opaque pointer associated with AndroidDevice instance.
 *  ads - Common socket descriptor for the event socket.
 *  failure - If zero connection has succeeded, otherwise contains 'errno'-reason
 *      for connection failure.
 */
static void
_on_android_device_connected_async(void* opaque,
                                   AndroidDevSocket* ads,
                                   int failure)
{
    int res;
    AndroidDevice* ad = ads->ad;

    if (failure) {
        /* Depending on the failure code we will either retry, or bail out. */
        switch (failure) {
            case EPIPE:
            case EAGAIN:
            case EINPROGRESS:
            case EALREADY:
            case EHOSTUNREACH:
            case EHOSTDOWN:
            case ECONNREFUSED:
            case ESHUTDOWN:
            case ENOTCONN:
            case ECONNRESET:
            case ECONNABORTED:
            case ENETRESET:
            case ENETUNREACH:
            case ENETDOWN:
            case EBUSY:
#if !defined(_DARWIN_C_SOURCE) && !defined(_WIN32)
            case ERESTART:
            case ECOMM:
            case ENONET:
#endif  /* !_DARWIN_C_SOURCE && !_WIN32 */
                /* Device is not available / reachable at the moment.
                 * Retry connection later. */
                loopTimer_startRelative(ad->timer, ADS_RETRY_CONNECTION_TIMEOUT);
                return;
            default:
                D("Failed to asynchronously connect channel '%s':%d %s",
                  _ads_id_str(ads), _ads_port(ads), strerror(errno));
            if (ad->on_connected != NULL) {
                ad->on_connected(ad->opaque, ad, failure);
            }
            break;
        }
        return;
    }

    /* Event socket is connected. Connect the query socket now. Give it 5
     * seconds to connect. */
    _ads_set_deadline(&ad->query_socket.dev_socket, 5000);
    res = _android_query_socket_connect(&ad->query_socket);
    if (res == 0) {
        /* Query socket is connected. */
        if (ad->on_connected != NULL) {
            ad->on_connected(ad->opaque, ad, 0);
        }
    } else {
        /* If connection completion has failed - disconnect the sockets. */
        _android_event_socket_disconnect(&ad->event_socket);
        _android_query_socket_disconnect(&ad->query_socket);

        if (ad->on_connected != NULL) {
            ad->on_connected(ad->opaque, ad, errno);
        }
    }
}

static void
_on_timer(void* opaque)
{
    /* Retry the connection. */
    AndroidDevice* ad = (AndroidDevice*)opaque;
    android_device_connect_async(ad, ad->on_connected);
}

/* Destroys and frees the descriptor. */
static void
_android_device_free(AndroidDevice* ad)
{
    if (ad != NULL) {
        _android_event_socket_destroy(&ad->event_socket);
        _android_query_socket_destroy(&ad->query_socket);

        /* Delete asynchronous I/O looper. */
        if (ad->looper != NULL ) {
            loopTimer_done(ad->timer);
            looper_free(ad->looper);
        }

        /* Delete synchronous I/O looper. */
        if (ad->io_looper != NULL) {
            iolooper_reset(ad->io_looper);
            iolooper_free(ad->io_looper);
        }

        AFREE(ad);
    }
}

/********************************************************************************
 *                          Android device API
 *******************************************************************************/

AndroidDevice*
android_device_init(void* opaque, int port, io_failure_cb on_io_failure)
{
    int res;
    AndroidDevice* ad;

    ANEW0(ad);

    ad->opaque = opaque;
    ad->on_io_failure = on_io_failure;

    /* Create I/O looper for synchronous I/O on the device. */
    ad->io_looper = iolooper_new();
    if (ad->io_looper == NULL) {
        E("Unable to create synchronous I/O looper for android device.");
        _android_device_free(ad);
        return NULL;
    }

    /* Create a looper for asynchronous I/O on the device. */
    ad->looper = looper_newCore();
    if (ad->looper != NULL) {
        /* Create a timer that will be used for connection retries. */
        loopTimer_init(ad->timer, ad->looper, _on_timer, ad);
    } else {
        E("Unable to create asynchronous I/O looper for android device.");
        _android_device_free(ad);
        return NULL;
    }

    /* Init query socket. */
    res = _android_query_socket_init(&ad->query_socket, opaque, ad, port);
    if (res) {
        _android_device_free(ad);
        return NULL;
    }

    /* Init event socket. */
    res = _android_event_socket_init(&ad->event_socket, opaque, ad, port);
    if (res) {
        _android_device_free(ad);
        return NULL;
    }

    return ad;
}

void
android_device_destroy(AndroidDevice* ad)
{
    if (ad != NULL) {
        _android_device_free(ad);
    }
}

int
android_device_connect_sync(AndroidDevice* ad, int to)
{
    int res;

    /* Setup deadline for the connections. */
    _ads_set_deadline(&ad->query_socket.dev_socket, to);
    ad->event_socket.dev_socket.deadline = ad->query_socket.dev_socket.deadline;

    /* Connect the query socket first. */
    res = _android_query_socket_connect(&ad->query_socket);
    if (!res) {
        /* Connect to the event socket next. */
        res = _android_event_socket_connect_sync(&ad->event_socket);
    }

    return res;
}

int
android_device_connect_async(AndroidDevice* ad, device_connected_cb on_connected)
{
    /* No deadline for async connections. */
    ad->query_socket.dev_socket.deadline = DURATION_INFINITE;
    ad->event_socket.dev_socket.deadline = DURATION_INFINITE;

    /* Connect to the event socket first, and delegate query socket connection
     * into callback invoked when event socket is connected. NOTE: In case of
     * failure 'on_connected' callback has already been called from
     * _on_android_device_connected_async routine. */
    ad->on_connected = on_connected;
    return _android_event_socket_connect_async(&ad->event_socket,
                                               _on_android_device_connected_async);
}

void
android_device_disconnect(AndroidDevice* ad)
{
    _android_event_socket_disconnect(&ad->event_socket);
    _android_query_socket_disconnect(&ad->query_socket);
}

int
android_device_query(AndroidDevice* ad,
                     const char* query,
                     char* buff,
                     size_t buffsize,
                     int to)
{
    int res;

    /* Setup deadline for the query. */
    _ads_set_deadline(&ad->query_socket.dev_socket, to);

    /* Send the query. */
    res = _android_dev_socket_send(&ad->query_socket.dev_socket, query,
                                   strlen(query) + 1);
    if (res > 0) {
        /* Receive the response. */
        res = _android_dev_socket_read_response(&ad->query_socket.dev_socket,
                                                buff, buffsize);
        return (res >= 0) ? 0 : -1;
    }

    return -1;
}

int
android_device_start_query(AndroidDevice* ad, const char* query, int to)
{
    int res;

    /* Setup deadline for the query. */
    _ads_set_deadline(&ad->query_socket.dev_socket, to);

    /* Send the query header. */
    res = _android_dev_socket_send(&ad->query_socket.dev_socket, query,
                                   strlen(query) + 1);
    return (res > 0) ? 0 : -1;
}

int
android_device_send_query_data(AndroidDevice* ad, const void* data, int size)
{
    return _android_dev_socket_send(&ad->query_socket.dev_socket, data, size);
}

int
android_device_complete_query(AndroidDevice* ad, char* buff, size_t buffsize)
{
    /* Receive the response to the query. */
    const int res = _android_dev_socket_read_response(&ad->query_socket.dev_socket,
                                                      buff, buffsize);
    return (res >= 0) ? 0 : -1;
}

int
android_device_listen(AndroidDevice* ad,
                      char* buff,
                      int buffsize,
                      event_cb on_event)
{
    return _android_event_socket_listen(&ad->event_socket, buff, buffsize,
                                        on_event);
}

int
android_device_send_async(AndroidDevice* ad,
                          void* data,
                          int size,
                          int free_on_close,
                          async_send_cb cb,
                          void* opaque)
{
    return _android_event_socket_send(&ad->event_socket, data, size,
                                      free_on_close, cb, opaque);
}

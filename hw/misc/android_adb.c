/*
 * ARM Android emulator adb backend
 *
 * Copyright (c) 2014 Linaro Limited
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Handle connections to the qemud:adb pipe from Android guests and route
 * traffic over this pipe to the host adb server connecting to the qemu
 * process on a tcp socket.
 */

#include <glib.h>
#include <stdio.h>
#include <unistd.h>

#include "qemu-common.h"
#include "qemu/config-file.h"
#include "qemu/main-loop.h"
#include "qapi/error.h"
#include "qemu/module.h"
#include "qemu/sockets.h"

#include "hw/misc/android_pipe.h"

//#define DEBUG_ADB

#ifdef DEBUG_ADB
#define DPRINTF(fmt, ...) \
do { fprintf(stderr, "adb: " fmt , ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) do {} while(0)
#endif

#define PIPE_QUEUE_LEN     16
#define HANDSHAKE_MAXLEN   128
#define ADB_BUFFER_LEN     4096

#define ADB_SERVER_PORT         5037

/* 'accept' request from adbd */
static const char _accept_req[] = "accept";
/* 'start' request from adbd */
static const char _start_req[]  = "start";
/* handshake reply to adbd */
static const char _ok_resp[]    = "ok";
static const char _ko_resp[]    = "ko";


static bool pipe_backend_initialized = false;

enum adb_connect_state {
    ADB_CONNECTION_STATE_UNCONNECTED = 0,
    ADB_CONNECTION_STATE_ACCEPT,
    ADB_CONNECTION_STATE_CONNECTED,
};

typedef struct {
    void*     hwpipe;
    enum adb_connect_state state;
    GIOChannel *chan;   /* I/O channel to adb server */
    unsigned flags;

    /* TODO: Make sure access to thes buffers is
     * synchronized through the io accessor
     * functions in QEMU or some other
     * mechanism. */
    char out_buffer[ADB_BUFFER_LEN];
    char *out_next;
    unsigned out_cnt;

} adb_pipe;

/*
 * This structure keeps track of the adb-server connection state (that
 * is HOST adb-server <-> QEMU connections). We only expect to see one
 * active connection at a time as data is multiplexed over the pipe.
 */

typedef struct {
    GIOChannel *listen_chan;    /* listener/connect socket */
    GIOChannel *chan;           /* actual comms socket */
    /* these cache the read/write state for when wakeon is called */
    gboolean    data_in;        /* have we seen data? */
    gboolean    data_out;       /* can we output data? */
    adb_pipe *adb_pipes[PIPE_QUEUE_LEN];
    adb_pipe *connected_pipe;
} adb_backend_state;

static adb_backend_state adb_state;

static void adb_reply(adb_pipe *apipe, const char *reply);


/****************************************************************************
 * ADB Server Interface
 */

static QemuOpts* adb_server_config(void) {
    QemuOpts *socket_opts;

    socket_opts = qemu_opts_create(&socket_optslist, NULL, 0,
                                   &error_abort);

    if (!qemu_opt_get(socket_opts, "host")) {
        qemu_opt_set(socket_opts, "host", "localhost");
    }

    if (!qemu_opt_get(socket_opts, "port")) {
        qemu_opt_set_number(socket_opts, "port", ADB_SERVER_PORT);
    }

    return socket_opts;
}

static void adb_server_notify(int adb_port) {
    Error *local_err = NULL;
    QemuOpts *socket_opts = adb_server_config();
    int sock = inet_connect_opts(socket_opts, &local_err, NULL, NULL);
    size_t len;
    gchar *message,*handshake;

    if (sock >= 0) {
        socket_set_nodelay(sock);
    }

    /* Failed to establish connection */
    if (sock < 0) {
        fprintf(stderr,"%s: Failed to establish connection to ADB server\n",
                __func__);
        return;
    }

    message = g_strdup_printf("host:emulator:%d", adb_port);
    handshake = g_strdup_printf("%04x%s", (int) strlen(message), message);
    len = strlen(handshake);

    if (send_all(sock, handshake, len) != len) {
        fprintf(stderr,"%s: error sending string:%s\n", __func__, handshake);
    }

    closesocket(sock);

    g_free(message);
    g_free(handshake);
    return;
}

/* TODO: Needs a common implementation with the likes of qemu-char.c */
static GIOChannel *io_channel_from_socket(int fd)
{
    GIOChannel *chan;

    if (fd == -1) {
        return NULL;
    }

#ifdef _WIN32
    chan = g_io_channel_win32_new_socket(fd);
#else
    chan = g_io_channel_unix_new(fd);
#endif

    g_io_channel_set_encoding(chan, NULL, NULL);
    g_io_channel_set_buffered(chan, FALSE);

    return chan;
}
static gboolean tcp_adb_accept(GIOChannel *channel, GIOCondition cond,
                               void *opaque);

/* Close a connection to the server.
**
** We need to ensure we clean-up any connection state and re-enable
** the watch on the listen socket so new connections can be created.
*/
static void tcp_adb_server_close(adb_backend_state *bs)
{
    g_assert(bs->chan);
    g_assert(bs->listen_chan);

    /* clean-up connected pipes */
    if (bs->connected_pipe) {
        DPRINTF("%s: closing connected pipe\n", __func__);
        android_pipe_close(bs->connected_pipe->hwpipe);
        bs->connected_pipe->chan = NULL;
        bs->connected_pipe = NULL;
    }

    /* wait for new connections */
    g_io_add_watch(bs->listen_chan, G_IO_IN, tcp_adb_accept, bs);

    /* finally close down this socket */
    g_io_channel_shutdown(bs->chan, FALSE, NULL);
    g_io_channel_unref(bs->chan);
    bs->chan = NULL;
}

/*
 * This handles state changes on the server socket. We don't directly
 * start receiving or sending data here but we do need to ensure that
 * the pipe guest wakes up so it can start reading data.
 *
 * This is a one-shot wake up - the watch needs re-adding whenever
 * things go quite so we'll wake up again when needed.
 */
static gboolean tcp_adb_server_data(GIOChannel *channel, GIOCondition cond,
                                    void *opaque)
{
    adb_backend_state *bs = (adb_backend_state *) opaque;
    if (cond & G_IO_IN) {
        bs->data_in = TRUE;
        if (bs->connected_pipe && bs->connected_pipe->flags & PIPE_WAKE_READ) {
            DPRINTF("%s: waking up pipe for incomming data\n", __func__);
            android_pipe_wake(bs->connected_pipe->hwpipe, PIPE_WAKE_READ);
        }
    }

    if (cond & G_IO_OUT) {
        bs->data_out = TRUE;
        if (bs->connected_pipe && bs->connected_pipe->flags & PIPE_WAKE_WRITE) {
            DPRINTF("%s: waking up pipe for now able to write\n", __func__);
            android_pipe_wake(bs->connected_pipe->hwpipe, PIPE_WAKE_WRITE);
        }
    }

    if ((cond & G_IO_ERR) ||
        (cond & G_IO_HUP)) {
        DPRINTF("%s: error %d - closing server connectio\n", __func__, cond);
        tcp_adb_server_close(bs);
    }

    /* Done, we must re-add watch next time we are waiting for data */
    return FALSE;
}

static gboolean tcp_adb_connect(adb_backend_state *bs, int fd)
{
    if (bs->chan) {
        DPRINTF("%s: existing connection %p, fail connect!\n",
                __func__, bs->chan);
        return FALSE;
    } else {
        DPRINTF("%s: in-coming connection on %d\n", __func__, fd);

        qemu_set_nonblock(fd);
        bs->chan = io_channel_from_socket(fd);
        g_io_add_watch(bs->chan, G_IO_IN|G_IO_ERR|G_IO_HUP, tcp_adb_server_data, bs);

        /* If we don't have a pipe to use for the tcp backend, then find one in
         * the accept state.  Note, this can happen, for example, if the previous
         * connected pipe was closed for some reason.  Also note that this becomes
         * sort of random which pipe we select, but there doesn't seem to be any
         * clearly defined semantics about the ordering here.  A proper fifo may
         * be a better data structure for this.
         */
        if (!bs->connected_pipe) {
            int i;
            for (i = 0; i < PIPE_QUEUE_LEN; i++) {
                if (bs->adb_pipes[i] &&
                    bs->adb_pipes[i]->state == ADB_CONNECTION_STATE_ACCEPT) {
                    bs->connected_pipe = bs->adb_pipes[i];
                }
            }
        }

        /* Tell the adbd that the adb server has conected and that we're ready to
         * receive the start package */
        if (bs->connected_pipe) {
            DPRINTF("Incoming TCP connection on already accepted pipe, connect\n");
            adb_pipe *apipe = bs->connected_pipe;
            apipe->chan = bs->chan;
            if (apipe->out_next) {
                fprintf(stderr, "Pending reply on non-connected pipe, error\n");
                abort();
            }
            adb_reply(apipe, _ok_resp);
        }

        return TRUE;
    }
}

/* Accept incoming connections. If the connect succeeds and we create
 * a connection we return FALSE to take the listen socket out of the
 * polling loop. We need to re-add the watch if/when the connection
 * dies so another connection can be created.
 */
static gboolean tcp_adb_accept(GIOChannel *channel, GIOCondition cond,
                               void *opaque)
{
    adb_backend_state *bs = opaque;
    struct sockaddr_in saddr;
    struct sockaddr *addr;
    socklen_t len;
    int fd;

    for(;;) {
        len = sizeof(saddr);
        addr = (struct sockaddr *)&saddr;
        fd = qemu_accept(g_io_channel_unix_get_fd(bs->listen_chan), addr, &len);
        if (fd < 0 && errno != EINTR) {
            DPRINTF("%s: failed to accept %d/%d\n", __func__, fd, errno);
            return FALSE;
        } else if (fd >= 0) {
            break;
        }
    }

    if (tcp_adb_connect(bs, fd)) {
        return FALSE;
    } else {
        return TRUE;
    }
}

static bool adb_server_listen_incoming(int port)
{
    adb_backend_state *bs = &adb_state;
    char *host_port;
    Error *err = NULL;
    int fd;

    host_port = g_strdup_printf("127.0.0.1:%d", port);
    fd = inet_listen(host_port, NULL, 0, SOCK_STREAM, 0, &err);
    if (fd < 0) {
        return false;
    }

    bs->listen_chan = io_channel_from_socket(fd);
    g_io_add_watch(bs->listen_chan, G_IO_IN, tcp_adb_accept, bs);
    return true;
}


/****************************************************************************
 * Android Pipe adbd Interface
 */

static void* adb_pipe_init(void *hwpipe,void *opaque, const char *args)
{
    adb_pipe *apipe;
    int i;

    DPRINTF("%s: hwpipe=%p\n", __FUNCTION__, hwpipe);
    apipe = g_malloc0(sizeof(adb_pipe));
    apipe->hwpipe = hwpipe;
    apipe->state = ADB_CONNECTION_STATE_UNCONNECTED;
    apipe->out_next = NULL;
    apipe->out_cnt = 0;
    apipe->flags = 0;

    for (i = 0; i < PIPE_QUEUE_LEN; i++) {
        if (adb_state.adb_pipes[i] == NULL) {
            adb_state.adb_pipes[i] = apipe;
            break;
        }
    }

    if (i == PIPE_QUEUE_LEN) {
        DPRINTF("Could not handle more open adb pipes\n");
        g_free(apipe);
        return NULL;
    }

    return apipe;
}

static void adb_pipe_close(void *opaque )
{
    adb_pipe *apipe = opaque;
    int i;

    DPRINTF("%s: hwpipe=%p\n", __FUNCTION__, apipe->hwpipe);
    if (adb_state.connected_pipe == apipe) {
        tcp_adb_server_close(&adb_state);
        adb_state.connected_pipe = NULL;
    }
    for (i = 0; i < PIPE_QUEUE_LEN; i++) {
        if (adb_state.adb_pipes[i] == apipe) {
            adb_state.adb_pipes[i] = NULL;
            break;
        }
    }

    g_free(apipe);
}

static int pipe_send_data(adb_pipe *apipe, char *data, unsigned len,
                          const AndroidPipeBuffer *buffers, int cnt)
{
    int avail = len;
    int chunk;

    while (cnt > 0 && avail > 0) {
        if (avail < buffers[0].size) {
            chunk = avail;
        } else {
            chunk = buffers[0].size;
        }

        memcpy(data, buffers[0].data, chunk);
        data += chunk;
        avail -= chunk;

        buffers++;
        cnt--;
    }

    return len - avail;
}

static bool match_request(const char *request, int len, const char *match)
{
    return len == strlen(match) && !strncmp(request, match, strlen(match));
}

static const char *handle_request(adb_pipe *apipe, const char *request, int len)
{
    adb_backend_state *bs = &adb_state;

    if (match_request(request, len, _accept_req)) {
        if (apipe->state != ADB_CONNECTION_STATE_UNCONNECTED) {
            return _ko_resp;
        }

        apipe->state = ADB_CONNECTION_STATE_ACCEPT;

        /* If we don't have any connected adb_pipe to the tcp backend yet,
         * elect ourselves.  If the tcp connection is ready as well, go ahead
         * and thell adbd to carry on.
         */
        if (!bs->connected_pipe) {
            bs->connected_pipe = apipe;
            if (bs->chan) {
                g_assert(!bs->connected_pipe->chan);
                bs->connected_pipe->chan = bs->chan;
                DPRINTF("Already have tcp connection, reply 'ok' to 'accept'\n");
                return _ok_resp;
            }
        }

        DPRINTF("no tcp connection, wait for it\n");
        return NULL; /* wait until adb server connects */
    } else if (match_request(request, len, _start_req)) {
        if (apipe->state != ADB_CONNECTION_STATE_ACCEPT) {
            DPRINTF("adbd requested 'start' when we are not ready, error\n");
            android_pipe_close(apipe->hwpipe);
            return NULL;
        }

        if (!bs->chan) {
            DPRINTF("adbd requested 'start' but tcp connection not yet connected, error\n");
            bs->connected_pipe->chan = NULL;
            android_pipe_close(apipe->hwpipe);
            return NULL;
        }

        apipe->state = ADB_CONNECTION_STATE_CONNECTED;
        return NULL; /* start proxying data */
    } else {
        /* unrecognized command */
        return _ko_resp;
    }
}

static void adb_reply(adb_pipe *apipe, const char *reply)
{
    strcpy(apipe->out_buffer, reply);
    apipe->out_cnt = strlen(reply);
    apipe->out_next = &apipe->out_buffer[0];
}

static int adb_pipe_proxy_send(adb_pipe *apipe, const AndroidPipeBuffer *buffers,
                               int cnt)
{
    gsize total_copied = 0;
    adb_backend_state *bs = &adb_state;

    g_assert(apipe->chan);

    DPRINTF("%s: %p/%d\n", __func__, buffers, cnt);
    do {
        GError *error = NULL;
        gchar *bptr = (gchar *) buffers[0].data;
        gsize bsize = buffers[0].size;
        gsize copied = 0;
        GIOStatus status = g_io_channel_write_chars(
            apipe->chan, bptr, bsize, &copied, &error);

        total_copied += copied;

        /* If we have already copied some data lets signal that and
         * let it come back to here.
         */
        if (total_copied > 0 &&
            ((status == G_IO_STATUS_EOF || status == G_IO_STATUS_AGAIN))) {
            bs->data_out = FALSE;
            return total_copied;
        }

        /* Can't write and more data.... */
        if (status == G_IO_STATUS_AGAIN) {
            DPRINTF("%s: out of data, setting up watch\n", __func__);
            g_io_add_watch(apipe->chan, G_IO_IN|G_IO_ERR|G_IO_HUP,
                           tcp_adb_server_data, bs);
            bs->data_out = FALSE;
            return PIPE_ERROR_AGAIN;
        }

        if (status == G_IO_STATUS_EOF) {
            bs->data_out = FALSE;
            tcp_adb_server_close(bs);
            return 0;
        }

        if (status != G_IO_STATUS_NORMAL) {
            DPRINTF("%s: went wrong (%d)\n", __func__, status);
            bs->data_out = FALSE;
            tcp_adb_server_close(bs);
            return PIPE_ERROR_IO;
        }

        if (copied < bsize) {
            g_assert(total_copied > 0);
            bs->data_out = FALSE;
            break;
        }

        buffers++;
    } while (--cnt);

    return total_copied;
}

static int adb_pipe_send(void *opaque, const AndroidPipeBuffer* buffers,
                             int cnt)
{
    adb_pipe *apipe = opaque;
    int  ret = 0;
    char request[HANDSHAKE_MAXLEN + 1];
    const char *reply;

    if (apipe->state != ADB_CONNECTION_STATE_CONNECTED) {
        if (apipe->out_next) {
            /* Existing reply in progress, not ready */
            DPRINTF("%s: unconnected pipe, reply in progress\n", __func__);
            return PIPE_ERROR_AGAIN;
        }
        ret = pipe_send_data(apipe, request, sizeof(request), buffers, cnt);
        reply = handle_request(apipe, request, ret);

        if (reply) {
            adb_reply(apipe, reply);
            android_pipe_wake(adb_state.connected_pipe->hwpipe, PIPE_WAKE_READ);
        }
    } else {
        ret = adb_pipe_proxy_send(apipe, buffers, cnt);
        if (ret == 0) {
            return PIPE_ERROR_AGAIN;
        }
    }

    return ret;
}

static int pipe_recv_data(adb_pipe *apipe, const char *data, unsigned len,
                          AndroidPipeBuffer *buffers, int cnt)
{
    int remain = len;
    int chunk;

    while (remain > 0 && cnt > 0) {
        if (remain > buffers[0].size) {
            chunk = buffers[0].size;
        } else {
            chunk = remain;
        }

        memcpy(buffers[0].data, data, chunk);
        data += chunk;
        remain -= chunk;

        buffers++;
        cnt--;
    }

    return len - remain;
}

static int adb_pipe_proxy_recv(adb_pipe *apipe, AndroidPipeBuffer *buffers,
                               int cnt)
{
    gsize total_copied = 0;
    adb_backend_state *bs = &adb_state;

    g_assert(apipe->chan);
    g_assert(bs->chan == apipe->chan);

    DPRINTF("%s: hwpipe=%p (buffers %p/%d)\n", __func__, apipe->hwpipe, buffers, cnt);
    do {
        GError *error = NULL;
        gchar *bptr = (gchar *) buffers[0].data;
        gsize bsize = buffers[0].size;
        gsize copied = 0;
        GIOStatus status = g_io_channel_read_chars(
            apipe->chan, bptr, bsize, &copied, &error);

        DPRINTF("%s: read %zd bytes into %p[%zd] -> %d\n", __func__,
                copied, bptr, bsize, status);

        total_copied += copied;

        /* If we have already copied some data lets signal that and
         * let it come back to here */
        if (total_copied > 0 &&
            ((status == G_IO_STATUS_EOF || status == G_IO_STATUS_AGAIN))) {
            bs->data_in = FALSE;
            return total_copied;
        }

        /* no data to read.... */
        if (status == G_IO_STATUS_AGAIN) {
            DPRINTF("%s: out of data, setting up watch\n", __func__);
            g_io_add_watch(apipe->chan, G_IO_IN|G_IO_ERR|G_IO_HUP,
                           tcp_adb_server_data, bs);
            bs->data_in = FALSE;
            return PIPE_ERROR_AGAIN;
        }

        if (status == G_IO_STATUS_EOF) {
            bs->data_in = FALSE;
            tcp_adb_server_close(bs);
            return 0;
        }

        if (status != G_IO_STATUS_NORMAL) {
            DPRINTF("%s: went wrong (%d)\n", __func__, status);
            bs->data_in = FALSE;
            tcp_adb_server_close(bs);
            return PIPE_ERROR_IO;
        }

        if (copied < bsize) {
            g_assert(total_copied > 0);
            bs->data_in = FALSE;
            break;
        }
        buffers++;
        cnt--;
    } while (cnt);

    return total_copied;
}

static int adb_pipe_recv(void *opaque, AndroidPipeBuffer *buffers,
                             int cnt)
{
    adb_pipe *apipe = opaque;
    adb_backend_state *bs = &adb_state;
    int ret = 0;

    if (apipe->state == ADB_CONNECTION_STATE_CONNECTED) {
        if (bs->data_in) {
            ret = adb_pipe_proxy_recv(apipe, buffers, cnt);
            return ret;
        } else {
            return PIPE_ERROR_AGAIN;
        }
    }

    assert(cnt > 0);

    if (!apipe->out_next) {
        return PIPE_ERROR_AGAIN;
    }

    assert((apipe->out_next - apipe->out_buffer)
             + apipe->out_cnt < ADB_BUFFER_LEN);

    ret = pipe_recv_data(apipe, apipe->out_next, apipe->out_cnt, buffers, cnt);
    apipe->out_cnt -= ret;
    if (ret == apipe->out_cnt) {
        apipe->out_next = NULL;
    } else {
        apipe->out_next += ret;
    }

    return ret;
}

static unsigned adb_pipe_poll(void *opaque)
{
    adb_pipe *apipe = opaque;
    adb_backend_state *bs = &adb_state;
    unsigned flags = 0;

    if (apipe->state != ADB_CONNECTION_STATE_CONNECTED) {
        if (apipe->out_next) {
            flags |= PIPE_POLL_IN;
        } else {
            flags |= PIPE_POLL_OUT;
        }
    } else {
        if (bs->data_in) {
            flags |= PIPE_POLL_IN;
        }
        /* We can always forward data to the socket as far as we know */
        flags |= PIPE_POLL_OUT;
    }

    return flags;
}

static void adb_pipe_wake_on(void *opaque, int flags)
{
    adb_pipe *apipe = opaque;
    DPRINTF("%s: setting flags 0x%x->0x%x\n", __func__, apipe->flags, flags);
    apipe->flags |= flags;

    if (flags & PIPE_WAKE_READ) {
        android_pipe_wake(apipe->hwpipe, PIPE_WAKE_READ);
    }

    if (flags & PIPE_WAKE_WRITE && adb_state.data_out) {
        android_pipe_wake(apipe->hwpipe, PIPE_WAKE_WRITE);
    }
}

static const AndroidPipeFuncs adb_pipe_funcs = {
    adb_pipe_init,
    adb_pipe_close,
    adb_pipe_send,
    adb_pipe_recv,
    adb_pipe_poll,
    adb_pipe_wake_on,
};

/* Initialize and try to listen on the specified port;
 * return true on success or false if the port was in use.
 * Note that there's no way to undo this, so the board must
 * set up the console port first and the adb port second.
 */
bool adb_server_init(int port)
{
    if (!pipe_backend_initialized) {
        adb_state.chan = NULL;
        adb_state.listen_chan = NULL;
        adb_state.data_in = FALSE;
        adb_state.connected_pipe = NULL;

        android_pipe_add_type("qemud:adb", NULL, &adb_pipe_funcs);
        pipe_backend_initialized = true;
    }

    if (!adb_server_listen_incoming(port)) {
        return false;
    }
    adb_server_notify(port);
    return true;
}

/* Copyright (C) 2010 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include <unistd.h>

#include "sockets.h"
#include "qemu-common.h"
#include "errno.h"
#include "iolooper.h"
#include "android/android.h"
#include "android/utils/debug.h"
#include "android/globals.h"
#include "android/console-client.h"

/* Descriptor for a client, connected to the core via console port. */
struct AndroidConsoleClient {
    /* Socket address of the console. */
    SockAddress console_address;

    // Helper for performing I/O on the console socket.
    IoLooper* iolooper;

    /* Stream name. Can be:
     *  - NULL for the console itself.
     *  - "attach UI" for the attached UI client.
     */
    char* stream_name;

    /* Console socket handle. */
    int fd;
};

static int
android_console_reply_ok(const char* reply, int reply_size)
{
    return reply_size < 3 ? 0 : !memcmp(reply, "OK:", 3);
}

static int
android_console_reply_ko(const char* reply, int reply_size)
{
    return reply_size < 3 ? 0 : !memcmp(reply, "KO:", 3);
}

int
android_console_open(SockAddress* sockaddr)
{
    IoLooper* looper = iolooper_new();
    int connect_status;
    char buf[512];

    int fd = socket_create_inet(SOCKET_STREAM);
    if (fd < 0) {
        return fd;
    }

    socket_set_xreuseaddr(fd);
    socket_set_nonblock(fd);
    connect_status = socket_connect(fd, sockaddr);
    if (connect_status < 0 && errno == EINPROGRESS) {
        // Wait till connection occurs.
        iolooper_add_write(looper, fd);
        connect_status = iolooper_wait(looper, CORE_PORT_TIMEOUT_MS);
        if (connect_status > 0) {
            iolooper_del_write(looper, fd);
        } else {
            connect_status = -1;
        }
    }
    if (connect_status >= 0) {
        iolooper_add_read(looper, fd);
        // Read the handshake message from the core's console.
        if (iolooper_wait(looper, CORE_PORT_TIMEOUT_MS) > 0 &&
            iolooper_is_read(looper, fd)) {
            int read_bytes = read(fd, buf, sizeof(buf));
            // Check for the console handshake.
            if (read_bytes >= 15 && !strncmp(buf, "Android Console", 15)) {
                /* Console replies with two strings written separately.
                 * Lets make sure that we've read both of them. If we did, there
                 * should be two '\n' characters in the buffer. */
                if (read_bytes < sizeof(buf)) {
                    buf[read_bytes] = '\0';
                } else {
                    buf[sizeof(buf) - 1] = '\0';
                }
                if (strchr(buf, '\n') == strrchr(buf, '\n')) {
                    // Only one string has been read. Read the second one!
                    if (iolooper_wait(looper, CORE_PORT_TIMEOUT_MS) > 0 &&
                        iolooper_is_read(looper, fd)) {
                        read(fd, buf, sizeof(buf));
                    }
                }
            } else {
                socket_close(fd);
                fd = -1;
            }
        } else {
            socket_close(fd);
            fd = -1;
        }
    } else {
        socket_close(fd);
        fd = -1;
    }

    iolooper_free(looper);

    return fd;
}

AndroidConsoleClient*
android_console_client_create(SockAddress* console_address)
{
    AndroidConsoleClient* desc = malloc(sizeof(AndroidConsoleClient));
    if (desc == NULL) {
        derror("PANIC: not enough memory\n");
        exit(1);
    }
    desc->iolooper = iolooper_new();
    if (desc->iolooper == NULL) {
        free(desc);
        derror("PANIC: not enough memory\n");
        exit(1);
    }
    desc->console_address = console_address[0];
    desc->stream_name = NULL;
    desc->fd = -1;

    return desc;
}

void
android_console_client_free(AndroidConsoleClient* desc)
{
    if (desc == NULL) {
        return;
    }
    if (desc->iolooper != NULL) {
        iolooper_reset(desc->iolooper);
        iolooper_free(desc->iolooper);
    }
    if (desc->stream_name != NULL) {
        free(desc->stream_name);
    }
    free(desc);
}

int
android_console_client_open(AndroidConsoleClient* desc)
{
    if (desc == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (desc->fd >= 0) {
        return 0;
    }

    desc->fd = android_console_open(&desc->console_address);

    return desc->fd >= 0 ? 0 : -1;
}

void
android_console_client_close(AndroidConsoleClient* desc)
{
    if (desc == NULL) {
        return;
    }
    if (desc->iolooper != NULL) {
        iolooper_reset(desc->iolooper);
    }
    if (desc->fd != -1) {
        socket_close(desc->fd);
        desc->fd = -1;
    }
}

int
android_console_client_write(AndroidConsoleClient* desc,
                             const void* buffer,
                             int to_write,
                             int* written_bytes)
{
    int tmp = write(desc->fd, buffer, to_write);
    if (written_bytes != NULL) {
        *written_bytes = tmp != -1 ? tmp : 0;
    }
    return tmp != -1 ? 0 : -1;
}

int
android_console_client_read(AndroidConsoleClient* desc,
                            void* buffer,
                            int to_read,
                            int* read_bytes)
{
    int tmp;
    iolooper_add_read(desc->iolooper, desc->fd);
    if (iolooper_wait(desc->iolooper, CORE_PORT_TIMEOUT_MS) <= 0 ||
        !iolooper_is_read(desc->iolooper, desc->fd)) {
        iolooper_del_read(desc->iolooper, desc->fd);
        return -1;
    }
    tmp = read(desc->fd, buffer, to_read);
    iolooper_del_read(desc->iolooper, desc->fd);
    if (read_bytes != NULL) {
        *read_bytes = tmp != -1 ? tmp : 0;
    }
    return tmp != -1 ? 0 : -1;
}

int
android_console_client_switch_stream(AndroidConsoleClient* desc,
                                     const char* stream_name,
                                     char** handshake)
{
    char buf[4096];
    int handshake_len;

    *handshake = NULL;
    if (desc == NULL || desc->stream_name != NULL || stream_name == NULL) {
        errno = EINVAL;
        return -1;
    }

    // Prepare and write "switch" command.
    snprintf(buf, sizeof(buf), "qemu %s\r\n", stream_name);
    if (android_console_client_write(desc, buf, strlen(buf), NULL)) {
        return -1;
    }
    // Read result / handshake
    if (android_console_client_read(desc, buf, sizeof(buf), &handshake_len)) {
        return -1;
    }
    // Make sure that buf is zero-terminated string.
    if (handshake_len < sizeof(buf)) {
        buf[handshake_len] = '\0';
    } else {
        buf[sizeof(buf) - 1] = '\0';
    }
    // Replace terminating "\r\n" with 0
    if (handshake_len >= 2 && buf[handshake_len - 2] == '\r') {
        buf[handshake_len - 2] = '\0';
    }
    // Lets see what kind of response we've got here.
    if (android_console_reply_ok(buf, handshake_len)) {
        *handshake = strdup(buf + 3);
        desc->stream_name = strdup(stream_name);
        return 0;
    } else if (android_console_reply_ko(buf, handshake_len)) {
        *handshake = strdup(buf + 3);
        return -1;
    } else {
        // No OK, no KO? Should be an error!
        *handshake = strdup(buf);
        return -1;
    }
}

void
android_console_client_detach(AndroidConsoleClient* desc)
{
    android_console_client_write(desc, "\n", 1, NULL);
}

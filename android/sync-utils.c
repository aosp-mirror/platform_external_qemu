/*
 * Copyright (C) 2010 The Android Open Source Project
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
 * Contains helper routines dealing with syncronous access to a non-blocking
 * sokets.
 */

#include "qemu-common.h"
#include "errno.h"
#include "iolooper.h"
#include "sockets.h"
#include "android/utils/debug.h"
#include "android/sync-utils.h"

#define  D(...)  do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

struct SyncSocket {
    // Helper for performing synchronous I/O on the socket.
    IoLooper* iolooper;

    /* Opened socket handle. */
    int fd;
};

SyncSocket*
syncSocket_connect(int fd, SockAddress* sockaddr, int timeout)
{
    IoLooper* looper = NULL;
    int connect_status;
    SyncSocket* sync_socket;

    socket_set_nonblock(fd);

    connect_status = socket_connect(fd, sockaddr);
    if (connect_status >= 0) {
		// Connected. Create IoLooper for the helper.
	    looper = iolooper_new();
	} else if (errno != EINPROGRESS) {
		return NULL;
	} else {
		// Connection is in progress. Wait till it's finished.
	    looper = iolooper_new();
        iolooper_add_write(looper, fd);
        connect_status = iolooper_wait(looper, timeout);
        if (connect_status > 0) {
            iolooper_del_write(looper, fd);
        } else {
            iolooper_free(looper);
            return NULL;
        }
	}

	sync_socket = malloc(sizeof(SyncSocket));
	if (sync_socket == NULL) {
        derror("PANIC: not enough memory\n");
        exit(1);
	}

	sync_socket->iolooper = looper;
	sync_socket->fd = fd;

	return sync_socket;
}

void
syncSocket_close(SyncSocket* ssocket)
{
	if (ssocket != NULL && ssocket->fd >= 0) {
		if (ssocket->iolooper != NULL) {
			iolooper_reset(ssocket->iolooper);
		}
		socket_close(ssocket->fd);
		ssocket->fd = -1;
	}
}

void
syncSocket_free(SyncSocket* ssocket)
{
	if (ssocket != NULL) {
		syncSocket_close(ssocket);
		if (ssocket->iolooper != NULL) {
			iolooper_free(ssocket->iolooper);
		}
		free(ssocket);
	}
}

int
syncSocket_start_read(SyncSocket* ssocket)
{
	if (ssocket == NULL || ssocket->fd < 0 || ssocket->iolooper == NULL) {
		errno = EINVAL;
		return -1;
	}
	iolooper_add_read(ssocket->iolooper, ssocket->fd);
	return 0;
}

int
syncSocket_stop_read(SyncSocket* ssocket)
{
	if (ssocket == NULL || ssocket->fd < 0 || ssocket->iolooper == NULL) {
		errno = EINVAL;
		return -1;
	}
	iolooper_del_read(ssocket->iolooper, ssocket->fd);
	return 0;
}

int
syncSocket_read_absolute(SyncSocket* ssocket,
						 void* buf,
						 int size,
						 int64_t deadline)
{
	int ret;

	if (ssocket == NULL || ssocket->fd < 0 || ssocket->iolooper == NULL) {
		errno = EINVAL;
		return -1;
	}

	ret = iolooper_wait_absolute(ssocket->iolooper, deadline);
	if (ret > 0) {
		if (!iolooper_is_read(ssocket->iolooper, ssocket->fd)) {
			D("%s: Internal error, iolooper_is_read() not set!", __FUNCTION__);
			return -1;
		}
		ret = read(ssocket->fd, buf, size);
	}
	return ret;
}

int
syncSocket_read(SyncSocket* ssocket, void* buf, int size, int timeout)
{
	return syncSocket_read_absolute(ssocket, buf, size, iolooper_now() + timeout);
}

// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

// These are basically going to be forwarded to androids os independent socket
// utils

#ifdef _MSC_VER
#include "msvc-posix.h"
#endif

#include <stdbool.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Close a socket, preserves errno to ensure it can be used in
// error-handling code paths.
void socketClose(int socket);

// Try to receive up to |bufferLen| bytes from |socket| into |buffer|.
// Return the number of bytes actually read, 0 in case of disconnection,
// or -1/errno in case of error. Note that this loops around EINTR as
// a convenience.
ssize_t socketRecv(int socket, void* buffer, size_t bufferLen);

// Try to send up to |bufferLen| bytes to |socket| from |buffer|.
// Return the number of bytes actually sent, 0 in case of disconnection,
// or -1/errno in case of error. Note that this loops around EINTR as
// a convenience. Also, this will *not* generate a SIGPIPE signal when
// writing to a broken pipe (but errno will be set to EPIPE).
ssize_t socketSend(int socket, const void* buffer, size_t bufferLen);

// Same as socketSend() but loop around transient writes.
// Returns true if all bytes were sent, false otherwise.
bool socketSendAll(int socket, const void* buffer, size_t bufferLen);

// Same as socketRecv() but loop around transient reads.
// Returns ture if all bytes were received, false otherwise.
bool socketRecvAll(int socket, void* buffer, size_t bufferLen);

// Connect to TCP |port| on loopback interface (i.e. 127.0.0.1).
// Return new socket on success, or -1/errno on error.
int socketTcp4LoopbackClient(int port);

// Connecto TCP |port| on IPV6 loopback interface (i.e. ::1).
// Return new socket on success, or -1/errno on error.
int socketTcp6LoopbackClient(int port);

#ifdef __cplusplus
}
#endif

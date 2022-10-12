// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

#include "android_sockets.h"
#include <ctype.h>
#include <stdio.h>
#include "aemu/base/sockets/SocketUtils.h"

/* set to 1 for very verbose debugging */
#define DEBUG 0
#if DEBUG >= 1
#include <stdio.h>
#define DD(fmt, ...) \
    printf("socket: %s:%d| " fmt "\n", __func__, __LINE__, ##__VA_ARGS__)
#define DD_BUF(fd, buf, len)                                    \
    do {                                                        \
        printf("socket: %s:%d| (%d):", __func__, __LINE__, fd); \
        for (int x = 0; x < len; x++) {                         \
            if (isprint((int)(buf)[x]))                         \
                printf("%c", (buf)[x]);                         \
            else                                                \
                printf("[0x%02x]", 0xff & (int)(buf)[x]);       \
        }                                                       \
        printf("\n");                                           \
    } while (0)
#else
#define DD(...) (void)0
#define DD_BUF(...) (void)0
#endif

extern "C" {
void socketClose(int socket) {
    android::base::socketClose(socket);
}

ssize_t socketRecv(int socket, void* buffer, size_t bufferLen) {
    auto res = android::base::socketRecv(socket, buffer, bufferLen);
    DD_BUF(socket, ((char*)buffer), bufferLen);
    return res;
}
ssize_t socketSend(int socket, const void* buffer, size_t bufferLen) {
    auto res = android::base::socketSend(socket, buffer, bufferLen);
    DD_BUF(socket, ((char*)buffer), bufferLen);
    return res;
}

bool socketSendAll(int socket, const void* buffer, size_t bufferLen) {
    return android::base::socketSendAll(socket, buffer, bufferLen);
}

bool socketRecvAll(int socket, void* buffer, size_t bufferLen) {
    return android::base::socketRecvAll(socket, buffer, bufferLen);
}

int socketTcp4LoopbackClient(int port) {
    return android::base::socketTcp4LoopbackClient(port);
}
int socketTcp6LoopbackClient(int port) {
    return android::base::socketTcp6LoopbackClient(port);
}
}

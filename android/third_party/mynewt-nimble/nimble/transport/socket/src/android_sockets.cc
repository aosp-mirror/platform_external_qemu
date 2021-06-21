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
#include "android/base/sockets/SocketUtils.h"

extern "C" {
void socketClose(int socket) {
    android::base::socketClose(socket);
}

ssize_t socketRecv(int socket, void* buffer, size_t bufferLen) {
    return android::base::socketRecv(socket, buffer, bufferLen);
}
ssize_t socketSend(int socket, const void* buffer, size_t bufferLen) {
    return android::base::socketSend(socket, buffer, bufferLen);
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

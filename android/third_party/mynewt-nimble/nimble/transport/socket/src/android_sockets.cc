#include "android_sockets.h"
#include "android/base/sockets/SocketUtils.h"

extern "C" {
void socketClose(int socket) {
    android::base::socketClose(socket);
}

ssize_t socketRecv(int socket, void* buffer, size_t bufferLen) {
    return android::base::socketRecv(socket, buffer, bufferLen);
}
ssize_t socketSend(int socket, const void* buffer, size_t bufferLen)
{
    return android::base::socketSend(socket, buffer, bufferLen);
}

bool socketSendAll(int socket, const void* buffer, size_t bufferLen) {
    return android::base::socketSendAll(socket, buffer, bufferLen);
}

// Same as socketRecv() but loop around transient reads.
// Returns ture if all bytes were received, false otherwise.
bool socketRecvAll(int socket, void* buffer, size_t bufferLen) {
    return android::base::socketRecvAll(socket, buffer, bufferLen);
}


// Connect to TCP |port| on loopback interface (i.e. 127.0.0.1).
// Return new socket on success, or -1/errno on error.
int socketTcp4LoopbackClient(int port) {
    return android::base::socketTcp4LoopbackClient(port);
}

// Connecto TCP |port| on IPV6 loopback interface (i.e. ::1).
// Return new socket on success, or -1/errno on error.
int socketTcp6LoopbackClient(int port) {
    return android::base::socketTcp6LoopbackClient(port);
}

}

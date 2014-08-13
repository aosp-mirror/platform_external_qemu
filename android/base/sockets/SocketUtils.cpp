// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/sockets/SocketUtils.h"

#include "android/base/EintrWrapper.h"
#include "android/base/sockets/ScopedSocket.h"

#ifdef _WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <sys/socket.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#endif

#include <string.h>

namespace android {
namespace base {

namespace {

// Generic union to handle the mess of sockaddr variants without
// compiler warnings about aliasing.
union SockAddressStorage {
    struct sockaddr generic;
    struct sockaddr_in inet;

    void initLoopback(int port) {
        memset(&inet, 0, sizeof(inet));
        inet.sin_family = AF_INET;
        inet.sin_port = htons(port);
        inet.sin_addr.s_addr = htonl(0x7f000001);
    }
};

int socketSetOption(int socket, int  domain, int option, int  _flag) {
#ifdef _WIN32
    DWORD flag = (DWORD) _flag;
#else
    int flag = _flag;
#endif
    return ::setsockopt(socket,
                        domain,
                        option,
                        (const char*)&flag,
                        sizeof(flag) );
}

int socketSetXReuseAddr(int socket) {
#ifdef _WIN32
   /* on Windows, SO_REUSEADDR is used to indicate that several programs can
    * bind to the same port. this is completely different from the Unix
    * semantics. instead of SO_EXCLUSIVEADDR to ensure that explicitely prevent
    * this.
    */
    return socketSetOption(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
#else
    return socketSetOption(socket, SOL_SOCKET, SO_REUSEADDR, 1);
#endif
}

}  // namespace

void socketClose(int socket) {
    int save_errno = errno;
#ifdef _WIN32
    ::closesocket(socket);
#else
    IGNORE_EINTR(::close(socket));
#endif
    errno = save_errno;
}

ssize_t socketRecv(int socket, void* buffer, size_t bufferLen) {
#ifdef _WIN32
    return ::recv(socket, reinterpret_cast<char*>(buffer), bufferLen, 0);
#else
    return HANDLE_EINTR(::recv(socket, buffer, bufferLen, 0));
#endif
}

ssize_t socketSend(int socket, const void* buffer, size_t bufferLen) {
#ifdef _WIN32
    return ::send(socket, reinterpret_cast<const char*>(buffer), bufferLen, 0);
#else
    return HANDLE_EINTR(::send(socket, buffer, bufferLen, 0));
#endif
}

void socketShutdownWrites(int socket) {
#ifdef _WIN32
    ::shutdown(socket, SD_SEND);
#else
    ::shutdown(socket, SHUT_WR);
#endif
}

void socketShutdownReads(int socket) {
#ifdef _WIN32
    ::shutdown(socket, SD_RECEIVE);
#else
    ::shutdown(socket, SHUT_RD);
#endif
}

void socketSetNonBlocking(int socket) {
#ifdef _WIN32
    unsigned long opt = 1;
    ioctlsocket(socket, FIONBIO, &opt);
#else
    int   flags = fcntl(socket, F_GETFL);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
#endif
}

void socketSetBlocking(int socket) {
#ifdef _WIN32
    unsigned long opt = 0;
    ioctlsocket(socket, FIONBIO, &opt);
#else
    int   flags = fcntl(socket, F_GETFL);
    fcntl(socket, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

int socketTcpLoopbackServer(int port) {
    ScopedSocket s(::socket(AF_INET, SOCK_STREAM, 0));

    if (s.get() < 0) {
        return -1;
    }

    socketSetXReuseAddr(s.get());

    SockAddressStorage addr;
    addr.initLoopback(port);

    if (::bind(s.get(), &addr.generic, sizeof(addr.inet)) < 0 ||
        ::listen(s.get(), 4) < 0) {
        return -1;
    }

    return s.release();
}

int socketTcpLoopbackClient(int port) {
    ScopedSocket s(::socket(AF_INET, SOCK_STREAM, 0));

    if (s.get() < 0) {
        return -1;
    }

    SockAddressStorage addr;
    addr.initLoopback(port);

    if (::connect(s.get(), &addr.generic, sizeof(addr.inet)) < 0) {
        return -1;
    }

    return s.release();
}

int socketCreatePair(int* fd1, int* fd2) {
#ifndef _WIN32
    int fds[2];
    int ret = ::socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

    if (!ret) {
        socketSetNonBlocking(fds[0]);
        socketSetNonBlocking(fds[1]);
        *fd1 = fds[0];
        *fd2 = fds[1];
    }
    return ret;
#else /* _WIN32 */
    /* on Windows, select() only works with network sockets, which
     * means we absolutely cannot use Win32 PIPEs to implement
     * socket pairs with the current event loop implementation.
     * We're going to do like Cygwin: create a random pair
     * of localhost TCP sockets and connect them together
     */

    /* first, create the 'server' socket.
     * a port number of 0 means 'any port between 1024 and 5000.
     * see Winsock bind() documentation for details */
    ScopedSocket s0(socketTcpLoopbackServer(0));
    if (s0.get() < 0) {
        return -1;
    }

    socketSetNonBlocking(s0.get());

    /* now connect a client socket to it, we first need to
     * extract the server socket's port number */
    SockAddressStorage addr;
    socklen_t addrLen = sizeof(addr.inet);
    if (getsockname(s0.get(), &addr.generic, &addrLen) < 0) {
        return -1;
    }

    int port = ntohs(addr.inet.sin_port);
    ScopedSocket s2(socketTcpLoopbackClient(port));
    if (!s2.valid()) {
        return -1;
    }

    /* we need to accept the connection on the server socket
     * this will create the second socket for the pair
     */
    ScopedSocket s1(::accept(s0.get(), NULL, NULL));
    if (!s1.valid()) {
        return -1;
    }
    socketSetNonBlocking(s1.get());

    *fd1 = s1.release();
    *fd2 = s2.release();

    return 0;
#endif /* _WIN32 */
}

}  // namespace base
}  // namespace android

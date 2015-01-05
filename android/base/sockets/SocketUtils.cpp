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
#include "android/base/sockets/SocketErrors.h"

#ifdef _WIN32
#include "android/base/sockets/Winsock.h"
#else
#  include <sys/socket.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <netinet/tcp.h>
#endif

#include <stdlib.h>
#include <string.h>

namespace android {
namespace base {

namespace {

#ifdef _WIN32

#  ifndef ESHUTDOWN
#    define ESHUTDOWN     10058
#  endif

#  ifndef ETOOMANYREFS
#    define ETOOMANYREFS  10059
#  endif

#  ifndef EHOSTDOWN
#    define EHOSTDOWN     10064
#  endif

// This macro is used to implement a mapping of Winsock error codes
// to equivalent errno ones. The point here is to ensure that our socket
// helper functions always update errno to an appropriate value, even on
// Windows, where Winsock normally leaves this variable untouched in case
// of error (client code needs to call WSAGetLastError() instead).
#  define  WINSOCK_ERRORS_LIST \
    EE(WSA_INVALID_HANDLE,EINVAL,"invalid handle") \
    EE(WSA_NOT_ENOUGH_MEMORY,ENOMEM,"not enough memory") \
    EE(WSA_INVALID_PARAMETER,EINVAL,"invalid parameter") \
    EE(WSAEINTR,EINTR,"interrupted function call") \
    EE(WSAEALREADY,EALREADY,"operation already in progress") \
    EE(WSAEBADF,EBADF,"bad file descriptor") \
    EE(WSAEACCES,EACCES,"permission denied") \
    EE(WSAEFAULT,EFAULT,"bad address") \
    EE(WSAEINVAL,EINVAL,"invalid argument") \
    EE(WSAEMFILE,EMFILE,"too many opened files") \
    EE(WSAEWOULDBLOCK,EWOULDBLOCK,"resource temporarily unavailable") \
    EE(WSAEINPROGRESS,EINPROGRESS,"operation now in progress") \
    EE(WSAEALREADY,EAGAIN,"operation already in progress") \
    EE(WSAENOTSOCK,EBADF,"socket operation not on socket") \
    EE(WSAEDESTADDRREQ,EDESTADDRREQ,"destination address required") \
    EE(WSAEMSGSIZE,EMSGSIZE,"message too long") \
    EE(WSAEPROTOTYPE,EPROTOTYPE,"wrong protocol type for socket") \
    EE(WSAENOPROTOOPT,ENOPROTOOPT,"bad protocol option") \
    EE(WSAEADDRINUSE,EADDRINUSE,"address already in use") \
    EE(WSAEADDRNOTAVAIL,EADDRNOTAVAIL,"cannot assign requested address") \
    EE(WSAENETDOWN,ENETDOWN,"network is down") \
    EE(WSAENETUNREACH,ENETUNREACH,"network unreachable") \
    EE(WSAENETRESET,ENETRESET,"network dropped connection on reset") \
    EE(WSAECONNABORTED,ECONNABORTED,"software caused connection abort") \
    EE(WSAECONNRESET,ECONNRESET,"connection reset by peer") \
    EE(WSAENOBUFS,ENOBUFS,"no buffer space available") \
    EE(WSAEISCONN,EISCONN,"socket is already connected") \
    EE(WSAENOTCONN,ENOTCONN,"socket is not connected") \
    EE(WSAESHUTDOWN,ESHUTDOWN,"cannot send after socket shutdown") \
    EE(WSAETOOMANYREFS,ETOOMANYREFS,"too many references") \
    EE(WSAETIMEDOUT,ETIMEDOUT,"connection timed out") \
    EE(WSAECONNREFUSED,ECONNREFUSED,"connection refused") \
    EE(WSAELOOP,ELOOP,"cannot translate name") \
    EE(WSAENAMETOOLONG,ENAMETOOLONG,"name too long") \
    EE(WSAEHOSTDOWN,EHOSTDOWN,"host is down") \
    EE(WSAEHOSTUNREACH,EHOSTUNREACH,"no route to host") \

struct WinsockError {
    int winsock_error;
    int unix_error;
};

const WinsockError kWinsockErrors[] = {
#define  EE(w,u,s)   { w, u, },
    WINSOCK_ERRORS_LIST
#undef   EE
};

const size_t kWinsockErrorsSize =
        sizeof(kWinsockErrors) / sizeof(kWinsockErrors[0]);

// Read the latest winsock error code and updates errno appropriately.
// Also always returns -1 so it can be chained in function calls, as in:
//
//     return fixErrno();
//
int fixErrno() {
    int unix_error = EINVAL;  /* generic error code */
    int winsock_error = WSAGetLastError();

    for (size_t n = 0; n < kWinsockErrorsSize; ++n) {
        if (kWinsockErrors[n].winsock_error == winsock_error) {
            unix_error = kWinsockErrors[n].unix_error;
            break;
        }
    }

    errno = unix_error;
    return -1;
}

// Called on process exit to cleanup Winsock data.
void winsockCleanup() {
    WSACleanup();
}

// Initialize Winsock, must be called early, or socket creation will fail.
int winsockInit() {
    WSADATA Data;
    int ret = WSAStartup(MAKEWORD(2,2), &Data);
    if (ret != 0) {
        (void) WSAGetLastError();
        return -1;
    }
    ::atexit(winsockCleanup);
    return 0;
}

void socketInitWinsock() {
    enum {
        UNINITIALIZED = 0,
        INITIALIZING = 1,
        COMPLETED = 2,
    };
    // Ensure thread-safe lazy initialization.
    //
    // TODO(digit): Create android::base::Once instead to make this a
    // little more portable / testable.
    static LONG volatile sWinsockInit = 0;

    LONG status = InterlockedCompareExchange(
            &sWinsockInit, INITIALIZING, UNINITIALIZED);
    if (status == COMPLETED) {
        // Winsock already initialized.
        return;
    }
    if (status == UNINITIALIZED) {
        // First thread to call this function.
        winsockInit();
        InterlockedExchange(&sWinsockInit, COMPLETED);
        return;
    }
    while (status == INITIALIZING) {
        ::Sleep(0);
        status = InterlockedCompareExchange(
                &sWinsockInit, INITIALIZING, INITIALIZING);
    }
}

#endif // !_WIN32

// Use ON_SOCKET_ERROR_RETURN_M1 to return immediately with value -1
// and errno set to an appropriate value, when |ret| corresponds to
// a socket error code.
//
// On Windows, if |ret| is SOCKET_ERROR, this calls fixErrno() to
// set errno to an appropriate value and return -1.
//
// On Posix, if |ret| is negative, this simply returns -1.
#ifdef _WIN32
#  define ON_SOCKET_ERROR_RETURN_M1(ret) \
        do { \
            if (ret == SOCKET_ERROR) { \
                return fixErrno(); \
            } \
        } while (0)
#else  // !_WIN32
#  define ON_SOCKET_ERROR_RETURN_M1(ret) \
        do { \
            if ((ret) < 0) { \
                return -1; \
            } \
        } while (0);
#endif  // !_WIN32

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

    int getPort() {
        if (generic.sa_family == AF_INET) {
            return ntohs(inet.sin_port);
        } else {
            return -1;
        }
    }
};

int socketSetOption(int socket, int  domain, int option, int  _flag) {
#ifdef _WIN32
    DWORD flag = (DWORD) _flag;
#else
    int flag = _flag;
#endif
    int ret = ::setsockopt(socket,
                           domain,
                           option,
                           (const char*)&flag,
                           sizeof(flag));
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return 0;
}

int socketSetXReuseAddr(int socket) {
#ifdef _WIN32
   /* on Windows, SO_REUSEADDR is used to indicate that several programs can
    * bind to the same port. this is completely different from the Unix
    * semantics. Use instead SO_EXCLUSIVEADDR to explicitely prevent this.
    */
    return socketSetOption(socket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, 1);
#else
    return socketSetOption(socket, SOL_SOCKET, SO_REUSEADDR, 1);
#endif
}

#ifdef _WIN32
int socketTcpConnect(int socket, const SockAddressStorage* addr) {
    int ret = ::connect(socket, &addr->generic,
                        static_cast<socklen_t>(sizeof(addr->inet)));
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return ret;
}
#endif  // _WIN32

int socketTcpBindAndListen(int socket, const SockAddressStorage* addr) {
    socklen_t kSize = static_cast<socklen_t>(sizeof(addr->inet));
    int kBacklog = 5;

    int ret = ::bind(socket, &addr->generic, kSize);
    ON_SOCKET_ERROR_RETURN_M1(ret);

    ret = ::listen(socket, kBacklog);
    ON_SOCKET_ERROR_RETURN_M1(ret);

    return 0;
}

#ifdef _WIN32
int socketAccept(int socket) {
    int ret = ::accept(socket, NULL, NULL);
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return ret;
}
#endif  // _WIN32

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
    ssize_t ret = ::recv(socket, reinterpret_cast<char*>(buffer), bufferLen, 0);
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return ret;
}

ssize_t socketSend(int socket, const void* buffer, size_t bufferLen) {
    ssize_t ret = ::send(socket,
                         reinterpret_cast<const char*>(buffer),
                         bufferLen,
                         0);
    ON_SOCKET_ERROR_RETURN_M1(ret);
    return ret;
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
    ScopedSocket s(socketCreateTcp());

    if (s.get() < 0) {
        DPLOG(ERROR) << "Could not create TCP socket\n";
        return -1;
    }

    socketSetXReuseAddr(s.get());

    SockAddressStorage addr;
    addr.initLoopback(port);

    if (socketTcpBindAndListen(s.get(), &addr) < 0) {
        DPLOG(ERROR) << "Could not bind to TCP loopback port "
                     << port
                     << "\n";
        return -1;
    }

    return s.release();
}

int socketTcpLoopbackClient(int port) {
    ScopedSocket s(socketCreateTcp());

    if (s.get() < 0) {
        DPLOG(ERROR) << "Could not create TCP socket\n";
        return -1;
    }

    SockAddressStorage addr;
    addr.initLoopback(port);

    if (::connect(s.get(), &addr.generic, sizeof(addr.inet)) < 0) {
        DPLOG(ERROR) << "Could not connect to TCP loopback port "
                     << port
                     << "\n";
        return -1;
    }

    return s.release();
}

int socketAcceptAny(int serverSocket) {
    int s = HANDLE_EINTR(::accept(serverSocket, NULL, NULL));
    ON_SOCKET_ERROR_RETURN_M1(s);
    return s;
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
    } else {
        DPLOG(ERROR) << "Could not create socket pair\n";
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

    // IMPORTANT: Keep the s0 socket in blocking mode, or the accept()
    // below may fail with WSAEWOULDBLOCK randomly.

    /* now connect a client socket to it, we first need to
     * extract the server socket's port number */
    int port = socketGetPort(s0.get());

    ScopedSocket s2(socketTcpLoopbackClient(port));
    if (!s2.valid()) {
        return -1;
    }

    /* we need to accept the connection on the server socket
     * this will create the second socket for the pair
     */
    ScopedSocket s1(socketAccept(s0.get()));
    if (!s1.valid()) {
        DPLOG(ERROR) << "Could not accept connection from server socket\n";
        return -1;
    }
    socketSetNonBlocking(s1.get());

    *fd1 = s1.release();
    *fd2 = s2.release();

    return 0;
#endif /* _WIN32 */
}

int socketCreateTcp() {
#ifdef _WIN32
    socketInitWinsock();
#endif
    int s = ::socket(AF_INET, SOCK_STREAM, 0);
    ON_SOCKET_ERROR_RETURN_M1(s);
    return s;
}

int socketGetPort(int socket) {
    SockAddressStorage addr;
    socklen_t addrLen = sizeof(addr);
    if (getsockname(socket, &addr.generic, &addrLen) < 0) {
        DPLOG(ERROR) << "Could not get socket name!\n";
        return -1;
    }
    return addr.getPort();
}

}  // namespace base
}  // namespace android

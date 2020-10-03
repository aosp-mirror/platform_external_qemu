/*
 * os-win32.c
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 * Copyright (c) 2010-2016 Red Hat, Inc.
 *
 * QEMU library functions for win32 which are shared between QEMU and
 * the QEMU tools.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * The implementation of g_poll (functions poll_rest, g_poll) at the end of
 * this file are based on code from GNOME glib-2 and use a different license,
 * see the license comment there.
 */

#include "qemu/osdep.h"
#include <windows.h>
#include "qapi/error.h"
#include "sysemu/sysemu.h"
#include "qemu/main-loop.h"
#include "trace.h"
#include "qemu/sockets.h"
#include "qemu/cutils.h"

/* this must come after including "trace.h" */
#include <shlobj.h>
#include <psapi.h>

void *qemu_oom_check(void *ptr)
{
    if (ptr == NULL) {
        fprintf(stderr, "Failed to allocate memory: %lu\n", GetLastError());
        abort();
    }
    return ptr;
}

void *qemu_try_memalign(size_t alignment, size_t size)
{
    void *ptr;

    if (!size) {
        abort();
    }
    ptr = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
    trace_qemu_memalign(alignment, size, ptr);
    return ptr;
}

void *qemu_memalign(size_t alignment, size_t size)
{
    return qemu_oom_check(qemu_try_memalign(alignment, size));
}

bool insufficientMemMessage = false;

void *qemu_anon_ram_alloc(size_t size, uint64_t *align, bool shared)
{
    void *ptr;

    /* FIXME: this is not exactly optimal solution since VirtualAlloc
       has 64Kb granularity, but at least it guarantees us that the
       memory is page aligned. */
    ptr = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

    // On failure, check the commit size.
    if (!ptr) {
        bool notEnoughCommit = false;
        bool shouldTryAgain = false;
        SIZE_T commitTotalBytes, commitLimitBytes;
        const double oneMbBytes = 1048576.0;
        const double safetyFactor = 1.2;
        double totalMb, limitMb, neededMb, remainingMb;

        PERFORMANCE_INFORMATION pi = { sizeof(pi) };

        if (!GetPerformanceInfo(&pi, sizeof(pi))) {
            fprintf(stderr, "ERROR: GetPerformanceInfo() failed.\n");
            return ptr;
        }

        commitTotalBytes = pi.PageSize * pi.CommitTotal;
        commitLimitBytes = pi.PageSize * pi.CommitLimit;

        totalMb = commitTotalBytes / oneMbBytes;
        limitMb = commitLimitBytes / oneMbBytes;
        neededMb = size / oneMbBytes;
        remainingMb = limitMb - totalMb;

        if (neededMb * 1.2 > limitMb - totalMb) {
            fprintf(stderr,
                "ERROR: Insufficient RAM free for launching emulator.\n"
                "Please free up memory (close other programs and files),\n"
                "or decrease the AVD RAM size and try again.\n"
                "Stats:\n"
                "    Currently committed: %f MB\n"
                "    Commit limit: %f MB\n"
                "    Remaining: %f MB\n"
                "    Needed: %f MB\n"
                "For more info, see the Emulator Troubleshooting website:\n"
                "https://developer.android.com/studio/run/emulator-troubleshooting#windows_free_ram_and_commit_charge\n"
                "under \"Windows: Free RAM and commit charge\".",
                totalMb,
                limitMb,
                remainingMb,
                neededMb);
            insufficientMemMessage = true;
            return ptr;
        } else {
            // Maybe things cleared up. Try again.
            ptr = VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);
            trace_qemu_anon_ram_alloc(size, ptr);
            return ptr;
        }
    }

    trace_qemu_anon_ram_alloc(size, ptr);
    return ptr;
}

void qemu_vfree(void *ptr)
{
    trace_qemu_vfree(ptr);
    if (ptr) {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }
}

void qemu_anon_ram_free(void *ptr, size_t size)
{
    trace_qemu_anon_ram_free(ptr, size);
    if (ptr) {
        VirtualFree(ptr, 0, MEM_RELEASE);
    }
}

#ifndef CONFIG_LOCALTIME_R
/* FIXME: add proper locking */
struct tm *gmtime_r(const time_t *timep, struct tm *result)
{
    struct tm *p = gmtime(timep);
    memset(result, 0, sizeof(*result));
    if (p) {
        *result = *p;
        p = result;
    }
    return p;
}

/* FIXME: add proper locking */
struct tm *localtime_r(const time_t *timep, struct tm *result)
{
    struct tm *p = localtime(timep);
    memset(result, 0, sizeof(*result));
    if (p) {
        *result = *p;
        p = result;
    }
    return p;
}
#endif /* CONFIG_LOCALTIME_R */

void qemu_set_block(int fd)
{
    unsigned long opt = 0;
    WSAEventSelect(fd, NULL, 0);
    ioctlsocket(fd, FIONBIO, &opt);
}

void qemu_set_nonblock(int fd)
{
    unsigned long opt = 1;
    ioctlsocket(fd, FIONBIO, &opt);
    qemu_fd_register(fd);
}

int socket_set_fast_reuse(int fd)
{
    /* Enabling the reuse of an endpoint that was used by a socket still in
     * TIME_WAIT state is usually performed by setting SO_REUSEADDR. On Windows
     * fast reuse is the default and SO_REUSEADDR does strange things. So we
     * don't have to do anything here. More info can be found at:
     * http://msdn.microsoft.com/en-us/library/windows/desktop/ms740621.aspx */
    return 0;
}


static int socket_error(void)
{
    switch (WSAGetLastError()) {
    case 0:
        return 0;
    case WSAEINTR:
        return EINTR;
    case WSAEINVAL:
        return EINVAL;
    case WSA_INVALID_HANDLE:
        return EBADF;
    case WSA_NOT_ENOUGH_MEMORY:
        return ENOMEM;
    case WSA_INVALID_PARAMETER:
        return EINVAL;
    case WSAENAMETOOLONG:
        return ENAMETOOLONG;
    case WSAENOTEMPTY:
        return ENOTEMPTY;
    case WSAEWOULDBLOCK:
         /* not using EWOULDBLOCK as we don't want code to have
          * to check both EWOULDBLOCK and EAGAIN */
        return EAGAIN;
    case WSAEINPROGRESS:
        return EINPROGRESS;
    case WSAEALREADY:
        return EALREADY;
    case WSAENOTSOCK:
        return ENOTSOCK;
    case WSAEDESTADDRREQ:
        return EDESTADDRREQ;
    case WSAEMSGSIZE:
        return EMSGSIZE;
    case WSAEPROTOTYPE:
        return EPROTOTYPE;
    case WSAENOPROTOOPT:
        return ENOPROTOOPT;
    case WSAEPROTONOSUPPORT:
        return EPROTONOSUPPORT;
    case WSAEOPNOTSUPP:
        return EOPNOTSUPP;
    case WSAEAFNOSUPPORT:
        return EAFNOSUPPORT;
    case WSAEADDRINUSE:
        return EADDRINUSE;
    case WSAEADDRNOTAVAIL:
        return EADDRNOTAVAIL;
    case WSAENETDOWN:
        return ENETDOWN;
    case WSAENETUNREACH:
        return ENETUNREACH;
    case WSAENETRESET:
        return ENETRESET;
    case WSAECONNABORTED:
        return ECONNABORTED;
    case WSAECONNRESET:
        return ECONNRESET;
    case WSAENOBUFS:
        return ENOBUFS;
    case WSAEISCONN:
        return EISCONN;
    case WSAENOTCONN:
        return ENOTCONN;
    case WSAETIMEDOUT:
        return ETIMEDOUT;
    case WSAECONNREFUSED:
        return ECONNREFUSED;
    case WSAELOOP:
        return ELOOP;
    case WSAEHOSTUNREACH:
        return EHOSTUNREACH;
    default:
        return EIO;
    }
}

int inet_aton(const char *cp, struct in_addr *ia)
{
    uint32_t addr = inet_addr(cp);
    if (addr == 0xffffffff) {
        return 0;
    }
    ia->s_addr = addr;
    return 1;
}

void qemu_set_cloexec(int fd)
{
}

/* Offset between 1/1/1601 and 1/1/1970 in 100 nanosec units */
#define _W32_FT_OFFSET (116444736000000000ULL)

#ifdef _MSC_VER
int qemu_gettimeofday(struct timeval* tp)
#else
int qemu_gettimeofday(qemu_timeval* tp)
#endif
{
    union {
        unsigned long long ns100; /*time since 1 Jan 1601 in 100ns units */
        FILETIME ft;
    } _now;

    if (tp) {
        GetSystemTimeAsFileTime(&_now.ft);
        tp->tv_usec = (long)((_now.ns100 / 10ULL) % 1000000ULL);
        tp->tv_sec = (long)((_now.ns100 - _W32_FT_OFFSET) / 10000000ULL);
    }
    /* Always return 0 as per Open Group Base Specifications Issue 6.
       Do not set errno on error.  */
    return 0;
}

int qemu_get_thread_id(void)
{
    return GetCurrentThreadId();
}

char *
qemu_get_local_state_pathname(const char *relative_pathname)
{
    HRESULT result;
    char base_path[MAX_PATH+1] = "";

    result = SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL,
                             /* SHGFP_TYPE_CURRENT */ 0, base_path);
    if (result != S_OK) {
        /* misconfigured environment */
        g_critical("CSIDL_COMMON_APPDATA unavailable: %ld", (long)result);
        abort();
    }
    return g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", base_path,
                           relative_pathname);
}

void qemu_set_tty_echo(int fd, bool echo)
{
    HANDLE handle = (HANDLE)_get_osfhandle(fd);
    DWORD dwMode = 0;

    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }

    GetConsoleMode(handle, &dwMode);

    if (echo) {
        SetConsoleMode(handle, dwMode | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT);
    } else {
        SetConsoleMode(handle,
                       dwMode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
    }
}

static char exec_dir[PATH_MAX];

void qemu_init_exec_dir(const char *argv0)
{

    char *p;
    char buf[MAX_PATH];
    DWORD len;

    len = win32GetModuleFileName(NULL, buf, sizeof(buf) - 1);
    if (len == 0) {
        return;
    }

    buf[len] = 0;
    p = buf + len - 1;
    while (p != buf && *p != '\\') {
        p--;
    }
    *p = 0;
    if (access(buf, R_OK) == 0) {
        pstrcpy(exec_dir, sizeof(exec_dir), buf);
    }
}

char *qemu_get_exec_dir(void)
{
    return g_strdup(exec_dir);
}

#if !GLIB_CHECK_VERSION(2, 50, 0)
/*
 * The original implementation of g_poll from glib has a problem on Windows
 * when using timeouts < 10 ms.
 *
 * Whenever g_poll is called with timeout < 10 ms, it does a quick poll instead
 * of wait. This causes significant performance degradation of QEMU.
 *
 * The following code is a copy of the original code from glib/gpoll.c
 * (glib commit 20f4d1820b8d4d0fc4447188e33efffd6d4a88d8 from 2014-02-19).
 * Some debug code was removed and the code was reformatted.
 * All other code modifications are marked with 'QEMU'.
 */

/*
 * gpoll.c: poll(2) abstraction
 * Copyright 1998 Owen Taylor
 * Copyright 2008 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

static int poll_rest(gboolean poll_msgs, HANDLE *handles, gint nhandles,
                     GPollFD *fds, guint nfds, gint timeout)
{
    DWORD ready;
    GPollFD *f;
    int recursed_result;

    if (poll_msgs) {
        /* Wait for either messages or handles
         * -> Use MsgWaitForMultipleObjectsEx
         */
        ready = MsgWaitForMultipleObjectsEx(nhandles, handles, timeout,
                                            QS_ALLINPUT, MWMO_ALERTABLE);

        if (ready == WAIT_FAILED) {
            gchar *emsg = g_win32_error_message(GetLastError());
            g_warning("MsgWaitForMultipleObjectsEx failed: %s", emsg);
            g_free(emsg);
        }
    } else if (nhandles == 0) {
        /* No handles to wait for, just the timeout */
        if (timeout == INFINITE) {
            ready = WAIT_FAILED;
        } else {
            SleepEx(timeout, TRUE);
            ready = WAIT_TIMEOUT;
        }
    } else {
        /* Wait for just handles
         * -> Use WaitForMultipleObjectsEx
         */
        ready =
            WaitForMultipleObjectsEx(nhandles, handles, FALSE, timeout, TRUE);
        if (ready == WAIT_FAILED) {
            gchar *emsg = g_win32_error_message(GetLastError());
            g_warning("WaitForMultipleObjectsEx failed: %s", emsg);
            g_free(emsg);
        }
    }

    if (ready == WAIT_FAILED) {
        return -1;
    } else if (ready == WAIT_TIMEOUT || ready == WAIT_IO_COMPLETION) {
        return 0;
    } else if (poll_msgs && ready == WAIT_OBJECT_0 + nhandles) {
        for (f = fds; f < &fds[nfds]; ++f) {
            if (f->fd == G_WIN32_MSG_HANDLE && f->events & G_IO_IN) {
                f->revents |= G_IO_IN;
            }
        }

        /* If we have a timeout, or no handles to poll, be satisfied
         * with just noticing we have messages waiting.
         */
        if (timeout != 0 || nhandles == 0) {
            return 1;
        }

        /* If no timeout and handles to poll, recurse to poll them,
         * too.
         */
        recursed_result = poll_rest(FALSE, handles, nhandles, fds, nfds, 0);
        return (recursed_result == -1) ? -1 : 1 + recursed_result;
    } else if (/* QEMU: removed the following unneeded statement which causes
                * a compiler warning: ready >= WAIT_OBJECT_0 && */
               ready < WAIT_OBJECT_0 + nhandles) {
        for (f = fds; f < &fds[nfds]; ++f) {
            if ((HANDLE) f->fd == handles[ready - WAIT_OBJECT_0]) {
                f->revents = f->events;
            }
        }

        /* If no timeout and polling several handles, recurse to poll
         * the rest of them.
         */
        if (timeout == 0 && nhandles > 1) {
            /* Remove the handle that fired */
            int i;
            for (i = ready - WAIT_OBJECT_0 + 1; i < nhandles; i++) {
                handles[i-1] = handles[i];
            }
            nhandles--;
            recursed_result = poll_rest(FALSE, handles, nhandles, fds, nfds, 0);
            return (recursed_result == -1) ? -1 : 1 + recursed_result;
        }
        return 1;
    }

    return 0;
}

extern int win32_wait_for_objects(GPollFD *fds, guint nfds, gint timeout);

gint g_poll(GPollFD *fds, guint nfds, gint timeout)
{
    HANDLE handles[MAXIMUM_WAIT_OBJECTS];
    gboolean poll_msgs = FALSE;
    GPollFD *f;
    gint nhandles = 0;
    int retval;

    for (f = fds; f < &fds[nfds]; ++f) {
        if (f->fd == G_WIN32_MSG_HANDLE && (f->events & G_IO_IN)) {
            poll_msgs = TRUE;
        } else if (f->fd > 0) {
            /* Don't add the same handle several times into the array, as
             * docs say that is not allowed, even if it actually does seem
             * to work.
             */
            gint i;

            for (i = 0; i < nhandles; i++) {
                if (handles[i] == (HANDLE) f->fd) {
                    break;
                }
            }

            if (i == nhandles) {
                if (nhandles == MAXIMUM_WAIT_OBJECTS) {
                    return win32_wait_for_objects(fds, nfds, timeout);
                } else {
                    handles[nhandles++] = (HANDLE) f->fd;
                }
            }
        }
    }

    for (f = fds; f < &fds[nfds]; ++f) {
        f->revents = 0;
    }

    if (timeout == -1) {
        timeout = INFINITE;
    }

    /* Polling for several things? */
    if (nhandles > 1 || (nhandles > 0 && poll_msgs)) {
        /* First check if one or several of them are immediately
         * available
         */
        retval = poll_rest(poll_msgs, handles, nhandles, fds, nfds, 0);

        /* If not, and we have a significant timeout, poll again with
         * timeout then. Note that this will return indication for only
         * one event, or only for messages. We ignore timeouts less than
         * ten milliseconds as they are mostly pointless on Windows, the
         * MsgWaitForMultipleObjectsEx() call will timeout right away
         * anyway.
         *
         * Modification for QEMU: replaced timeout >= 10 by timeout > 0.
         */
        if (retval == 0 && (timeout == INFINITE || timeout > 0)) {
            retval = poll_rest(poll_msgs, handles, nhandles,
                               fds, nfds, timeout);
        }
    } else {
        /* Just polling for one thing, so no need to check first if
         * available immediately
         */
        retval = poll_rest(poll_msgs, handles, nhandles, fds, nfds, timeout);
    }

    if (retval == -1) {
        for (f = fds; f < &fds[nfds]; ++f) {
            f->revents = 0;
        }
    }

    return retval;
}
#endif

int getpagesize(void)
{
    SYSTEM_INFO system_info;

    GetSystemInfo(&system_info);
    return system_info.dwPageSize;
}

void os_mem_prealloc(int fd, char *area, size_t memory, int smp_cpus,
                     Error **errp)
{
    int i;
    size_t pagesize = getpagesize();

    memory = (memory + pagesize - 1) & -pagesize;
    for (i = 0; i < memory / pagesize; i++) {
        memset(area + pagesize * i, 0, 1);
    }
}


char *qemu_get_pid_name(pid_t pid)
{
    /* XXX Implement me */
    abort();
}


pid_t qemu_fork(Error **errp)
{
    errno = ENOSYS;
    error_setg_errno(errp, errno,
                     "cannot fork child process");
    return -1;
}


#undef connect
int qemu_connect_wrap(int sockfd, const struct sockaddr *addr,
                      socklen_t addrlen)
{
    int ret;
    ret = connect(sockfd, addr, addrlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef listen
int qemu_listen_wrap(int sockfd, int backlog)
{
    int ret;
    ret = listen(sockfd, backlog);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef bind
int qemu_bind_wrap(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen)
{
    int ret;
    ret = bind(sockfd, addr, addrlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef socket
int qemu_socket_wrap(int domain, int type, int protocol)
{
    int ret;
    ret = socket(domain, type, protocol);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef accept
int qemu_accept_wrap(int sockfd, struct sockaddr *addr,
                     socklen_t *addrlen)
{
    int ret;
    ret = accept(sockfd, addr, addrlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef shutdown
int qemu_shutdown_wrap(int sockfd, int how)
{
    int ret;
    ret = shutdown(sockfd, how);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef ioctlsocket
int qemu_ioctlsocket_wrap(int fd, int req, void *val)
{
    int ret;
    ret = ioctlsocket(fd, req, val);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef closesocket
int qemu_closesocket_wrap(int fd)
{
    int ret;
    ret = closesocket(fd);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef getsockopt
int qemu_getsockopt_wrap(int sockfd, int level, int optname,
                         void *optval, socklen_t *optlen)
{
    int ret;
    ret = getsockopt(sockfd, level, optname, optval, optlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef setsockopt
int qemu_setsockopt_wrap(int sockfd, int level, int optname,
                         const void *optval, socklen_t optlen)
{
    int ret;
    ret = setsockopt(sockfd, level, optname, optval, optlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef getpeername
int qemu_getpeername_wrap(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen)
{
    int ret;
    ret = getpeername(sockfd, addr, addrlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef getsockname
int qemu_getsockname_wrap(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen)
{
    int ret;
    ret = getsockname(sockfd, addr, addrlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef send
ssize_t qemu_send_wrap(int sockfd, const void *buf, size_t len, int flags)
{
    int ret;
    ret = send(sockfd, buf, len, flags);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef sendto
ssize_t qemu_sendto_wrap(int sockfd, const void *buf, size_t len, int flags,
                         const struct sockaddr *addr, socklen_t addrlen)
{
    int ret;
    ret = sendto(sockfd, buf, len, flags, addr, addrlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef recv
ssize_t qemu_recv_wrap(int sockfd, void *buf, size_t len, int flags)
{
    int ret;
    ret = recv(sockfd, buf, len, flags);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}


#undef recvfrom
ssize_t qemu_recvfrom_wrap(int sockfd, void *buf, size_t len, int flags,
                           struct sockaddr *addr, socklen_t *addrlen)
{
    int ret;
    ret = recvfrom(sockfd, buf, len, flags, addr, addrlen);
    if (ret < 0) {
        errno = socket_error();
    }
    return ret;
}

int qemu_stat(const char* path, struct stat* st)
{
    return win32_stat(path, st);
}

int qemu_lstat(const char* path, struct stat* st)
{
    return win32_lstat(path, st);
}

#ifdef _MSC_VER

#ifdef USE_QEMU_GETOPT

int opterr = 1;      /* if error message should be printed */
int optind = 1;      /* index into parent argv vector */
int optopt = '?';    /* character checked for validity */
char* optarg = NULL; /* argument associated with option */

#define PRINT_ERROR ((opterr) && (*options != ':'))

#define FLAG_PERMUTE 0x01  /* permute non-options to the end of argv */
#define FLAG_ALLARGS 0x02  /* treat non-options as args to option "-1" */
#define FLAG_LONGONLY 0x04 /* operate as getopt_long_only */

/* return values */
#define BADCH (int)'?'
#define BADARG ((*options == ':') ? (int)':' : (int)'?')
#define INORDER (int)1

#define __progname __argv[0]
#define EMSG ""

static int getopt_internal(int,
                           char* const*,
                           const char*,
                           const struct option*,
                           int*,
                           int);
static int parse_long_options(char* const*,
                              const char*,
                              const struct option*,
                              int*,
                              int);
static int gcd(int, int);
static void permute_args(int, int, int, char* const*);

static char* place = EMSG; /* option letter processing */

static int nonopt_start = -1; /* first non option argument (for permute) */
static int nonopt_end = -1;   /* first option after non options (for permute) */

/* Error messages */
static const char recargchar[] = "option requires an argument -- %c";
static const char recargstring[] = "option requires an argument -- %s";
static const char ambig[] = "ambiguous option -- %.*s";
static const char noarg[] = "option doesn't take an argument -- %.*s";
static const char illoptchar[] = "unknown option -- %c";
static const char illoptstring[] = "unknown option -- %s";

static void _vwarnx(const char* fmt, va_list ap) {
    (void)fprintf(stderr, "%s: ", __progname);
    if (fmt != NULL)
        (void)vfprintf(stderr, fmt, ap);
    (void)fprintf(stderr, "\n");
}

static void warnx(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    _vwarnx(fmt, ap);
    va_end(ap);
}

/*
 * Compute the greatest common divisor of a and b.
 */
static int gcd(int a, int b) {
    int c;

    c = a % b;
    while (c != 0) {
        a = b;
        b = c;
        c = a % b;
    }

    return (b);
}

/*
 * Exchange the block from nonopt_start to nonopt_end with the block
 * from nonopt_end to opt_end (keeping the same order of arguments
 * in each block).
 */
static void permute_args(int panonopt_start,
                         int panonopt_end,
                         int opt_end,
                         char* const* nargv) {
    int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
    char* swap;

    /*
     * compute lengths of blocks and number and size of cycles
     */
    nnonopts = panonopt_end - panonopt_start;
    nopts = opt_end - panonopt_end;
    ncycle = gcd(nnonopts, nopts);
    cyclelen = (opt_end - panonopt_start) / ncycle;

    for (i = 0; i < ncycle; i++) {
        cstart = panonopt_end + i;
        pos = cstart;
        for (j = 0; j < cyclelen; j++) {
            if (pos >= panonopt_end)
                pos -= nnonopts;
            else
                pos += nopts;
            swap = nargv[pos];
            /* LINTED const cast */
            ((char**)nargv)[pos] = nargv[cstart];
            /* LINTED const cast */
            ((char**)nargv)[cstart] = swap;
        }
    }
}

/*
 * parse_long_options --
 *	Parse long options in argc/argv argument vector.
 * Returns -1 if short_too is set and the option does not match long_options.
 */
static int parse_long_options(char* const* nargv,
                              const char* options,
                              const struct option* long_options,
                              int* idx,
                              int short_too) {
    char *current_argv, *has_equal;
    size_t current_argv_len;
    int i, ambiguous, match;

#define IDENTICAL_INTERPRETATION(_x, _y)                         \
    (long_options[(_x)].has_arg == long_options[(_y)].has_arg && \
     long_options[(_x)].flag == long_options[(_y)].flag &&       \
     long_options[(_x)].val == long_options[(_y)].val)

    current_argv = place;
    match = -1;
    ambiguous = 0;

    optind++;

    if ((has_equal = strchr(current_argv, '=')) != NULL) {
        /* argument found (--option=arg) */
        current_argv_len = has_equal - current_argv;
        has_equal++;
    } else
        current_argv_len = strlen(current_argv);

    for (i = 0; long_options[i].name; i++) {
        /* find matching long option */
        if (strncmp(current_argv, long_options[i].name, current_argv_len))
            continue;

        if (strlen(long_options[i].name) == current_argv_len) {
            /* exact match */
            match = i;
            ambiguous = 0;
            break;
        }
        /*
         * If this is a known short option, don't allow
         * a partial match of a single character.
         */
        if (short_too && current_argv_len == 1)
            continue;

        if (match == -1) /* partial match */
            match = i;
        else if (!IDENTICAL_INTERPRETATION(i, match))
            ambiguous = 1;
    }
    if (ambiguous) {
        /* ambiguous abbreviation */
        if (PRINT_ERROR)
            warnx(ambig, (int)current_argv_len, current_argv);
        optopt = 0;
        return (BADCH);
    }
    if (match != -1) { /* option found */
        if (long_options[match].has_arg == no_argument && has_equal) {
            if (PRINT_ERROR)
                warnx(noarg, (int)current_argv_len, current_argv);
            /*
             * XXX: GNU sets optopt to val regardless of flag
             */
            if (long_options[match].flag == NULL)
                optopt = long_options[match].val;
            else
                optopt = 0;
            return (BADARG);
        }
        if (long_options[match].has_arg == required_argument ||
            long_options[match].has_arg == optional_argument) {
            if (has_equal)
                optarg = has_equal;
            else if (long_options[match].has_arg == required_argument) {
                /*
                 * optional argument doesn't use next nargv
                 */
                optarg = nargv[optind++];
            }
        }
        if ((long_options[match].has_arg == required_argument) &&
            (optarg == NULL)) {
            /*
             * Missing argument; leading ':' indicates no error
             * should be generated.
             */
            if (PRINT_ERROR)
                warnx(recargstring, current_argv);
            /*
             * XXX: GNU sets optopt to val regardless of flag
             */
            if (long_options[match].flag == NULL)
                optopt = long_options[match].val;
            else
                optopt = 0;
            --optind;
            return (BADARG);
        }
    } else { /* unknown option */
        if (short_too) {
            --optind;
            return (-1);
        }
        if (PRINT_ERROR)
            warnx(illoptstring, current_argv);
        optopt = 0;
        return (BADCH);
    }
    if (idx)
        *idx = match;
    if (long_options[match].flag) {
        *long_options[match].flag = long_options[match].val;
        return (0);
    } else
        return (long_options[match].val);
#undef IDENTICAL_INTERPRETATION
}

/*
 * getopt_internal --
 *	Parse argc/argv argument vector.  Called by user level routines.
 */
static int getopt_internal(int nargc,
                           char* const* nargv,
                           const char* options,
                           const struct option* long_options,
                           int* idx,
                           int flags) {
    char* oli; /* option letter list index */
    int optchar, short_too;
    static int posixly_correct = -1;

    if (options == NULL)
        return (-1);

    if (optind == 0)
        optind = 1;

    /*
     * Disable GNU extensions if POSIXLY_CORRECT is set or options
     * string begins with a '+'.
     *
     * CV, 2009-12-14: Check POSIXLY_CORRECT anew if optind == 0 or
     *                 optreset != 0 for GNU compatibility.
     */
    if (posixly_correct == -1)
        posixly_correct = (getenv("POSIXLY_CORRECT") != NULL);
    if (*options == '-')
        flags |= FLAG_ALLARGS;
    else if (posixly_correct || *options == '+')
        flags &= ~FLAG_PERMUTE;
    if (*options == '+' || *options == '-')
        options++;

    optarg = NULL;
start:
    if (!*place) {             /* update scanning pointer */
        if (optind >= nargc) { /* end of argument vector */
            place = EMSG;
            if (nonopt_end != -1) {
                /* do permutation, if we have to */
                permute_args(nonopt_start, nonopt_end, optind, nargv);
                optind -= nonopt_end - nonopt_start;
            } else if (nonopt_start != -1) {
                /*
                 * If we skipped non-options, set optind
                 * to the first of them.
                 */
                optind = nonopt_start;
            }
            nonopt_start = nonopt_end = -1;
            return (-1);
        }
        if (*(place = nargv[optind]) != '-' ||
            (place[1] == '\0' && strchr(options, '-') == NULL)) {
            place = EMSG; /* found non-option */
            if (flags & FLAG_ALLARGS) {
                /*
                 * GNU extension:
                 * return non-option as argument to option 1
                 */
                optarg = nargv[optind++];
                return (INORDER);
            }
            if (!(flags & FLAG_PERMUTE)) {
                /*
                 * If no permutation wanted, stop parsing
                 * at first non-option.
                 */
                return (-1);
            }
            /* do permutation */
            if (nonopt_start == -1)
                nonopt_start = optind;
            else if (nonopt_end != -1) {
                permute_args(nonopt_start, nonopt_end, optind, nargv);
                nonopt_start = optind - (nonopt_end - nonopt_start);
                nonopt_end = -1;
            }
            optind++;
            /* process next argument */
            goto start;
        }
        if (nonopt_start != -1 && nonopt_end == -1)
            nonopt_end = optind;

        /*
         * If we have "-" do nothing, if "--" we are done.
         */
        if (place[1] != '\0' && *++place == '-' && place[1] == '\0') {
            optind++;
            place = EMSG;
            /*
             * We found an option (--), so if we skipped
             * non-options, we have to permute.
             */
            if (nonopt_end != -1) {
                permute_args(nonopt_start, nonopt_end, optind, nargv);
                optind -= nonopt_end - nonopt_start;
            }
            nonopt_start = nonopt_end = -1;
            return (-1);
        }
    }

    /*
     * Check long options if:
     *  1) we were passed some
     *  2) the arg is not just "-"
     *  3) either the arg starts with -- we are getopt_long_only()
     */
    if (long_options != NULL && place != nargv[optind] &&
        (*place == '-' || (flags & FLAG_LONGONLY))) {
        short_too = 0;
        if (*place == '-')
            place++; /* --foo long option */
        else if (*place != ':' && strchr(options, *place) != NULL)
            short_too = 1; /* could be short option too */

        optchar = parse_long_options(nargv, options, long_options, idx,
                                     short_too);
        if (optchar != -1) {
            place = EMSG;
            return (optchar);
        }
    }

    if ((optchar = (int)*place++) == (int)':' ||
        (optchar == (int)'-' && *place != '\0') ||
        (oli = strchr(options, optchar)) == NULL) {
        /*
         * If the user specified "-" and  '-' isn't listed in
         * options, return -1 (non-option) as per POSIX.
         * Otherwise, it is an unknown option character (or ':').
         */
        if (optchar == (int)'-' && *place == '\0')
            return (-1);
        if (!*place)
            ++optind;
        if (PRINT_ERROR)
            warnx(illoptchar, optchar);
        optopt = optchar;
        return (BADCH);
    }
    if (long_options != NULL && optchar == 'W' && oli[1] == ';') {
        /* -W long-option */
        if (*place) /* no space */
            /* NOTHING */;
        else if (++optind >= nargc) { /* no arg */
            place = EMSG;
            if (PRINT_ERROR)
                warnx(recargchar, optchar);
            optopt = optchar;
            return (BADARG);
        } else /* white space */
            place = nargv[optind];
        optchar = parse_long_options(nargv, options, long_options, idx, 0);
        place = EMSG;
        return (optchar);
    }
    if (*++oli != ':') { /* doesn't take argument */
        if (!*place)
            ++optind;
    } else { /* takes (optional) argument */
        optarg = NULL;
        if (*place) /* no white space */
            optarg = place;
        else if (oli[1] != ':') {    /* arg not optional */
            if (++optind >= nargc) { /* no arg */
                place = EMSG;
                if (PRINT_ERROR)
                    warnx(recargchar, optchar);
                optopt = optchar;
                return (BADARG);
            } else
                optarg = nargv[optind];
        }
        place = EMSG;
        ++optind;
    }
    /* dump back option letter */
    return (optchar);
}

/*
 * getopt --
 *	Parse argc/argv argument vector.
 *
 * [eventually this will replace the BSD getopt]
 */
#ifndef _MSC_VER
int getopt(int nargc, char* const* nargv, const char* options) {
    /*
     * We don't pass FLAG_PERMUTE to getopt_internal() since
     * the BSD getopt(3) (unlike GNU) has never done this.
     *
     * Furthermore, since many privileged programs call getopt()
     * before dropping privileges it makes sense to keep things
     * as simple (and bug-free) as possible.
     */
    return (getopt_internal(nargc, nargv, options, NULL, NULL, 0));
}
#endif

/*
 * getopt_long --
 *	Parse argc/argv argument vector.
 */
int getopt_long(int nargc,
                char* const* nargv,
                const char* options,
                const struct option* long_options,
                int* idx) {
    return (getopt_internal(nargc, nargv, options, long_options, idx,
                            FLAG_PERMUTE));
}

/*
 * getopt_long_only --
 *	Parse argc/argv argument vector.
 */
int getopt_long_only(int nargc,
                     char* const* nargv,
                     const char* options,
                     const struct option* long_options,
                     int* idx) {
    return (getopt_internal(nargc, nargv, options, long_options, idx,
                            FLAG_PERMUTE | FLAG_LONGONLY));
}

#endif  // USE_QEMU_GETOPT

int qemu_mkstemp(char* t) {
    int len = strlen(t) + 1;
    errno_t err = _mktemp_s(t, len);

    if (err != 0) {
        return -1;
    }

    return _sopen(t, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYRW,
                  _S_IREAD | _S_IWRITE);
}

#endif  // _MSC_VER

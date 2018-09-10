/*
 * win32 msvc specific declarations
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
 * Copyright (c) 2010 Jes Sorensen <Jes.Sorensen@redhat.com>
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
 */

#ifndef QEMU_OS_WIN32_MSVC_H
#define QEMU_OS_WIN32_MSVC_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <basetsd.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <ehstorioctl.h>

#define MAP_FAILED ((void*)(-1))

#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif
#ifndef STDERR_FILENO
#define STDERR_FILENO _fileno(stderr)
#endif  // !STDERR_FILENO
#ifndef STDIN_FILENO
#define STDIN_FILENO _fileno(stdin)
#endif  // !STDIN_FILENO
#ifndef STDOUT_FILENO
#define STDOUT_FILENO _fileno(stdout)
#endif  // !STDOUT_FILENO

typedef SSIZE_T ssize_t;
typedef int mode_t;
#ifdef _WIN64
typedef int64_t pid_t;
#else
typedef int pid_t;
#endif
typedef int64_t off64_t;

#define strcasecmp _stricmp
#define gmtime_r(i,m) gmtime_s(m,i)
#define localtime_r(i, m) localtime_s(m, i)
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif

// MSVC has #define interface struct, but it conflicts with some names
// in qemu code. Undefine it.
#ifdef interface
#undef interface
#endif
#ifdef small
#undef small
#endif

/* QEMU uses sigsetjmp()/siglongjmp() as the portable way to specify
 * "longjmp and don't touch the signal masks". Since we know that the
 * savemask parameter will always be zero we can safely define these
 * in terms of setjmp/longjmp on Win32.
 */
#define sigjmp_buf jmp_buf

#ifndef _WIN64

#define sigsetjmp(env, savemask) setjmp(env)
#define siglongjmp(env, val) longjmp(env, val)

#else

/* On w64, setjmp implementation tends to crash in MinGW (there were several
 * attempts to fix it, but it keeps crashing). The only good implementation
 * is the GCC's undocumented builtins - so let's use them.
 */
# undef setjmp
# undef longjmp
# define setjmp __builtin_setjmp
# define longjmp __builtin_longjmp
# define sigsetjmp(x,y) __builtin_setjmp((x))
# define siglongjmp(x,y) __builtin_longjmp((x),(y))

#endif

/* Missing POSIX functions. Don't use MinGW-w64 macros. */
#ifndef CONFIG_LOCALTIME_R
#undef gmtime_r
struct tm *gmtime_r(const time_t *timep, struct tm *result);
#undef localtime_r
struct tm *localtime_r(const time_t *timep, struct tm *result);
#endif /* CONFIG_LOCALTIME_R */

static inline void os_setup_signal_handling(void) {}
static inline void os_daemonize(void) {}
static inline void os_setup_post(void) {}
void os_set_line_buffering(void);
static inline void os_set_proc_name(const char *dummy) {}

int getpagesize(void);

#if !defined(EPROTONOSUPPORT)
# define EPROTONOSUPPORT EINVAL
#endif

int setenv(const char *name, const char *value, int overwrite);

typedef struct timeval qemu_timeval;
int qemu_gettimeofday(struct timeval *tp);
int qemu_mkstemp(char* t);

static inline bool is_daemonized(void)
{
    return false;
}

static inline int os_mlock(void)
{
    return -ENOSYS;
}

#define fsync _commit

#if !defined(lseek)
# define lseek _lseeki64
#endif

int qemu_ftruncate64(int, int64_t);

#if !defined(ftruncate)
# define ftruncate qemu_ftruncate64
#endif

static inline char *realpath(const char *path, char *resolved_path)
{
    _fullpath(resolved_path, path, _MAX_PATH);
    return resolved_path;
}

/* ??? Mingw appears to export _lock_file and _unlock_file as the functions
 * with which to lock a stdio handle.  But something is wrong in the markup,
 * either in the header or the library, such that we get undefined references
 * to "_imp___lock_file" etc when linking.  Since we seem to have no other
 * alternative, and the usage within the logging functions isn't critical,
 * ignore FILE locking.
 */

static inline void qemu_flockfile(FILE *f)
{
}

static inline void qemu_funlockfile(FILE *f)
{
}

/* We wrap all the sockets functions so that we can
 * set errno based on WSAGetLastError()
 */

#undef connect
#define connect qemu_connect_wrap
int qemu_connect_wrap(int sockfd, const struct sockaddr *addr,
                      socklen_t addrlen);

#undef listen
#define listen qemu_listen_wrap
int qemu_listen_wrap(int sockfd, int backlog);

#undef bind
#define bind qemu_bind_wrap
int qemu_bind_wrap(int sockfd, const struct sockaddr *addr,
                   socklen_t addrlen);

#undef socket
#define socket qemu_socket_wrap
int qemu_socket_wrap(int domain, int type, int protocol);

#undef accept
#define accept qemu_accept_wrap
int qemu_accept_wrap(int sockfd, struct sockaddr *addr,
                     socklen_t *addrlen);

#undef shutdown
#define shutdown qemu_shutdown_wrap
int qemu_shutdown_wrap(int sockfd, int how);

#undef ioctlsocket
#define ioctlsocket qemu_ioctlsocket_wrap
int qemu_ioctlsocket_wrap(int fd, int req, void *val);

#undef closesocket
#define closesocket qemu_closesocket_wrap
int qemu_closesocket_wrap(int fd);

#undef getsockopt
#define getsockopt qemu_getsockopt_wrap
int qemu_getsockopt_wrap(int sockfd, int level, int optname,
                         void *optval, socklen_t *optlen);

#undef setsockopt
#define setsockopt qemu_setsockopt_wrap
int qemu_setsockopt_wrap(int sockfd, int level, int optname,
                         const void *optval, socklen_t optlen);

#undef getpeername
#define getpeername qemu_getpeername_wrap
int qemu_getpeername_wrap(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen);

#undef getsockname
#define getsockname qemu_getsockname_wrap
int qemu_getsockname_wrap(int sockfd, struct sockaddr *addr,
                          socklen_t *addrlen);

#undef send
#define send qemu_send_wrap
ssize_t qemu_send_wrap(int sockfd, const void *buf, size_t len, int flags);

#undef sendto
#define sendto qemu_sendto_wrap
ssize_t qemu_sendto_wrap(int sockfd, const void *buf, size_t len, int flags,
                         const struct sockaddr *addr, socklen_t addrlen);

#undef recv
#define recv qemu_recv_wrap
ssize_t qemu_recv_wrap(int sockfd, void *buf, size_t len, int flags);

#undef recvfrom
#define recvfrom qemu_recvfrom_wrap
ssize_t qemu_recvfrom_wrap(int sockfd, void *buf, size_t len, int flags,
                           struct sockaddr *addr, socklen_t *addrlen);

// ANDROID_BEGIN
/* These are wrappers around Win32 functions. When building against the
 * Android emulator, they will treat file names as UTF-8 encoded strings,
 * instead of ANSI ones. */
HANDLE win32CreateFile(
        LPCTSTR               lpFileName,
        DWORD                 dwDesiredAccess,
        DWORD                 dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD                 dwCreationDisposition,
        DWORD                 dwFlagsAndAttributes,
        HANDLE                hTemplateFile);

DWORD win32GetCurrentDirectory(
        DWORD  nBufferLength,
        LPTSTR lpBuffer);

DWORD win32GetModuleFileName(
        HMODULE hModule,
        LPTSTR  lpFilename,
        DWORD   nSize);
// ANDROID_END


// Define USE_QEMU_GETOPT if you want to use this getopt.
#ifdef USE_QEMU_GETOPT

#define optind qemu_optind
extern int qemu_optind;   /* index of first non-option in argv      */
#define optopt qemu_optopt
extern int qemu_optopt;   /* single option character, as parsed     */
#define opterr qemu_opterr
extern int qemu_opterr;   /* flag to enable built-in diagnostics... */
                     /* (user may set to zero, to suppress)    */
#define optarg qemu_optarg
char* qemu_optarg; /* pointer to argument of current option  */
#define getopt qemu_getopt
int qemu_getopt(int nargc, char * const *nargv, const char *options);

struct option		/* specification for a long form option...	*/
{
  const char *name;		/* option name, without leading hyphens */
  int         has_arg;		/* does it take an argument?		*/
  int        *flag;		/* where to save its status, or NULL	*/
  int         val;		/* its associated status value		*/
};

enum    		/* permitted values for its `has_arg' field...	*/
{
  no_argument = 0,      	/* option never takes an argument	*/
  required_argument,		/* option always requires an argument	*/
  optional_argument		/* option may take an argument		*/
};

#define getopt_long qemu_getopt_long
int qemu_getopt_long(int nargc, char * const *nargv, const char *options,
    const struct option *long_options, int *idx);
#define getopt_long_only qemu_getopt_long_only
int qemu_getopt_long_only(int nargc, char * const *nargv, const char *options,
    const struct option *long_options, int *idx);
#endif  // USE_QEMU_GETOPT

#define gettimeofday(tv,tz) qemu_gettimeofday(tv)
#define mkstemp qemu_mkstemp

#endif  // QEMU_OS_WIN32_MSVC_H

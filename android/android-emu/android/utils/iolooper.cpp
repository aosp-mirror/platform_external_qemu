#include "android/utils/iolooper.h"

#include "android/base/sockets/SocketWaiter.h"

#include <stddef.h>
#include <sys/types.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

using namespace android::base;

namespace {

SocketWaiter* asWaiter(IoLooper* iol) {
    return reinterpret_cast<SocketWaiter*>(iol);
}

unsigned toWaiterEvents(int flags) {
    unsigned events = 0;
    if (flags & IOLOOPER_READ) {
        events |= SocketWaiter::kEventRead;
    }
    if (flags & IOLOOPER_WRITE) {
        events |= SocketWaiter::kEventWrite;
    }
    return events;
}

}  // namespace

IoLooper* iolooper_new(void) {
    return reinterpret_cast<IoLooper*>(SocketWaiter::create());
}

void iolooper_free(IoLooper*  iol) {
    delete asWaiter(iol);
}

void iolooper_reset(IoLooper*  iol) {
    asWaiter(iol)->reset();
}

void iolooper_modify(IoLooper* iol, int fd, int oldflags, int newflags) {
    asWaiter(iol)->update(fd, toWaiterEvents(newflags));
}

void iolooper_add_read(IoLooper* iol, int fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events | SocketWaiter::kEventRead);
}

void iolooper_add_write(IoLooper* iol, int fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events | SocketWaiter::kEventWrite);
}

void iolooper_del_read(IoLooper*  iol, int  fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events & ~SocketWaiter::kEventRead);
}

void iolooper_del_write(IoLooper*  iol, int  fd) {
    SocketWaiter* waiter = asWaiter(iol);
    unsigned events = waiter->wantedEventsFor(fd);
    waiter->update(fd, events & ~SocketWaiter::kEventWrite);
}

int iolooper_wait(IoLooper* iol, int64_t  duration) {
    return asWaiter(iol)->wait(duration);
}

int iolooper_is_read(IoLooper* iol, int fd) {
    return asWaiter(iol)->pendingEventsFor(fd) & SocketWaiter::kEventRead;
}

int iolooper_is_write(IoLooper* iol, int  fd) {
    return asWaiter(iol)->pendingEventsFor(fd) & SocketWaiter::kEventWrite;
}

int iolooper_has_operations(IoLooper* iol) {
    return asWaiter(iol)->hasFds();
}

/* Offset between 1/1/1601 and 1/1/1970 in 100 nanosec units */
#define _WIN32_FT_OFFSET (116444736000000000ULL)
int64_t iolooper_now(void) {
    struct timeval time_now;
#ifdef _WIN32
    // Taken from qemu_gettimeofday() from oslib-win32.c
    union {
      unsigned long long ns100; /*time since 1 Jan 1601 in 100ns units */
      FILETIME ft;
    }  _now;

    GetSystemTimeAsFileTime (&_now.ft);
    time_now.tv_usec=(long)((_now.ns100 / 10ULL) % 1000000ULL );
    time_now.tv_sec= (long)((_now.ns100 - _WIN32_FT_OFFSET) / 10000000ULL);
    return (int64_t)time_now.tv_sec * time_now.tv_usec / 1000;
#else
    return gettimeofday(&time_now, NULL) ? -1 : (int64_t)time_now.tv_sec * 1000LL +
                                                time_now.tv_usec / 1000;
#endif
}

int iolooper_wait_absolute(IoLooper* iol, int64_t deadline) {
    int64_t timeout = deadline - iolooper_now();

    /* If the deadline has passed, set the timeout to 0, this allows us
     * to poll the file descriptor nonetheless */
    if (timeout < 0)
        timeout = 0;

    return iolooper_wait(iol, timeout);
}

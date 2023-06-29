/*
 * QEMU System Emulator
 *
 * Copyright (c) 2003-2008 Fabrice Bellard
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

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qemu/cutils.h"
#include "qemu/timer.h"
#include "qemu/sockets.h"	// struct in_addr needed for libslirp.h
#include "sysemu/qtest.h"
#include "sysemu/cpus.h"
#include "sysemu/replay.h"
#include "slirp/libslirp.h"
#include "qemu/main-loop.h"
#include "block/aio.h"
#include "qemu/error-report.h"

#define DEBUG_MAIN_LOOP_PERF 0

static MainLoopPollCallback main_loop_poll_callback;

void main_loop_register_poll_callback(MainLoopPollCallback poll_func)
{
    main_loop_poll_callback = poll_func;
}

#ifndef _WIN32

/* If we have signalfd, we mask out the signals we want to handle and then
 * use signalfd to listen for them.  We rely on whatever the current signal
 * handler is to dispatch the signals when we receive them.
 */
static void sigfd_handler(void *opaque)
{
    int fd = (intptr_t)opaque;
    struct qemu_signalfd_siginfo info;
    struct sigaction action;
    ssize_t len;

    while (1) {
        do {
            len = read(fd, &info, sizeof(info));
        } while (len == -1 && errno == EINTR);

        if (len == -1 && errno == EAGAIN) {
            break;
        }

        if (len != sizeof(info)) {
            printf("read from sigfd returned %zd: %m\n", len);
            return;
        }

        sigaction(info.ssi_signo, NULL, &action);
        if ((action.sa_flags & SA_SIGINFO) && action.sa_sigaction) {
            sigaction_invoke(&action, &info);
        } else if (action.sa_handler) {
            action.sa_handler(info.ssi_signo);
        }
    }
}

static int qemu_signal_init(void)
{
    int sigfd;
    sigset_t set;

    /*
     * SIG_IPI must be blocked in the main thread and must not be caught
     * by sigwait() in the signal thread. Otherwise, the cpu thread will
     * not catch it reliably.
     */
    sigemptyset(&set);
    sigaddset(&set, SIG_IPI);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGBUS);
    /* SIGINT cannot be handled via signalfd, so that ^C can be used
     * to interrupt QEMU when it is being run under gdb.  SIGHUP and
     * SIGTERM are also handled asynchronously, even though it is not
     * strictly necessary, because they use the same handler as SIGINT.
     */
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    sigdelset(&set, SIG_IPI);
    sigfd = qemu_signalfd(&set);
    if (sigfd == -1) {
        fprintf(stderr, "failed to create signalfd\n");
        return -errno;
    }

    fcntl_setfl(sigfd, O_NONBLOCK);

    qemu_set_fd_handler(sigfd, sigfd_handler, NULL, (void *)(intptr_t)sigfd);

    return 0;
}

#else /* _WIN32 */

static int qemu_signal_init(void)
{
    return 0;
}
#endif

static AioContext *qemu_aio_context;
static QEMUBH *qemu_notify_bh;

static void notify_event_cb(void *opaque)
{
    /* No need to do anything; this bottom half is only used to
     * kick the kernel out of ppoll/poll/WaitForMultipleObjects.
     */
}

AioContext *qemu_get_aio_context(void)
{
    return qemu_aio_context;
}

void qemu_notify_event(void)
{
    if (!qemu_aio_context) {
        return;
    }
    qemu_bh_schedule(qemu_notify_bh);
}

static GArray *gpollfds;

int qemu_init_main_loop(Error **errp)
{
    int ret;
    GSource *src;
    Error *local_error = NULL;

    init_clocks(qemu_timer_notify_cb);

    ret = qemu_signal_init();
    if (ret) {
        return ret;
    }

    qemu_aio_context = aio_context_new(&local_error);
    if (!qemu_aio_context) {
        error_propagate(errp, local_error);
        return -EMFILE;
    }
    qemu_notify_bh = qemu_bh_new(notify_event_cb, NULL);
    gpollfds = g_array_new(FALSE, FALSE, sizeof(GPollFD));
    src = aio_get_g_source(qemu_aio_context);
    g_source_set_name(src, "aio-context");
    g_source_attach(src, NULL);
    g_source_unref(src);
    src = iohandler_get_g_source();
    g_source_set_name(src, "io-handler");
    g_source_attach(src, NULL);
    g_source_unref(src);
    return 0;
}

static int max_priority;

#ifndef _WIN32
static int glib_pollfds_idx;
static int glib_n_poll_fds;

static void glib_pollfds_fill(int64_t *cur_timeout)
{
    GMainContext *context = g_main_context_default();
    int timeout = 0;
    int64_t timeout_ns;
    int n;

    g_main_context_prepare(context, &max_priority);

    glib_pollfds_idx = gpollfds->len;
    n = glib_n_poll_fds;
    do {
        GPollFD *pfds = NULL;
        glib_n_poll_fds = n;
        g_array_set_size(gpollfds, glib_pollfds_idx + glib_n_poll_fds);
        if (gpollfds->data) {
            pfds = &g_array_index(gpollfds, GPollFD, glib_pollfds_idx);
        }
        n = g_main_context_query(context, max_priority, &timeout, pfds,
                                glib_n_poll_fds);
    } while (n != glib_n_poll_fds);

    if (timeout < 0) {
        timeout_ns = -1;
    } else {
        timeout_ns = (int64_t)timeout * (int64_t)SCALE_MS;
    }

    *cur_timeout = qemu_soonest_timeout(timeout_ns, *cur_timeout);
}

static void glib_pollfds_poll(void)
{
    GMainContext *context = g_main_context_default();
    GPollFD *pfds = &g_array_index(gpollfds, GPollFD, glib_pollfds_idx);

    if (g_main_context_check(context, max_priority, pfds, glib_n_poll_fds)) {
        g_main_context_dispatch(context);
    }
}

#define MAX_MAIN_LOOP_SPIN (1000)

static int os_host_main_loop_wait(int64_t timeout)
{
    GMainContext *context = g_main_context_default();
    int ret;
    static int spin_counter;

    g_main_context_acquire(context);

    glib_pollfds_fill(&timeout);

    /* If the I/O thread is very busy or we are incorrectly busy waiting in
     * the I/O thread, this can lead to starvation of the BQL such that the
     * VCPU threads never run.  To make sure we can detect the later case,
     * print a message to the screen.  If we run into this condition, create
     * a fake timeout in order to give the VCPU threads a chance to run.
     */
    if (!timeout && (spin_counter > MAX_MAIN_LOOP_SPIN)) {
        static bool notified;

        if (!notified && !qtest_enabled() && !qtest_driver()) {
            warn_report("I/O thread spun for %d iterations",
                        MAX_MAIN_LOOP_SPIN);
            notified = true;
        }

        timeout = SCALE_MS;
    }


    if (timeout) {
        spin_counter = 0;
    } else {
        spin_counter++;
    }
    qemu_mutex_unlock_iothread();
    replay_mutex_unlock();

    ret = qemu_poll_ns((GPollFD *)gpollfds->data, gpollfds->len, timeout);

    replay_mutex_lock();
    qemu_mutex_lock_iothread();

    glib_pollfds_poll();

    g_main_context_release(context);

    return ret;
}
#else
/***********************************************************/
/* Polling handling */

typedef struct PollingEntry {
    PollingFunc *func;
    void *opaque;
    struct PollingEntry *next;
} PollingEntry;

static PollingEntry *first_polling_entry;

int qemu_add_polling_cb(PollingFunc *func, void *opaque)
{
    PollingEntry **ppe, *pe;
    pe = g_malloc0(sizeof(PollingEntry));
    pe->func = func;
    pe->opaque = opaque;
    for(ppe = &first_polling_entry; *ppe != NULL; ppe = &(*ppe)->next);
    *ppe = pe;
    return 0;
}

void qemu_del_polling_cb(PollingFunc *func, void *opaque)
{
    PollingEntry **ppe, *pe;
    for(ppe = &first_polling_entry; *ppe != NULL; ppe = &(*ppe)->next) {
        pe = *ppe;
        if (pe->func == func && pe->opaque == opaque) {
            *ppe = pe->next;
            g_free(pe);
            break;
        }
    }
}

/***********************************************************/
/* Wait objects support */
typedef struct WaitObjects {
    int num;
    int revents[MAXIMUM_WAIT_OBJECTS + 1];
    HANDLE events[MAXIMUM_WAIT_OBJECTS + 1];
    WaitObjectFunc *func[MAXIMUM_WAIT_OBJECTS + 1];
    void *opaque[MAXIMUM_WAIT_OBJECTS + 1];
} WaitObjects;

static WaitObjects wait_objects = {0};

int qemu_add_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque)
{
    WaitObjects *w = &wait_objects;
    if (w->num >= MAXIMUM_WAIT_OBJECTS) {
        return -1;
    }
    w->events[w->num] = handle;
    w->func[w->num] = func;
    w->opaque[w->num] = opaque;
    w->revents[w->num] = 0;
    w->num++;
    return 0;
}

void qemu_del_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque)
{
    int i, found;
    WaitObjects *w = &wait_objects;

    found = 0;
    for (i = 0; i < w->num; i++) {
        if (w->events[i] == handle) {
            found = 1;
        }
        if (found) {
            w->events[i] = w->events[i + 1];
            w->func[i] = w->func[i + 1];
            w->opaque[i] = w->opaque[i + 1];
            w->revents[i] = w->revents[i + 1];
        }
    }
    if (found) {
        w->num--;
    }
}

void qemu_fd_register(int fd)
{
    WSAEventSelect(fd, event_notifier_get_handle(&qemu_aio_context->notifier),
                   FD_READ | FD_ACCEPT | FD_CLOSE |
                   FD_CONNECT | FD_WRITE | FD_OOB);
}

static int pollfds_fill(GArray *pollfds, WSAPOLLFD fds[], int fds_count, int* sockets_count)
{
    int i;
    *sockets_count = 0;
    for (i = 0; i < pollfds->len && i < fds_count; i++) {
        GPollFD *pfd = &g_array_index(pollfds, GPollFD, i);
        int events = pfd->events;
        fds[i].events = 0;
        if (events & G_IO_IN) {
            fds[i].events |= POLLRDNORM;
        }
        if (events & G_IO_OUT) {
            fds[i].events |= POLLOUT;
        }
        if (events & G_IO_PRI) {
            fds[i].events |= POLLRDBAND;
        }

        if (fds[i].events == 0) {
            fds[i].fd = INVALID_SOCKET; // ignore this one
        } else {
            ++*sockets_count;
            fds[i].fd = pfd->fd;
        }
    }
    return i;
}

static void pollfds_poll(GArray *pollfds, const WSAPOLLFD fds[], int fds_count)
{
    int i;
    for (i = 0; i < pollfds->len; i++) {
        GPollFD *pfd = &g_array_index(pollfds, GPollFD, i);
        int revents = 0;
        if (i < fds_count && fds[i].fd != INVALID_SOCKET) {
            if (fds[i].revents & (POLLRDNORM | POLLERR | POLLHUP)) {
                revents |= G_IO_IN;
            }
            if (fds[i].revents & POLLWRNORM) {
                revents |= G_IO_OUT;
            }
            if (fds[i].revents & (POLLERR | POLLHUP | POLLPRI | POLLRDBAND)) {
                revents |= G_IO_PRI;
            }
        }
        pfd->revents = revents & pfd->events;
    }
}

static int os_host_main_loop_wait(int64_t timeout)
{
    GMainContext *context = g_main_context_default();
    GPollFD poll_fds[256]; /* Windows can't wait on more than 64 handles anyway */
    int poll_ret = 0;
    int g_poll_ret, ret, i, n_poll_fds;
    PollingEntry *pe;
    WaitObjects *w = &wait_objects;
    gint poll_timeout;
    int64_t poll_timeout_ns;
    int nfds;

    g_main_context_acquire(context);

    /* XXX: need to suppress polling by better using win32 events */
    ret = 0;
    for (pe = first_polling_entry; pe != NULL; pe = pe->next) {
        ret |= pe->func(pe->opaque);
    }
    if (ret != 0) {
        g_main_context_release(context);
        return ret;
    }

    WSAPOLLFD fds[128];
    int polled_count = 0;
    nfds = pollfds_fill(gpollfds, fds, ARRAY_SIZE(fds), &polled_count);
    if (polled_count > 0) {
        qemu_mutex_unlock_iothread();
        poll_ret = WSAPoll(fds, nfds, 0);
        qemu_mutex_lock_iothread();
        if (poll_ret != 0) {
            timeout = 0;
        }
        if (poll_ret > 0) {
            pollfds_poll(gpollfds, fds, nfds);
        }
    }

    g_main_context_prepare(context, &max_priority);
    n_poll_fds = g_main_context_query(context, max_priority, &poll_timeout,
                                      poll_fds, ARRAY_SIZE(poll_fds));
    g_assert(n_poll_fds <= ARRAY_SIZE(poll_fds));

    for (i = 0; i < w->num; i++) {
        poll_fds[n_poll_fds + i].fd = (DWORD_PTR)w->events[i];
        poll_fds[n_poll_fds + i].events = G_IO_IN;
    }

    if (poll_timeout < 0) {
        poll_timeout_ns = -1;
    } else {
        poll_timeout_ns = (int64_t)poll_timeout * (int64_t)SCALE_MS;
    }

    poll_timeout_ns = qemu_soonest_timeout(poll_timeout_ns, timeout);

    qemu_mutex_unlock_iothread();

    replay_mutex_unlock();

    g_poll_ret = qemu_poll_ns(poll_fds, n_poll_fds + w->num, poll_timeout_ns);

    replay_mutex_lock();

    qemu_mutex_lock_iothread();
    if (g_poll_ret > 0) {
        for (i = 0; i < w->num; i++) {
            w->revents[i] = poll_fds[n_poll_fds + i].revents;
        }
        for (i = 0; i < w->num; i++) {
            if (w->revents[i] && w->func[i]) {
                w->func[i](w->opaque[i]);
            }
        }
    }

    if (g_main_context_check(context, max_priority, poll_fds, n_poll_fds)) {
        g_main_context_dispatch(context);
    }

    g_main_context_release(context);

    return poll_ret || g_poll_ret;
}
#endif

void main_loop_wait(int nonblocking)
{
    int ret;
    uint32_t timeout = UINT32_MAX;
    int64_t timeout_ns;

    if (nonblocking) {
        timeout = 0;
    }

    /* poll any events */
    g_array_set_size(gpollfds, 0); /* reset for new iteration */
    /* XXX: separate device handlers from system ones */
    slirp_pollfds_fill(gpollfds, &timeout);

    if (timeout == UINT32_MAX) {
        timeout_ns = -1;
    } else {
        timeout_ns = (uint64_t)timeout * (int64_t)(SCALE_MS);
    }

    timeout_ns = qemu_soonest_timeout(timeout_ns,
                                      timerlistgroup_deadline_ns(
                                          &main_loop_tlg));

    ret = os_host_main_loop_wait(timeout_ns);

#if DEBUG_MAIN_LOOP_PERF
    static int iter_count = 0;
    static struct timespec lastreport;
    if (++iter_count == 3000) {
        iter_count = 0;
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        if (lastreport.tv_sec != 0) {
            long long diff = (now.tv_sec - lastreport.tv_sec)*1000000000ll +
                             now.tv_nsec - lastreport.tv_nsec;
            printf("%s: 3k iterations in %06.04f ms\n", __func__, diff / 1000000.0);
        }
        lastreport = now;
    }
#endif

#ifdef CONFIG_SLIRP
    slirp_pollfds_poll(gpollfds, (ret < 0));
#endif

    if (main_loop_poll_callback)
        (*main_loop_poll_callback)();

    /* CPU thread can infinitely wait for event after
       missing the warp */
    qemu_start_warp_timer();
    qemu_clock_run_all_timers();
}

/* Functions to operate on the main QEMU AioContext.  */

QEMUBH *qemu_bh_new(QEMUBHFunc *cb, void *opaque)
{
    return aio_bh_new(qemu_aio_context, cb, opaque);
}

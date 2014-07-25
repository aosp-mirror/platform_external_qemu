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

#include "android/charpipe.h"
#include "android/log-rotate.h"
#include "android/snaphost-android.h"
#include "block/aio.h"
#include "exec/hax.h"
#include "hw/hw.h"
#include "monitor/monitor.h"
#include "net/net.h"
#include "qemu-common.h"
#include "qemu/sockets.h"
#include "qemu/timer.h"
#include "slirp-android/libslirp.h"
#include "sysemu/cpus.h"
#include "sysemu/sysemu.h"

#ifdef __linux__
#include <sys/ioctl.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <mmsystem.h>
#endif

int qemu_calculate_timeout(void);

#ifndef CONFIG_ANDROID
/* Conversion factor from emulated instructions to virtual clock ticks.  */
int icount_time_shift;
/* Arbitrarily pick 1MIPS as the minimum allowable speed.  */
#define MAX_ICOUNT_SHIFT 10
/* Compensate for varying guest execution speed.  */
int64_t qemu_icount_bias;
static QEMUTimer *icount_rt_timer;
static QEMUTimer *icount_vm_timer;
#endif  // !CONFIG_ANDROID

#ifndef _WIN32
static int io_thread_fd = -1;

static void qemu_event_read(void *opaque)
{
    int fd = (unsigned long)opaque;
    ssize_t len;

    /* Drain the notify pipe */
    do {
        char buffer[512];
        len = read(fd, buffer, sizeof(buffer));
    } while ((len == -1 && errno == EINTR) || len > 0);
}

static int qemu_main_loop_event_init(void)
{
    int err;
    int fds[2];

    err = pipe(fds);
    if (err == -1)
        return -errno;

    err = fcntl_setfl(fds[0], O_NONBLOCK);
    if (err < 0)
        goto fail;

    err = fcntl_setfl(fds[1], O_NONBLOCK);
    if (err < 0)
        goto fail;

    qemu_set_fd_handler2(fds[0], NULL, qemu_event_read, NULL,
                         (void *)(unsigned long)fds[0]);

    io_thread_fd = fds[1];
    return 0;

fail:
    close(fds[0]);
    close(fds[1]);
    return err;
}
#else
HANDLE qemu_event_handle;

static void dummy_event_handler(void *opaque)
{
}

static int qemu_main_loop_event_init(void)
{
    qemu_event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!qemu_event_handle) {
        perror("Failed CreateEvent");
        return -1;
    }
    qemu_add_wait_object(qemu_event_handle, dummy_event_handler, NULL);
    return 0;
}
#endif

int qemu_init_main_loop(void)
{
    return qemu_main_loop_event_init();
}

#ifndef _WIN32

static inline void os_host_main_loop_wait(int *timeout)
{
}

#else  // _WIN32

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
    HANDLE events[MAXIMUM_WAIT_OBJECTS + 1];
    WaitObjectFunc *func[MAXIMUM_WAIT_OBJECTS + 1];
    void *opaque[MAXIMUM_WAIT_OBJECTS + 1];
} WaitObjects;

static WaitObjects wait_objects = {0};

int qemu_add_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque)
{
    WaitObjects *w = &wait_objects;

    if (w->num >= MAXIMUM_WAIT_OBJECTS)
        return -1;
    w->events[w->num] = handle;
    w->func[w->num] = func;
    w->opaque[w->num] = opaque;
    w->num++;
    return 0;
}

void qemu_del_wait_object(HANDLE handle, WaitObjectFunc *func, void *opaque)
{
    int i, found;
    WaitObjects *w = &wait_objects;

    found = 0;
    for (i = 0; i < w->num; i++) {
        if (w->events[i] == handle)
            found = 1;
        if (found) {
            w->events[i] = w->events[i + 1];
            w->func[i] = w->func[i + 1];
            w->opaque[i] = w->opaque[i + 1];
        }
    }
    if (found)
        w->num--;
}

void os_host_main_loop_wait(int *timeout)
{
    int ret, ret2, i;
    PollingEntry *pe;

    /* XXX: need to suppress polling by better using win32 events */
    ret = 0;
    for(pe = first_polling_entry; pe != NULL; pe = pe->next) {
        ret |= pe->func(pe->opaque);
    }
    if (ret == 0) {
        int err;
        WaitObjects *w = &wait_objects;

        qemu_mutex_unlock_iothread();
        ret = WaitForMultipleObjects(w->num, w->events, FALSE, *timeout);
        qemu_mutex_lock_iothread();
        if (WAIT_OBJECT_0 + 0 <= ret && ret <= WAIT_OBJECT_0 + w->num - 1) {
            if (w->func[ret - WAIT_OBJECT_0])
                w->func[ret - WAIT_OBJECT_0](w->opaque[ret - WAIT_OBJECT_0]);

            /* Check for additional signaled events */
            for(i = (ret - WAIT_OBJECT_0 + 1); i < w->num; i++) {

                /* Check if event is signaled */
                ret2 = WaitForSingleObject(w->events[i], 0);
                if(ret2 == WAIT_OBJECT_0) {
                    if (w->func[i])
                        w->func[i](w->opaque[i]);
                } else if (ret2 == WAIT_TIMEOUT) {
                } else {
                    err = GetLastError();
                    fprintf(stderr, "WaitForSingleObject error %d %d\n", i, err);
                }
            }
        } else if (ret == WAIT_TIMEOUT) {
        } else {
            err = GetLastError();
            fprintf(stderr, "WaitForMultipleObjects error %d %d\n", ret, err);
        }
    }

    *timeout = 0;
}

#endif  // _WIN32

static void qemu_run_alarm_timer(void);  // forward

void main_loop_wait(int timeout)
{
    fd_set rfds, wfds, xfds;
    int ret, nfds;
    struct timeval tv;

    qemu_bh_update_timeout(&timeout);

    os_host_main_loop_wait(&timeout);

    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    /* poll any events */

    /* XXX: separate device handlers from system ones */
    nfds = -1;
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&xfds);
    qemu_iohandler_fill(&nfds, &rfds, &wfds, &xfds);
    if (slirp_is_inited()) {
        slirp_select_fill(&nfds, &rfds, &wfds, &xfds);
    }

    qemu_mutex_unlock_iothread();
    ret = select(nfds + 1, &rfds, &wfds, &xfds, &tv);
    qemu_mutex_lock_iothread();
    qemu_iohandler_poll(&rfds, &wfds, &xfds, ret);
    if (slirp_is_inited()) {
        if (ret < 0) {
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            FD_ZERO(&xfds);
        }
        slirp_select_poll(&rfds, &wfds, &xfds);
    }
    charpipe_poll();

    qemu_clock_run_all_timers();

    qemu_run_alarm_timer();

    /* Check bottom-halves last in case any of the earlier events triggered
       them.  */
    qemu_bh_poll();

}

void main_loop(void)
{
    int r;

#ifdef CONFIG_HAX
    if (hax_enabled())
        hax_sync_vcpus();
#endif

    for (;;) {
        do {
#ifdef CONFIG_PROFILER
            int64_t ti;
#endif
            tcg_cpu_exec();
#ifdef CONFIG_PROFILER
            ti = profile_getclock();
#endif
            main_loop_wait(qemu_calculate_timeout());
#ifdef CONFIG_PROFILER
            dev_time += profile_getclock() - ti;
#endif

            qemu_log_rotation_poll();

        } while (vm_can_run());

        if (qemu_debug_requested())
            vm_stop(EXCP_DEBUG);
        if (qemu_shutdown_requested()) {
            if (no_shutdown) {
                vm_stop(0);
                no_shutdown = 0;
            } else {
                if (savevm_on_exit != NULL) {
                  /* Prior to saving VM to the snapshot file, save HW config
                   * settings for that VM, so we can match them when VM gets
                   * loaded from the snapshot. */
                  snaphost_save_config(savevm_on_exit);
                  do_savevm(cur_mon, savevm_on_exit);
                }
                break;
            }
        }
        if (qemu_reset_requested()) {
            pause_all_vcpus();
            qemu_system_reset();
            resume_all_vcpus();
        }
        if (qemu_powerdown_requested())
            qemu_system_powerdown();
        if ((r = qemu_vmstop_requested()))
            vm_stop(r);
    }
    pause_all_vcpus();
}

// TODO(digit): Re-enable icount handling int he future.
void configure_icount(const char* opts) {
}

struct qemu_alarm_timer {
    char const *name;
    int (*start)(struct qemu_alarm_timer *t);
    void (*stop)(struct qemu_alarm_timer *t);
    void (*rearm)(struct qemu_alarm_timer *t);
#if defined(__linux__)
    int fd;
    timer_t timer;
#elif defined(_WIN32)
    HANDLE timer;
#endif
    char expired;
};

static struct qemu_alarm_timer *alarm_timer;

static inline int alarm_has_dynticks(struct qemu_alarm_timer *t)
{
    return t->rearm != NULL;
}

static void qemu_rearm_alarm_timer(struct qemu_alarm_timer *t)
{
    if (t->rearm) {
        t->rearm(t);
    }
}

static void qemu_run_alarm_timer(void) {
    /* rearm timer, if not periodic */
    if (alarm_timer->expired) {
        alarm_timer->expired = 0;
        qemu_rearm_alarm_timer(alarm_timer);
    }
}

/* TODO: MIN_TIMER_REARM_NS should be optimized */
#define MIN_TIMER_REARM_NS 250000

#ifdef _WIN32

static int mm_start_timer(struct qemu_alarm_timer *t);
static void mm_stop_timer(struct qemu_alarm_timer *t);
static void mm_rearm_timer(struct qemu_alarm_timer *t);

static int win32_start_timer(struct qemu_alarm_timer *t);
static void win32_stop_timer(struct qemu_alarm_timer *t);
static void win32_rearm_timer(struct qemu_alarm_timer *t);

#else

static int unix_start_timer(struct qemu_alarm_timer *t);
static void unix_stop_timer(struct qemu_alarm_timer *t);

#ifdef __linux__

static int dynticks_start_timer(struct qemu_alarm_timer *t);
static void dynticks_stop_timer(struct qemu_alarm_timer *t);
static void dynticks_rearm_timer(struct qemu_alarm_timer *t);

#endif /* __linux__ */

#endif /* _WIN32 */

int64_t qemu_icount_round(int64_t count)
{
    return (count + (1 << icount_time_shift) - 1) >> icount_time_shift;
}

static struct qemu_alarm_timer alarm_timers[] = {
#ifndef _WIN32
    {"unix", unix_start_timer, unix_stop_timer, NULL},
#ifdef __linux__
    /* on Linux, the 'dynticks' clock sometimes doesn't work
     * properly. this results in the UI freezing while emulation
     * continues, for several seconds... So move it to the end
     * of the list. */
    {"dynticks", dynticks_start_timer,
     dynticks_stop_timer, dynticks_rearm_timer},
#endif
#else
    {"mmtimer", mm_start_timer, mm_stop_timer, NULL},
    {"mmtimer2", mm_start_timer, mm_stop_timer, mm_rearm_timer},
    {"dynticks", win32_start_timer, win32_stop_timer, win32_rearm_timer},
    {"win32", win32_start_timer, win32_stop_timer, NULL},
#endif
    {NULL, }
};

static void show_available_alarms(void)
{
    int i;

    printf("Available alarm timers, in order of precedence:\n");
    for (i = 0; alarm_timers[i].name; i++)
        printf("%s\n", alarm_timers[i].name);
}

void configure_alarms(char const *opt)
{
    int i;
    int cur = 0;
    int count = ARRAY_SIZE(alarm_timers) - 1;
    char *arg;
    char *name;
    struct qemu_alarm_timer tmp;

    if (!strcmp(opt, "?")) {
        show_available_alarms();
        exit(0);
    }

    arg = g_strdup(opt);

    /* Reorder the array */
    name = strtok(arg, ",");
    while (name) {
        for (i = 0; i < count && alarm_timers[i].name; i++) {
            if (!strcmp(alarm_timers[i].name, name))
                break;
        }

        if (i == count) {
            fprintf(stderr, "Unknown clock %s\n", name);
            goto next;
        }

        if (i < cur)
            /* Ignore */
            goto next;

        /* Swap */
        tmp = alarm_timers[i];
        alarm_timers[i] = alarm_timers[cur];
        alarm_timers[cur] = tmp;

        cur++;
next:
        name = strtok(NULL, ",");
    }

    g_free(arg);

    if (cur) {
        /* Disable remaining timers */
        for (i = cur; i < count; i++)
            alarm_timers[i].name = NULL;
    } else {
        show_available_alarms();
        exit(1);
    }
}

// This variable is used to notify the qemu_timer_alarm_pending() caller
// (really tcg_cpu_exec()) that an alarm has expired. It is set in the
// timer callback, which can be a signal handler on non-Windows platforms.
static volatile sig_atomic_t timer_alarm_pending = 1;

int qemu_timer_alarm_pending(void)
{
    int ret = timer_alarm_pending;
    timer_alarm_pending = 0;
    return ret;
}

// Compute the next alarm deadline, return a timeout in nanoseconds.
// NOTE: This function cannot be called from a signal handler since
// it calls qemu-timer.c functions that acquire/release global mutexes.
static int64_t qemu_next_alarm_deadline(void)
{
    int64_t delta = INT32_MAX;
    if (!use_icount) {
        delta = qemu_clock_deadline_ns_all(QEMU_CLOCK_VIRTUAL);
    }
    int64_t hdelta = qemu_clock_deadline_ns_all(QEMU_CLOCK_HOST);
    if (hdelta < delta) {
        delta = hdelta;
    }
    int64_t rtdelta = qemu_clock_deadline_ns_all(QEMU_CLOCK_REALTIME);
    if (rtdelta < delta) {
        delta = rtdelta;
    }
    return delta;
}

#ifdef _WIN32
static void CALLBACK host_alarm_handler(PVOID lpParam, BOOLEAN unused)
#else
static void host_alarm_handler(int host_signum)
#endif
{
    struct qemu_alarm_timer *t = alarm_timer;
    if (!t)
        return;

    // It's not possible to call qemu_next_alarm_deadline() to know
    // if a timer has really expired, in the case of non-dynamic alarms,
    // so just signal and let the main loop thread do the checks instead.
    timer_alarm_pending = 1;

    // Ensure a dynamic alarm will be properly rescheduled.
    if (alarm_has_dynticks(t))
        t->expired = 1;

    // This forces a cpu_exit() call that will end the current CPU
    // execution ASAP.
    qemu_notify_event();
}

#if defined(__linux__)

static int dynticks_start_timer(struct qemu_alarm_timer *t)
{
    struct sigevent ev;
    timer_t host_timer;
    struct sigaction act;

    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = host_alarm_handler;

    sigaction(SIGALRM, &act, NULL);

    /*
     * Initialize ev struct to 0 to avoid valgrind complaining
     * about uninitialized data in timer_create call
     */
    memset(&ev, 0, sizeof(ev));
    ev.sigev_value.sival_int = 0;
    ev.sigev_notify = SIGEV_SIGNAL;
    ev.sigev_signo = SIGALRM;

    if (timer_create(CLOCK_REALTIME, &ev, &host_timer)) {
        perror("timer_create");

        /* disable dynticks */
        fprintf(stderr, "Dynamic Ticks disabled\n");

        return -1;
    }

    t->timer = host_timer;

    return 0;
}

static void dynticks_stop_timer(struct qemu_alarm_timer *t)
{
    timer_t host_timer = t->timer;

    timer_delete(host_timer);
}

static void dynticks_rearm_timer(struct qemu_alarm_timer *t)
{
    timer_t host_timer = t->timer;
    struct itimerspec timeout;
    int64_t nearest_delta_ns = INT64_MAX;
    int64_t current_ns;

    assert(alarm_has_dynticks(t));
    if (!qemu_clock_has_timers(QEMU_CLOCK_REALTIME) &&
        !qemu_clock_has_timers(QEMU_CLOCK_VIRTUAL) &&
        !qemu_clock_has_timers(QEMU_CLOCK_HOST))
        return;

    nearest_delta_ns = qemu_next_alarm_deadline();
    if (nearest_delta_ns < MIN_TIMER_REARM_NS)
        nearest_delta_ns = MIN_TIMER_REARM_NS;

    /* check whether a timer is already running */
    if (timer_gettime(host_timer, &timeout)) {
        perror("gettime");
        fprintf(stderr, "Internal timer error: aborting\n");
        exit(1);
    }
    current_ns = timeout.it_value.tv_sec * 1000000000LL + timeout.it_value.tv_nsec;
    if (current_ns && current_ns <= nearest_delta_ns)
        return;

    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_nsec = 0; /* 0 for one-shot timer */
    timeout.it_value.tv_sec =  nearest_delta_ns / 1000000000;
    timeout.it_value.tv_nsec = nearest_delta_ns % 1000000000;
    if (timer_settime(host_timer, 0 /* RELATIVE */, &timeout, NULL)) {
        perror("settime");
        fprintf(stderr, "Internal timer error: aborting\n");
        exit(1);
    }
}

#endif /* defined(__linux__) */

#if !defined(_WIN32)

static int unix_start_timer(struct qemu_alarm_timer *t)
{
    struct sigaction act;
    struct itimerval itv;
    int err;

    /* timer signal */
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = host_alarm_handler;

    sigaction(SIGALRM, &act, NULL);

    itv.it_interval.tv_sec = 0;
    /* for i386 kernel 2.6 to get 1 ms */
    itv.it_interval.tv_usec = 999;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 10 * 1000;

    err = setitimer(ITIMER_REAL, &itv, NULL);
    if (err)
        return -1;

    return 0;
}

static void unix_stop_timer(struct qemu_alarm_timer *t)
{
    struct itimerval itv;

    memset(&itv, 0, sizeof(itv));
    setitimer(ITIMER_REAL, &itv, NULL);
}

#endif /* !defined(_WIN32) */


#ifdef _WIN32

static MMRESULT mm_timer;
static unsigned mm_period;

static void CALLBACK mm_alarm_handler(UINT uTimerID, UINT uMsg,
                                      DWORD_PTR dwUser, DWORD_PTR dw1,
                                      DWORD_PTR dw2)
{
    struct qemu_alarm_timer *t = alarm_timer;
    if (!t) {
        return;
    }
    // We can actually call qemu_next_alarm_deadline() here since this
    // doesn't run in a signal handler, but a different thread.
    if (alarm_has_dynticks(t) || qemu_next_alarm_deadline() <= 0) {
        t->expired = 1;
        timer_alarm_pending = 1;
        qemu_notify_event();
    }
}

static int mm_start_timer(struct qemu_alarm_timer *t)
{
    TIMECAPS tc;
    UINT flags;

    memset(&tc, 0, sizeof(tc));
    timeGetDevCaps(&tc, sizeof(tc));

    mm_period = tc.wPeriodMin;
    timeBeginPeriod(mm_period);

    flags = TIME_CALLBACK_FUNCTION;
    if (alarm_has_dynticks(t)) {
        flags |= TIME_ONESHOT;
    } else {
        flags |= TIME_PERIODIC;
    }

    mm_timer = timeSetEvent(1,                  /* interval (ms) */
                            mm_period,          /* resolution */
                            mm_alarm_handler,   /* function */
                            (DWORD_PTR)t,       /* parameter */
                        flags);

    if (!mm_timer) {
        fprintf(stderr, "Failed to initialize win32 alarm timer: %ld\n",
                GetLastError());
        timeEndPeriod(mm_period);
        return -1;
    }

    return 0;
}

static void mm_stop_timer(struct qemu_alarm_timer *t)
{
    timeKillEvent(mm_timer);
    timeEndPeriod(mm_period);
}

static void mm_rearm_timer(struct qemu_alarm_timer *t)
{
    int nearest_delta_ms;

    assert(alarm_has_dynticks(t));
    if (!qemu_clock_has_timers(QEMU_CLOCK_REALTIME) &&
        !qemu_clock_has_timers(QEMU_CLOCK_VIRTUAL) &&
        !qemu_clock_has_timers(QEMU_CLOCK_HOST)) {
        return;
    }

    timeKillEvent(mm_timer);

    nearest_delta_ms = (qemu_next_alarm_deadline() + 999999) / 1000000;
    if (nearest_delta_ms < 1) {
        nearest_delta_ms = 1;
    }
    mm_timer = timeSetEvent(nearest_delta_ms,
                            mm_period,
                            mm_alarm_handler,
                            (DWORD_PTR)t,
                            TIME_ONESHOT | TIME_CALLBACK_FUNCTION);

    if (!mm_timer) {
        fprintf(stderr, "Failed to re-arm win32 alarm timer %ld\n",
                GetLastError());

        timeEndPeriod(mm_period);
        exit(1);
    }
}

static int win32_start_timer(struct qemu_alarm_timer *t)
{
    HANDLE hTimer;
    BOOLEAN success;

    /* If you call ChangeTimerQueueTimer on a one-shot timer (its period
       is zero) that has already expired, the timer is not updated.  Since
       creating a new timer is relatively expensive, set a bogus one-hour
       interval in the dynticks case.  */
    success = CreateTimerQueueTimer(&hTimer,
                          NULL,
                          host_alarm_handler,
                          t,
                          1,
                          alarm_has_dynticks(t) ? 3600000 : 1,
                          WT_EXECUTEINTIMERTHREAD);

    if (!success) {
        fprintf(stderr, "Failed to initialize win32 alarm timer: %ld\n",
                GetLastError());
        return -1;
    }

    t->timer = hTimer;
    return 0;
}

static void win32_stop_timer(struct qemu_alarm_timer *t)
{
    HANDLE hTimer = t->timer;

    if (hTimer) {
        DeleteTimerQueueTimer(NULL, hTimer, NULL);
    }
}

static void win32_rearm_timer(struct qemu_alarm_timer *t)
{
    HANDLE hTimer = t->timer;
    int nearest_delta_ms;
    BOOLEAN success;

    assert(alarm_has_dynticks(t));
    if (!qemu_clock_has_timers(QEMU_CLOCK_REALTIME) &&
        !qemu_clock_has_timers(QEMU_CLOCK_VIRTUAL) &&
        !qemu_clock_has_timers(QEMU_CLOCK_HOST)) {
        return;
    }
    nearest_delta_ms = (qemu_next_alarm_deadline() + 999999) / 1000000;
    if (nearest_delta_ms < 1) {
        nearest_delta_ms = 1;
    }
    success = ChangeTimerQueueTimer(NULL,
                                    hTimer,
                                    nearest_delta_ms,
                                    3600000);

    if (!success) {
        fprintf(stderr, "Failed to rearm win32 alarm timer: %ld\n",
                GetLastError());
        exit(-1);
    }
}

#endif /* _WIN32 */

static void alarm_timer_on_change_state_rearm(void *opaque,
                                              int running,
                                              int reason)
{
    if (running)
        qemu_rearm_alarm_timer((struct qemu_alarm_timer *) opaque);
}

int init_timer_alarm(void)
{
    struct qemu_alarm_timer *t = NULL;
    int i, err = -1;

    for (i = 0; alarm_timers[i].name; i++) {
        t = &alarm_timers[i];

        err = t->start(t);
        if (!err)
            break;
    }

    if (err) {
        err = -ENOENT;
        goto fail;
    }

    /* first event is at time 0 */
    alarm_timer = t;
    timer_alarm_pending = 1;
    qemu_add_vm_change_state_handler(alarm_timer_on_change_state_rearm, t);

    return 0;

fail:
    return err;
}

void quit_timers(void)
{
    struct qemu_alarm_timer *t = alarm_timer;
    alarm_timer = NULL;
    t->stop(t);
}

int qemu_calculate_timeout(void)
{
    int timeout;

    if (!vm_running)
        timeout = 5000;
    else if (tcg_has_work())
        timeout = 0;
    else {
#ifdef WIN32
        /* This corresponds to the case where the emulated system is
         * totally idle and waiting for i/o. The problem is that on
         * Windows, the default value will prevent Windows user events
         * to be delivered in less than 5 seconds.
         *
         * Upstream contains a different way to handle this, for now
         * this hack should be sufficient until we integrate it into
         * our tree.
         */
        timeout = 1000/15;  /* deliver user events every 15/th of second */
#else
        timeout = 5000;
#endif
        int64_t timeout_ns = (int64_t)timeout * 1000000LL;
        timeout_ns = qemu_soonest_timeout(
                timeout_ns, timerlistgroup_deadline_ns(&main_loop_tlg));
        timeout = (int)((timeout_ns + 999999LL) / 1000000LL);
    }

    return timeout;
}

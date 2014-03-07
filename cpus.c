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
#include "config-host.h"

#include "cpu.h"
#include "exec/exec-all.h"
#include "monitor/monitor.h"
#include "sysemu/sysemu.h"
#include "exec/exec-all.h"
#include "exec/gdbstub.h"
#include "sysemu/dma.h"
#include "sysemu/kvm.h"
#include "exec/exec-all.h"
#include "exec/hax.h"

#include "sysemu/cpus.h"

static CPUOldState *cur_cpu;
static CPUOldState *next_cpu;

/***********************************************************/
void hw_error(const char *fmt, ...)
{
    va_list ap;
    CPUOldState *env;

    va_start(ap, fmt);
    fprintf(stderr, "qemu: hardware error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    for(env = first_cpu; env != NULL; env = env->next_cpu) {
        fprintf(stderr, "CPU #%d:\n", env->cpu_index);
#ifdef TARGET_I386
        cpu_dump_state(env, stderr, fprintf, X86_DUMP_FPU);
#else
        cpu_dump_state(env, stderr, fprintf, 0);
#endif
    }
    va_end(ap);
    abort();
}

static void do_vm_stop(int reason)
{
    if (vm_running) {
        cpu_disable_ticks();
        vm_running = 0;
        pause_all_vcpus();
        vm_state_notify(0, reason);
    }
}

static int cpu_can_run(CPUOldState *env)
{
    if (env->stop)
        return 0;
    if (env->stopped)
        return 0;
    return 1;
}

static int cpu_has_work(CPUOldState *env)
{
    if (env->stop)
        return 1;
    if (env->stopped)
        return 0;
    if (!env->halted)
        return 1;
    if (qemu_cpu_has_work(env))
        return 1;
    return 0;
}

int tcg_has_work(void)
{
    CPUOldState *env;

    for (env = first_cpu; env != NULL; env = env->next_cpu)
        if (cpu_has_work(env))
            return 1;
    return 0;
}

#ifndef _WIN32
static int io_thread_fd = -1;

#if 0
static void qemu_event_increment(void)
{
    static const char byte = 0;

    if (io_thread_fd == -1)
        return;

    write(io_thread_fd, &byte, sizeof(byte));
}
#endif

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

static int qemu_event_init(void)
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

static int qemu_event_init(void)
{
    qemu_event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!qemu_event_handle) {
        perror("Failed CreateEvent");
        return -1;
    }
    qemu_add_wait_object(qemu_event_handle, dummy_event_handler, NULL);
    return 0;
}

#if 0
static void qemu_event_increment(void)
{
    SetEvent(qemu_event_handle);
}
#endif
#endif

int qemu_init_main_loop(void)
{
    return qemu_event_init();
}

void qemu_init_vcpu(void *_env)
{
    CPUOldState *env = _env;

    if (kvm_enabled())
        kvm_init_vcpu(env);
#ifdef CONFIG_HAX
    if (hax_enabled())
        hax_init_vcpu(env);
#endif
    return;
}

int qemu_cpu_self(void *env)
{
    return 1;
}

void resume_all_vcpus(void)
{
}

void pause_all_vcpus(void)
{
}

void qemu_cpu_kick(void *env)
{
    return;
}

void qemu_notify_event(void)
{
    CPUOldState *env = cpu_single_env;

    if (env) {
        cpu_exit(env);
    /*
     * This is mainly for the Windows host, where the timer may be in
     * a different thread with vcpu. Thus the timer function needs to
     * notify the vcpu thread of more than simply cpu_exit.  If env is
     * not NULL, it means that the vcpu is in execute state, we need
     * only to set the flags.  If the guest is in execute state, the
     * HAX kernel module will exit to qemu.  If env is NULL, vcpu is
     * in main_loop_wait, and we need a event to notify it.
     */
#ifdef CONFIG_HAX
        if (hax_enabled())
            hax_raise_event(env);
     } else {
#ifdef _WIN32
         if(hax_enabled())
             SetEvent(qemu_event_handle);
#endif
     }
#else
     }
#endif
}

void qemu_mutex_lock_iothread(void)
{
}

void qemu_mutex_unlock_iothread(void)
{
}

void vm_stop(int reason)
{
    do_vm_stop(reason);
}

static int qemu_cpu_exec(CPUOldState *env)
{
    int ret;
#ifdef CONFIG_PROFILER
    int64_t ti;
#endif

#ifdef CONFIG_PROFILER
    ti = profile_getclock();
#endif
    if (use_icount) {
        int64_t count;
        int decr;
        qemu_icount -= (env->icount_decr.u16.low + env->icount_extra);
        env->icount_decr.u16.low = 0;
        env->icount_extra = 0;
        count = qemu_next_icount_deadline();
        count = (count + (1 << icount_time_shift) - 1)
                >> icount_time_shift;
        qemu_icount += count;
        decr = (count > 0xffff) ? 0xffff : count;
        count -= decr;
        env->icount_decr.u16.low = decr;
        env->icount_extra = count;
    }

    ret = cpu_exec(env);
#ifdef CONFIG_PROFILER
    qemu_time += profile_getclock() - ti;
#endif
    if (use_icount) {
        /* Fold pending instructions back into the
           instruction counter, and clear the interrupt flag.  */
        qemu_icount -= (env->icount_decr.u16.low
                        + env->icount_extra);
        env->icount_decr.u32 = 0;
        env->icount_extra = 0;
    }
    return ret;
}

void tcg_cpu_exec(void)
{
    int ret = 0;

    if (next_cpu == NULL)
        next_cpu = first_cpu;
    for (; next_cpu != NULL; next_cpu = next_cpu->next_cpu) {
        CPUOldState *env = cur_cpu = next_cpu;

        if (!vm_running)
            break;
        if (qemu_timer_alarm_pending()) {
            break;
        }
        if (cpu_can_run(env))
            ret = qemu_cpu_exec(env);
        if (ret == EXCP_DEBUG) {
            gdb_set_stop_cpu(env);
            debug_requested = 1;
            break;
        }
    }
}

/* Return the virtual CPU time, based on the instruction counter.  */
int64_t cpu_get_icount(void)
{
    int64_t icount;
    CPUOldState *env = cpu_single_env;;

    icount = qemu_icount;
    if (env) {
        if (!can_do_io(env)) {
            fprintf(stderr, "Bad clock read\n");
        }
        icount -= (env->icount_decr.u16.low + env->icount_extra);
    }
    return qemu_icount_bias + (icount << icount_time_shift);
}

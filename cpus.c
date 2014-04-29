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
#include "monitor/monitor.h"
#include "sysemu/sysemu.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/gdbstub.h"
#include "sysemu/dma.h"
#include "sysemu/kvm.h"
#include "exec/exec-all.h"
#include "exec/hax.h"

#include "sysemu/cpus.h"

static CPUState *cur_cpu;
static CPUState *next_cpu;

/***********************************************************/
void hw_error(const char *fmt, ...)
{
    va_list ap;
    CPUState *cpu;

    va_start(ap, fmt);
    fprintf(stderr, "qemu: hardware error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    CPU_FOREACH(cpu) {
        fprintf(stderr, "CPU #%d:\n", cpu->cpu_index);
#ifdef TARGET_I386
        cpu_dump_state(cpu->env_ptr, stderr, fprintf, X86_DUMP_FPU);
#else
        cpu_dump_state(cpu->env_ptr, stderr, fprintf, 0);
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

static int cpu_can_run(CPUArchState *env)
{
    CPUState *cpu = ENV_GET_CPU(env);
    if (cpu->stop)
        return 0;
    if (cpu->stopped)
        return 0;
    return 1;
}

int tcg_has_work(void)
{
    CPUState *cpu;

    CPU_FOREACH(cpu) {
        if (cpu->stop)
            return 1;
        if (cpu->stopped)
            return 0;
        if (!cpu->halted)
            return 1;
        if (cpu_has_work(cpu))
            return 1;
        return 0;
    }
    return 0;
}

void qemu_init_vcpu(CPUState *cpu)
{
    if (kvm_enabled())
        kvm_init_vcpu(cpu);
#ifdef CONFIG_HAX
    if (hax_enabled())
        hax_init_vcpu(cpu);
#endif
    return;
}

bool qemu_cpu_is_self(CPUState *cpu)
{
    return true;
}

void resume_all_vcpus(void)
{
}

void pause_all_vcpus(void)
{
}

void qemu_cpu_kick(CPUState *cpu)
{
    return;
}

// In main-loop.c
#ifdef _WIN32
extern HANDLE qemu_event_handle;
#endif

void qemu_notify_event(void)
{
    CPUState *cpu = current_cpu;

    if (cpu) {
        cpu_exit(cpu);
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
            hax_raise_event(cpu);
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
    int64_t ti = profile_getclock();
#endif
#ifndef CONFIG_ANDROID
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
#endif
    ret = cpu_exec(env);
#ifdef CONFIG_PROFILER
    qemu_time += profile_getclock() - ti;
#endif
#ifndef CONFIG_ANDROID
    if (use_icount) {
        /* Fold pending instructions back into the
           instruction counter, and clear the interrupt flag.  */
        qemu_icount -= (env->icount_decr.u16.low
                        + env->icount_extra);
        env->icount_decr.u32 = 0;
        env->icount_extra = 0;
    }
#endif
    return ret;
}

void tcg_cpu_exec(void)
{
    int ret = 0;

    if (next_cpu == NULL)
        next_cpu = QTAILQ_FIRST(&cpus);
    for (; next_cpu != NULL; next_cpu = QTAILQ_NEXT(next_cpu, node)) {\
        cur_cpu = next_cpu;
        CPUOldState *env = cur_cpu->env_ptr;

        if (!vm_running)
            break;
        if (qemu_timer_alarm_pending()) {
            break;
        }
        if (cpu_can_run(env))
            ret = qemu_cpu_exec(env);
        if (ret == EXCP_DEBUG) {
            gdb_set_stop_cpu(cur_cpu);
            debug_requested = 1;
            break;
        }
    }
}

/***********************************************************/
/* guest cycle counter */

typedef struct TimersState {
    int64_t cpu_ticks_prev;
    int64_t cpu_ticks_offset;
    int64_t cpu_clock_offset;
    int32_t cpu_ticks_enabled;
    int64_t dummy;
} TimersState;

static void timer_save(QEMUFile *f, void *opaque)
{
    TimersState *s = opaque;

    if (s->cpu_ticks_enabled) {
        hw_error("cannot save state if virtual timers are running");
    }
    qemu_put_be64(f, s->cpu_ticks_prev);
    qemu_put_be64(f, s->cpu_ticks_offset);
    qemu_put_be64(f, s->cpu_clock_offset);
 }

static int timer_load(QEMUFile *f, void *opaque, int version_id)
{
    TimersState *s = opaque;

    if (version_id != 1 && version_id != 2)
        return -EINVAL;
    if (s->cpu_ticks_enabled) {
        return -EINVAL;
    }
    s->cpu_ticks_prev   = qemu_get_sbe64(f);
    s->cpu_ticks_offset = qemu_get_sbe64(f);
    if (version_id == 2) {
        s->cpu_clock_offset = qemu_get_sbe64(f);
    }
    return 0;
}


TimersState timers_state;

void qemu_timer_register_savevm(void) {
    register_savevm(NULL,
                    "timer",
                    0,
                    2,
                    timer_save,
                    timer_load,
                    &timers_state);
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

/* return the host CPU cycle counter and handle stop/restart */
int64_t cpu_get_ticks(void)
{
    if (use_icount) {
        return cpu_get_icount();
    }
    if (!timers_state.cpu_ticks_enabled) {
        return timers_state.cpu_ticks_offset;
    } else {
        int64_t ticks;
        ticks = cpu_get_real_ticks();
        if (timers_state.cpu_ticks_prev > ticks) {
            /* Note: non increasing ticks may happen if the host uses
               software suspend */
            timers_state.cpu_ticks_offset += timers_state.cpu_ticks_prev - ticks;
        }
        timers_state.cpu_ticks_prev = ticks;
        return ticks + timers_state.cpu_ticks_offset;
    }
}

/* return the host CPU monotonic timer and handle stop/restart */
int64_t cpu_get_clock(void)
{
    int64_t ti;
    if (!timers_state.cpu_ticks_enabled) {
        return timers_state.cpu_clock_offset;
    } else {
        ti = get_clock();
        return ti + timers_state.cpu_clock_offset;
    }
}

/* enable cpu_get_ticks() */
void cpu_enable_ticks(void)
{
    if (!timers_state.cpu_ticks_enabled) {
        timers_state.cpu_ticks_offset -= cpu_get_real_ticks();
        timers_state.cpu_clock_offset -= get_clock();
        timers_state.cpu_ticks_enabled = 1;
    }
}

/* disable cpu_get_ticks() : the clock is stopped. You must not call
   cpu_get_ticks() after that.  */
void cpu_disable_ticks(void)
{
    if (timers_state.cpu_ticks_enabled) {
        timers_state.cpu_ticks_offset = cpu_get_ticks();
        timers_state.cpu_clock_offset = cpu_get_clock();
        timers_state.cpu_ticks_enabled = 0;
    }
}

void qemu_clock_warp(QEMUClockType clock) {
}

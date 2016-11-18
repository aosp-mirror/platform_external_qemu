#include "hw/hw.h"
#include "hw/mips/mips.h"
#include "qemu/timer.h"

#define TIMER_FREQ	100 * 1000 * 1000

/* XXX: do not use a global */
uint32_t cpu_mips_get_random (CPUOldState *env)
{
    static uint32_t lfsr = 1;
    static uint32_t prev_idx = 0;
    uint32_t idx;
    /* Don't return same value twice, so get another value */
    do {
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xd0000001u);
        idx = lfsr % (env->tlb->nb_tlb - env->CP0_Wired) + env->CP0_Wired;
    } while (idx == prev_idx);
    prev_idx = idx;
    return idx;
}

/* MIPS R4K timer */
uint32_t cpu_mips_get_count (CPUOldState *env)
{
    if (env->CP0_Cause & (1 << CP0Ca_DC))
        return env->CP0_Count;
    else
        return env->CP0_Count +
            (uint32_t)muldiv64(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL),
                               TIMER_FREQ, get_ticks_per_sec());
}

static void cpu_mips_timer_update(CPUOldState *env)
{
    uint64_t now, next;
    uint32_t wait;

    now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    wait = env->CP0_Compare - env->CP0_Count -
	    (uint32_t)muldiv64(now, TIMER_FREQ, get_ticks_per_sec());
    next = now + muldiv64(wait, get_ticks_per_sec(), TIMER_FREQ);
    timer_mod(env->timer, next);
}

void cpu_mips_store_count (CPUOldState *env, uint32_t count)
{
    if (env->CP0_Cause & (1 << CP0Ca_DC))
        env->CP0_Count = count;
    else {
        /* Store new count register */
        env->CP0_Count =
            count - (uint32_t)muldiv64(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL),
                                       TIMER_FREQ, get_ticks_per_sec());
        /* Update timer timer */
        cpu_mips_timer_update(env);
    }
}

void cpu_mips_store_compare (CPUOldState *env, uint32_t value)
{
    env->CP0_Compare = value;
    if (!(env->CP0_Cause & (1 << CP0Ca_DC)))
        cpu_mips_timer_update(env);
    if (env->insn_flags & ISA_MIPS32R2)
        env->CP0_Cause &= ~(1 << CP0Ca_TI);
    qemu_irq_lower(env->irq[(env->CP0_IntCtl >> CP0IntCtl_IPTI) & 0x7]);
}

void cpu_mips_start_count(CPUOldState *env)
{
    cpu_mips_store_count(env, env->CP0_Count);
}

void cpu_mips_stop_count(CPUOldState *env)
{
    /* Store the current value */
    env->CP0_Count += (uint32_t)muldiv64(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL),
                                         TIMER_FREQ, get_ticks_per_sec());
}

static void mips_timer_cb (void *opaque)
{
    CPUOldState *env;

    env = opaque;
#if 0
    qemu_log("%s\n", __func__);
#endif

    if (env->CP0_Cause & (1 << CP0Ca_DC))
        return;

    /* ??? This callback should occur when the counter is exactly equal to
       the comparator value.  Offset the count by one to avoid immediately
       retriggering the callback before any virtual time has passed.  */
    env->CP0_Count++;
    cpu_mips_timer_update(env);
    env->CP0_Count--;
    if (env->insn_flags & ISA_MIPS32R2)
        env->CP0_Cause |= 1 << CP0Ca_TI;
    qemu_irq_raise(env->irq[(env->CP0_IntCtl >> CP0IntCtl_IPTI) & 0x7]);
}

void cpu_mips_clock_init (CPUOldState *env)
{
    env->timer = timer_new(QEMU_CLOCK_VIRTUAL, SCALE_NS, &mips_timer_cb, env);
    env->CP0_Compare = 0;
    cpu_mips_store_count(env, 1);
}

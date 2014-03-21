/*
 *  ARM helper routines
 *
 *  Copyright (c) 2005-2007 CodeSourcery, LLC
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "cpu.h"
#include "tcg.h"
#include "helper.h"

#define SIGNBIT (uint32_t)0x80000000
#define SIGNBIT64 ((uint64_t)1 << 63)

#if !defined(CONFIG_USER_ONLY)
static void raise_exception(CPUARMState *env, int tt)
{
    env->exception_index = tt;
    cpu_loop_exit(env);
}
#endif

uint32_t HELPER(neon_tbl)(CPUARMState *env, uint32_t ireg, uint32_t def,
                          uint32_t rn, uint32_t maxindex)
{
    uint32_t val;
    uint32_t tmp;
    int index;
    int shift;
    uint64_t *table;
    table = (uint64_t *)&env->vfp.regs[rn];
    val = 0;
    for (shift = 0; shift < 32; shift += 8) {
        index = (ireg >> shift) & 0xff;
        if (index < maxindex) {
            tmp = (table[index >> 3] >> ((index & 7) << 3)) & 0xff;
            val |= tmp << shift;
        } else {
            val |= def & (0xff << shift);
        }
    }
    return val;
}

#undef env

#if !defined(CONFIG_USER_ONLY)


#define MMUSUFFIX _mmu

#define SHIFT 0
#include "exec/softmmu_template.h"

#define SHIFT 1
#include "exec/softmmu_template.h"

#define SHIFT 2
#include "exec/softmmu_template.h"

#define SHIFT 3
#include "exec/softmmu_template.h"

/* try to fill the TLB and return an exception if error. If retaddr is
   NULL, it means that the function was called in C code (i.e. not
   from generated code or from helper.c) */
void tlb_fill(CPUARMState *env, target_ulong addr, int is_write, int mmu_idx,
              uintptr_t retaddr)
{
    int ret;

    ret = cpu_arm_handle_mmu_fault(env, addr, is_write, mmu_idx);
    if (unlikely(ret)) {
        if (retaddr) {
            /* now we have a real cpu fault */
            cpu_restore_state(env, retaddr);
        }
        raise_exception(env, env->exception_index);
    }
}

void HELPER(set_cp)(CPUARMState *env, uint32_t insn, uint32_t val)
{
    int cp_num = (insn >> 8) & 0xf;
    int cp_info = (insn >> 5) & 7;
    int src = (insn >> 16) & 0xf;
    int operand = insn & 0xf;

    if (env->cp[cp_num].cp_write)
        env->cp[cp_num].cp_write(env->cp[cp_num].opaque,
                                 cp_info, src, operand, val, (void*)GETPC());
        }

uint32_t HELPER(get_cp)(CPUARMState *env, uint32_t insn)
{
    int cp_num = (insn >> 8) & 0xf;
    int cp_info = (insn >> 5) & 7;
    int dest = (insn >> 16) & 0xf;
    int operand = insn & 0xf;

    if (env->cp[cp_num].cp_read)
        return env->cp[cp_num].cp_read(env->cp[cp_num].opaque,
                                       cp_info, dest, operand, (void*)GETPC());
        return 0;
}

#else

void HELPER(set_cp)(CPUARMState *env, uint32_t insn, uint32_t val)
{
    int op1 = (insn >> 8) & 0xf;
    cpu_abort(env, "cp%i insn %08x\n", op1, insn);
    return;
}

uint32_t HELPER(get_cp)(CPUARMState *env, uint32_t insn)
{
    int op1 = (insn >> 8) & 0xf;
    cpu_abort(env, "cp%i insn %08x\n", op1, insn);
    return 0;
}

#endif

uint32_t HELPER(add_setq)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t res = a + b;
    if (((res ^ a) & SIGNBIT) && !((a ^ b) & SIGNBIT))
        env->QF = 1;
    return res;
}

uint32_t HELPER(add_saturate)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t res = a + b;
    if (((res ^ a) & SIGNBIT) && !((a ^ b) & SIGNBIT)) {
        env->QF = 1;
        res = ~(((int32_t)a >> 31) ^ SIGNBIT);
    }
    return res;
}

uint32_t HELPER(sub_saturate)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t res = a - b;
    if (((res ^ a) & SIGNBIT) && ((a ^ b) & SIGNBIT)) {
        env->QF = 1;
        res = ~(((int32_t)a >> 31) ^ SIGNBIT);
    }
    return res;
}

uint32_t HELPER(double_saturate)(CPUARMState *env, int32_t val)
{
    uint32_t res;
    if (val >= 0x40000000) {
        res = ~SIGNBIT;
        env->QF = 1;
    } else if (val <= (int32_t)0xc0000000) {
        res = SIGNBIT;
        env->QF = 1;
    } else {
        res = val << 1;
    }
    return res;
}

uint32_t HELPER(add_usaturate)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t res = a + b;
    if (res < a) {
        env->QF = 1;
        res = ~0;
    }
    return res;
}

uint32_t HELPER(sub_usaturate)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t res = a - b;
    if (res > a) {
        env->QF = 1;
        res = 0;
    }
    return res;
}

/* Signed saturation.  */
static inline uint32_t do_ssat(CPUARMState *env, int32_t val, int shift)
{
    int32_t top;
    uint32_t mask;

    top = val >> shift;
    mask = (1u << shift) - 1;
    if (top > 0) {
        env->QF = 1;
        return mask;
    } else if (top < -1) {
        env->QF = 1;
        return ~mask;
    }
    return val;
}

/* Unsigned saturation.  */
static inline uint32_t do_usat(CPUARMState *env, int32_t val, int shift)
{
    uint32_t max;

    max = (1u << shift) - 1;
    if (val < 0) {
        env->QF = 1;
        return 0;
    } else if (val > max) {
        env->QF = 1;
        return max;
    }
    return val;
}

/* Signed saturate.  */
uint32_t HELPER(ssat)(CPUARMState *env, uint32_t x, uint32_t shift)
{
    return do_ssat(env, x, shift);
}

/* Dual halfword signed saturate.  */
uint32_t HELPER(ssat16)(CPUARMState *env, uint32_t x, uint32_t shift)
{
    uint32_t res;

    res = (uint16_t)do_ssat(env, (int16_t)x, shift);
    res |= do_ssat(env, ((int32_t)x) >> 16, shift) << 16;
    return res;
}

/* Unsigned saturate.  */
uint32_t HELPER(usat)(CPUARMState *env, uint32_t x, uint32_t shift)
{
    return do_usat(env, x, shift);
}

/* Dual halfword unsigned saturate.  */
uint32_t HELPER(usat16)(CPUARMState *env, uint32_t x, uint32_t shift)
{
    uint32_t res;

    res = (uint16_t)do_usat(env, (int16_t)x, shift);
    res |= do_usat(env, ((int32_t)x) >> 16, shift) << 16;
    return res;
}

void HELPER(wfi)(CPUARMState *env)
{
    env->exception_index = EXCP_HLT;
    ENV_GET_CPU(env)->halted = 1;
    cpu_loop_exit(env);
}

void HELPER(exception)(CPUARMState *env, uint32_t excp)
{
    env->exception_index = excp;
    cpu_loop_exit(env);
}

uint32_t HELPER(cpsr_read)(CPUARMState *env)
{
    return cpsr_read(env) & ~CPSR_EXEC;
}

void HELPER(cpsr_write)(CPUARMState *env, uint32_t val, uint32_t mask)
{
    cpsr_write(env, val, mask);
}

/* Access to user mode registers from privileged modes.  */
uint32_t HELPER(get_user_reg)(CPUARMState *env, uint32_t regno)
{
    uint32_t val;

    if (regno == 13) {
        val = env->banked_r13[0];
    } else if (regno == 14) {
        val = env->banked_r14[0];
    } else if (regno >= 8
               && (env->uncached_cpsr & 0x1f) == ARM_CPU_MODE_FIQ) {
        val = env->usr_regs[regno - 8];
    } else {
        val = env->regs[regno];
    }
    return val;
}

void HELPER(set_user_reg)(CPUARMState *env, uint32_t regno, uint32_t val)
{
    if (regno == 13) {
        env->banked_r13[0] = val;
    } else if (regno == 14) {
        env->banked_r14[0] = val;
    } else if (regno >= 8
               && (env->uncached_cpsr & 0x1f) == ARM_CPU_MODE_FIQ) {
        env->usr_regs[regno - 8] = val;
    } else {
        env->regs[regno] = val;
    }
}

/* ??? Flag setting arithmetic is awkward because we need to do comparisons.
   The only way to do that in TCG is a conditional branch, which clobbers
   all our temporaries.  For now implement these as helper functions.  */

uint32_t HELPER (add_cc)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t result;
    result = a + b;
    env->NF = env->ZF = result;
    env->CF = result < a;
    env->VF = (a ^ b ^ -1) & (a ^ result);
    return result;
}

uint32_t HELPER(adc_cc)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t result;
    if (!env->CF) {
        result = a + b;
        env->CF = result < a;
    } else {
        result = a + b + 1;
        env->CF = result <= a;
    }
    env->VF = (a ^ b ^ -1) & (a ^ result);
    env->NF = env->ZF = result;
    return result;
}

uint32_t HELPER(sub_cc)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t result;
    result = a - b;
    env->NF = env->ZF = result;
    env->CF = a >= b;
    env->VF = (a ^ b) & (a ^ result);
    return result;
}

uint32_t HELPER(sbc_cc)(CPUARMState *env, uint32_t a, uint32_t b)
{
    uint32_t result;
    if (!env->CF) {
        result = a - b - 1;
        env->CF = a > b;
    } else {
        result = a - b;
        env->CF = a >= b;
    }
    env->VF = (a ^ b) & (a ^ result);
    env->NF = env->ZF = result;
    return result;
}

/* Similarly for variable shift instructions.  */

uint32_t HELPER(shl)(uint32_t x, uint32_t i)
{
    int shift = i & 0xff;
    if (shift >= 32)
        return 0;
    return x << shift;
}

uint32_t HELPER(shr)(uint32_t x, uint32_t i)
{
    int shift = i & 0xff;
    if (shift >= 32)
        return 0;
    return (uint32_t)x >> shift;
}

uint32_t HELPER(sar)(uint32_t x, uint32_t i)
{
    int shift = i & 0xff;
    if (shift >= 32)
        shift = 31;
    return (int32_t)x >> shift;
}

uint32_t HELPER(shl_cc)(CPUARMState *env, uint32_t x, uint32_t i)
{
    int shift = i & 0xff;
    if (shift >= 32) {
        if (shift == 32)
            env->CF = x & 1;
        else
            env->CF = 0;
        return 0;
    } else if (shift != 0) {
        env->CF = (x >> (32 - shift)) & 1;
        return x << shift;
    }
    return x;
}

uint32_t HELPER(shr_cc)(CPUARMState *env, uint32_t x, uint32_t i)
{
    int shift = i & 0xff;
    if (shift >= 32) {
        if (shift == 32)
            env->CF = (x >> 31) & 1;
        else
            env->CF = 0;
        return 0;
    } else if (shift != 0) {
        env->CF = (x >> (shift - 1)) & 1;
        return x >> shift;
    }
    return x;
}

uint32_t HELPER(sar_cc)(CPUARMState *env, uint32_t x, uint32_t i)
{
    int shift = i & 0xff;
    if (shift >= 32) {
        env->CF = (x >> 31) & 1;
        return (int32_t)x >> 31;
    } else if (shift != 0) {
        env->CF = (x >> (shift - 1)) & 1;
        return (int32_t)x >> shift;
    }
    return x;
}

uint32_t HELPER(ror_cc)(CPUARMState *env, uint32_t x, uint32_t i)
{
    int shift1, shift;
    shift1 = i & 0xff;
    shift = shift1 & 0x1f;
    if (shift == 0) {
        if (shift1 != 0)
            env->CF = (x >> 31) & 1;
        return x;
    } else {
        env->CF = (x >> (shift - 1)) & 1;
        return ((uint32_t)x >> shift) | (x << (32 - shift));
    }
}

void HELPER(neon_vldst_all)(CPUARMState *env, uint32_t insn)
{
#if defined(CONFIG_USER_ONLY)
#define LDB(addr) ldub(addr)
#define LDW(addr) lduw(addr)
#define LDL(addr) ldl(addr)
#define LDQ(addr) ldq(addr)
#define STB(addr, val) stb(addr, val)
#define STW(addr, val) stw(addr, val)
#define STL(addr, val) stl(addr, val)
#define STQ(addr, val) stq(addr, val)
#else
    int user = cpu_mmu_index(env);
#define LDB(addr) helper_ldb_mmu(env, addr, user)
#define LDW(addr) helper_le_lduw_mmu(env, addr, user, GETPC())
#define LDL(addr) helper_le_ldul_mmu(env, addr, user, GETPC())
#define LDQ(addr) helper_le_ldq_mmu(env, addr, user, GETPC())
#define STB(addr, val) helper_stb_mmu(env, addr, val, user)
#define STW(addr, val) helper_le_stw_mmu(env, addr, val, user, GETPC())
#define STL(addr, val) helper_le_stl_mmu(env, addr, val, user, GETPC())
#define STQ(addr, val) helper_le_stq_mmu(env, addr, val, user, GETPC())
#endif
    static const struct {
        int nregs;
        int interleave;
        int spacing;
    } neon_ls_element_type[11] = {
        {4, 4, 1},
        {4, 4, 2},
        {4, 1, 1},
        {4, 2, 1},
        {3, 3, 1},
        {3, 3, 2},
        {3, 1, 1},
        {1, 1, 1},
        {2, 2, 1},
        {2, 2, 2},
        {2, 1, 1}
    };

    const int op = (insn >> 8) & 0xf;
    const int size = (insn >> 6) & 3;
    int rd = ((insn >> 12) & 0x0f) | ((insn >> 18) & 0x10);
    const int rn = (insn >> 16) & 0xf;
    const int load = (insn & (1 << 21)) != 0;
    const int nregs = neon_ls_element_type[op].nregs;
    const int interleave = neon_ls_element_type[op].interleave;
    const int spacing = neon_ls_element_type[op].spacing;
    uint32_t addr = env->regs[rn];
    const int stride = (1 << size) * interleave;
    int i, reg;
    uint64_t tmp64;

    for (reg = 0; reg < nregs; reg++) {
        if (interleave > 2 || (interleave == 2 && nregs == 2)) {
            addr = env->regs[rn] + (1 << size) * reg;
        } else if (interleave == 2 && nregs == 4 && reg == 2) {
            addr = env->regs[rn] + (1 << size);
        }
        switch (size) {
            case 3:
                if (load) {
                    env->vfp.regs[rd] = make_float64(LDQ(addr));
                } else {
                    STQ(addr, float64_val(env->vfp.regs[rd]));
                }
                addr += stride;
                break;
            case 2:
                if (load) {
                    tmp64 = (uint32_t)LDL(addr);
                    addr += stride;
                    tmp64 |= (uint64_t)LDL(addr) << 32;
                    addr += stride;
                    env->vfp.regs[rd] = make_float64(tmp64);
                } else {
                    tmp64 = float64_val(env->vfp.regs[rd]);
                    STL(addr, tmp64);
                    addr += stride;
                    STL(addr, tmp64 >> 32);
                    addr += stride;
                }
                break;
            case 1:
                if (load) {
                    tmp64 = 0ull;
                    for (i = 0; i < 4; i++, addr += stride) {
                        tmp64 |= (uint64_t)LDW(addr) << (i * 16);
                    }
                    env->vfp.regs[rd] = make_float64(tmp64);
                } else {
                    tmp64 = float64_val(env->vfp.regs[rd]);
                    for (i = 0; i < 4; i++, addr += stride, tmp64 >>= 16) {
                        STW(addr, tmp64);
                    }
                }
                break;
            case 0:
                if (load) {
                    tmp64 = 0ull;
                    for (i = 0; i < 8; i++, addr += stride) {
                        tmp64 |= (uint64_t)LDB(addr) << (i * 8);
                    }
                    env->vfp.regs[rd] = make_float64(tmp64);
                } else {
                    tmp64 = float64_val(env->vfp.regs[rd]);
                    for (i = 0; i < 8; i++, addr += stride, tmp64 >>= 8) {
                        STB(addr, tmp64);
                    }
                }
                break;
        }
        rd += spacing;
    }
#undef LDB
#undef LDW
#undef LDL
#undef LDQ
#undef STB
#undef STW
#undef STL
#undef STQ
}

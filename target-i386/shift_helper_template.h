/*
 *  i386 helpers
 *
 *  Copyright (c) 2008 Fabrice Bellard
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA  02110-1301 USA
 */
#define DATA_BITS (1 << (3 + SHIFT))
#define SHIFT_MASK (DATA_BITS - 1)
#define SIGN_MASK (((target_ulong)1) << (DATA_BITS - 1))
#if DATA_BITS <= 32
#define SHIFT1_MASK 0x1f
#else
#define SHIFT1_MASK 0x3f
#endif

#if DATA_BITS == 8
#define SUFFIX b
#define DATA_TYPE uint8_t
#define DATA_STYPE int8_t
#define DATA_MASK 0xff
#elif DATA_BITS == 16
#define SUFFIX w
#define DATA_TYPE uint16_t
#define DATA_STYPE int16_t
#define DATA_MASK 0xffff
#elif DATA_BITS == 32
#define SUFFIX l
#define DATA_TYPE uint32_t
#define DATA_STYPE int32_t
#define DATA_MASK 0xffffffff
#elif DATA_BITS == 64
#define SUFFIX q
#define DATA_TYPE uint64_t
#define DATA_STYPE int64_t
#define DATA_MASK 0xffffffffffffffffULL
#else
#error unhandled operand size
#endif

/* shifts */

target_ulong glue(helper_rcl, SUFFIX)(CPUX86State *env,
                                      target_ulong t0, target_ulong t1)
{
    int count, eflags;
    target_ulong src;
    target_long res;

    count = t1 & SHIFT1_MASK;
#if DATA_BITS == 16
    count = rclw_table[count];
#elif DATA_BITS == 8
    count = rclb_table[count];
#endif
    if (count) {
        eflags = env->cc_src;
        t0 &= DATA_MASK;
        src = t0;
        res = (t0 << count) | ((target_ulong)(eflags & CC_C) << (count - 1));
        if (count > 1)
            res |= t0 >> (DATA_BITS + 1 - count);
        t0 = res;
        env->cc_src = (eflags & ~(CC_C | CC_O)) |
            (lshift(src ^ t0, 11 - (DATA_BITS - 1)) & CC_O) |
            ((src >> (DATA_BITS - count)) & CC_C);
    }
    return t0;
}

target_ulong glue(helper_rcr, SUFFIX)(CPUX86State *env,
                                      target_ulong t0, target_ulong t1)
{
    int count, eflags;
    target_ulong src;
    target_long res;

    count = t1 & SHIFT1_MASK;
#if DATA_BITS == 16
    count = rclw_table[count];
#elif DATA_BITS == 8
    count = rclb_table[count];
#endif
    if (count) {
        eflags = env->cc_src;
        t0 &= DATA_MASK;
        src = t0;
        res = (t0 >> count) | ((target_ulong)(eflags & CC_C) << (DATA_BITS - count));
        if (count > 1)
            res |= t0 << (DATA_BITS + 1 - count);
        t0 = res;
        env->cc_src = (eflags & ~(CC_C | CC_O)) |
            (lshift(src ^ t0, 11 - (DATA_BITS - 1)) & CC_O) |
            ((src >> (count - 1)) & CC_C);
    }
    return t0;
}

#undef DATA_BITS
#undef SHIFT_MASK
#undef SHIFT1_MASK
#undef SIGN_MASK
#undef DATA_TYPE
#undef DATA_STYPE
#undef DATA_MASK
#undef SUFFIX

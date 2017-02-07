/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001-2012  The Bochs Project
//  Copyright (C) 2017 Google Inc.
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA B 02110-1301 USA
/////////////////////////////////////////////////////////////////////////
/*
 * flags functions
 */

#include "qemu/osdep.h"
#include "qemu-common.h"

#include "cpu.h"
#include "x86_flags.h"
#include "x86.h"

void SET_FLAGS_OxxxxC(struct CPUState *cpu, uint32_t new_of, uint32_t new_cf)
{
    uint32_t temp_po = new_of ^ new_cf;
    cpu->hvf_x86->lflags.auxbits &= ~(LF_MASK_PO | LF_MASK_CF);
    cpu->hvf_x86->lflags.auxbits |= (temp_po << LF_BIT_PO) | (new_cf << LF_BIT_CF);
}

void SET_FLAGS_OSZAPC_SUB32(struct CPUState *cpu, uint32_t v1, uint32_t v2, uint32_t diff)
{
    SET_FLAGS_OSZAPC_SUB_32(v1, v2, diff);
}

void SET_FLAGS_OSZAPC_SUB16(struct CPUState *cpu, uint16_t v1, uint16_t v2, uint16_t diff)
{
    SET_FLAGS_OSZAPC_SUB_16(v1, v2, diff);
}

void SET_FLAGS_OSZAPC_SUB8(struct CPUState *cpu, uint8_t v1, uint8_t v2, uint8_t diff)
{
    SET_FLAGS_OSZAPC_SUB_8(v1, v2, diff);
}

void SET_FLAGS_OSZAPC_ADD32(struct CPUState *cpu, uint32_t v1, uint32_t v2, uint32_t diff)
{
    SET_FLAGS_OSZAPC_ADD_32(v1, v2, diff);
}

void SET_FLAGS_OSZAPC_ADD16(struct CPUState *cpu, uint16_t v1, uint16_t v2, uint16_t diff)
{
    SET_FLAGS_OSZAPC_ADD_16(v1, v2, diff);
}

void SET_FLAGS_OSZAPC_ADD8(struct CPUState *cpu, uint8_t v1, uint8_t v2, uint8_t diff)
{
    SET_FLAGS_OSZAPC_ADD_8(v1, v2, diff);
}

void SET_FLAGS_OSZAP_SUB32(struct CPUState *cpu, uint32_t v1, uint32_t v2, uint32_t diff)
{
    SET_FLAGS_OSZAP_SUB_32(v1, v2, diff);
}

void SET_FLAGS_OSZAP_SUB16(struct CPUState *cpu, uint16_t v1, uint16_t v2, uint16_t diff)
{
    SET_FLAGS_OSZAP_SUB_16(v1, v2, diff);
}

void SET_FLAGS_OSZAP_SUB8(struct CPUState *cpu, uint8_t v1, uint8_t v2, uint8_t diff)
{
    SET_FLAGS_OSZAP_SUB_8(v1, v2, diff);
}

void SET_FLAGS_OSZAP_ADD32(struct CPUState *cpu, uint32_t v1, uint32_t v2, uint32_t diff)
{
    SET_FLAGS_OSZAP_ADD_32(v1, v2, diff);
}

void SET_FLAGS_OSZAP_ADD16(struct CPUState *cpu, uint16_t v1, uint16_t v2, uint16_t diff)
{
    SET_FLAGS_OSZAP_ADD_16(v1, v2, diff);
}

void SET_FLAGS_OSZAP_ADD8(struct CPUState *cpu, uint8_t v1, uint8_t v2, uint8_t diff)
{
    SET_FLAGS_OSZAP_ADD_8(v1, v2, diff);
}


void SET_FLAGS_OSZAPC_LOGIC32(struct CPUState *cpu, uint32_t diff)
{
    SET_FLAGS_OSZAPC_LOGIC_32(diff);
}

void SET_FLAGS_OSZAPC_LOGIC16(struct CPUState *cpu, uint16_t diff)
{
    SET_FLAGS_OSZAPC_LOGIC_16(diff);
}

void SET_FLAGS_OSZAPC_LOGIC8(struct CPUState *cpu, uint8_t diff)
{
    SET_FLAGS_OSZAPC_LOGIC_8(diff);
}

void SET_FLAGS_SHR32(struct CPUState *cpu, uint32_t v, int count, uint32_t res)
{
    int cf = (v >> (count - 1)) & 0x1;
    int of = (((res << 1) ^ res) >> 31);

    SET_FLAGS_OSZAPC_LOGIC_32(res);
    SET_FLAGS_OxxxxC(cpu, of, cf);
}

void SET_FLAGS_SHR16(struct CPUState *cpu, uint16_t v, int count, uint16_t res)
{
    int cf = (v >> (count - 1)) & 0x1;
    int of = (((res << 1) ^ res) >> 15);

    SET_FLAGS_OSZAPC_LOGIC_16(res);
    SET_FLAGS_OxxxxC(cpu, of, cf);
}

void SET_FLAGS_SHR8(struct CPUState *cpu, uint8_t v, int count, uint8_t res)
{
    int cf = (v >> (count - 1)) & 0x1;
    int of = (((res << 1) ^ res) >> 7);

    SET_FLAGS_OSZAPC_LOGIC_8(res);
    SET_FLAGS_OxxxxC(cpu, of, cf);
}

void SET_FLAGS_SAR32(struct CPUState *cpu, int32_t v, int count, uint32_t res)
{
    int cf = (v >> (count - 1)) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_32(res);
    SET_FLAGS_OxxxxC(cpu, 0, cf);
}

void SET_FLAGS_SAR16(struct CPUState *cpu, int16_t v, int count, uint16_t res)
{
    int cf = (v >> (count - 1)) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_16(res);
    SET_FLAGS_OxxxxC(cpu, 0, cf);
}

void SET_FLAGS_SAR8(struct CPUState *cpu, int8_t v, int count, uint8_t res)
{
    int cf = (v >> (count - 1)) & 0x1;

    SET_FLAGS_OSZAPC_LOGIC_8(res);
    SET_FLAGS_OxxxxC(cpu, 0, cf);
}


void SET_FLAGS_SHL32(struct CPUState *cpu, uint32_t v, int count, uint32_t res)
{
    int of, cf;

    cf = (v >> (32 - count)) & 0x1;
    of = cf ^ (res >> 31);

    SET_FLAGS_OSZAPC_LOGIC_32(res);
    SET_FLAGS_OxxxxC(cpu, of, cf);
}

void SET_FLAGS_SHL16(struct CPUState *cpu, uint16_t v, int count, uint16_t res)
{
    int of = 0, cf = 0;

    if (count <= 16) {
        cf = (v >> (16 - count)) & 0x1;
        of = cf ^ (res >> 15);
    }

    SET_FLAGS_OSZAPC_LOGIC_16(res);
    SET_FLAGS_OxxxxC(cpu, of, cf);
}

void SET_FLAGS_SHL8(struct CPUState *cpu, uint8_t v, int count, uint8_t res)
{
    int of = 0, cf = 0;

    if (count <= 8) {
        cf = (v >> (8 - count)) & 0x1;
        of = cf ^ (res >> 7);
    }

    SET_FLAGS_OSZAPC_LOGIC_8(res);
    SET_FLAGS_OxxxxC(cpu, of, cf);
}

bool get_PF(struct CPUState *cpu)
{
    uint32_t temp = (255 & cpu->hvf_x86->lflags.result);
    temp = temp ^ (255 & (cpu->hvf_x86->lflags.auxbits >> LF_BIT_PDB));
    temp = (temp ^ (temp >> 4)) & 0x0F;
    return (0x9669U >> temp) & 1;
}

void set_PF(struct CPUState *cpu, bool val)
{
    uint32_t temp = (255 & cpu->hvf_x86->lflags.result) ^ (!val);
    cpu->hvf_x86->lflags.auxbits &= ~(LF_MASK_PDB);
    cpu->hvf_x86->lflags.auxbits |= (temp << LF_BIT_PDB);
}

bool _get_OF(struct CPUState *cpu)
{
    return ((cpu->hvf_x86->lflags.auxbits + (1U << LF_BIT_PO)) >> LF_BIT_CF) & 1;
}

bool get_OF(struct CPUState *cpu)
{
    return _get_OF(cpu);
}

bool _get_CF(struct CPUState *cpu)
{
    return (cpu->hvf_x86->lflags.auxbits >> LF_BIT_CF) & 1;
}

bool get_CF(struct CPUState *cpu)
{
    return _get_CF(cpu);
}

void set_OF(struct CPUState *cpu, bool val)
{
    SET_FLAGS_OxxxxC(cpu, val, _get_CF(cpu));
}

void set_CF(struct CPUState *cpu, bool val)
{
    SET_FLAGS_OxxxxC(cpu, _get_OF(cpu), (val));
}

bool get_AF(struct CPUState *cpu)
{
    return (cpu->hvf_x86->lflags.auxbits >> LF_BIT_AF) & 1;
}

void set_AF(struct CPUState *cpu, bool val)
{
    cpu->hvf_x86->lflags.auxbits &= ~(LF_MASK_AF);
    cpu->hvf_x86->lflags.auxbits |= (val) << LF_BIT_AF;
}

bool get_ZF(struct CPUState *cpu)
{
    return !cpu->hvf_x86->lflags.result;
}

void set_ZF(struct CPUState *cpu, bool val)
{
    if (val) {
        cpu->hvf_x86->lflags.auxbits ^= (((cpu->hvf_x86->lflags.result >> LF_SIGN_BIT) & 1) << LF_BIT_SD);
        // merge the parity bits into the Parity Delta Byte
        uint32_t temp_pdb = (255 & cpu->hvf_x86->lflags.result);
        cpu->hvf_x86->lflags.auxbits ^= (temp_pdb << LF_BIT_PDB);
        // now zero the .result value
        cpu->hvf_x86->lflags.result = 0;
    } else
        cpu->hvf_x86->lflags.result |= (1 << 8);
}

bool get_SF(struct CPUState *cpu)
{
    return ((cpu->hvf_x86->lflags.result >> LF_SIGN_BIT) ^ (cpu->hvf_x86->lflags.auxbits >> LF_BIT_SD)) & 1;
}

void set_SF(struct CPUState *cpu, bool val)
{
    bool temp_sf = get_SF(cpu);
    cpu->hvf_x86->lflags.auxbits ^= (temp_sf ^ val) << LF_BIT_SD;
}

void set_OSZAPC(struct CPUState *cpu, uint32_t flags32)
{
    set_OF(cpu, cpu->hvf_x86->rflags.of);
    set_SF(cpu, cpu->hvf_x86->rflags.sf);
    set_ZF(cpu, cpu->hvf_x86->rflags.zf);
    set_AF(cpu, cpu->hvf_x86->rflags.af);
    set_PF(cpu, cpu->hvf_x86->rflags.pf);
    set_CF(cpu, cpu->hvf_x86->rflags.cf);
}

void lflags_to_rflags(struct CPUState *cpu)
{
    cpu->hvf_x86->rflags.cf = get_CF(cpu);
    cpu->hvf_x86->rflags.pf = get_PF(cpu);
    cpu->hvf_x86->rflags.af = get_AF(cpu);
    cpu->hvf_x86->rflags.zf = get_ZF(cpu);
    cpu->hvf_x86->rflags.sf = get_SF(cpu);
    cpu->hvf_x86->rflags.of = get_OF(cpu);
}

void rflags_to_lflags(struct CPUState *cpu)
{
    cpu->hvf_x86->lflags.auxbits = cpu->hvf_x86->lflags.result = 0;
    set_OF(cpu, cpu->hvf_x86->rflags.of);
    set_SF(cpu, cpu->hvf_x86->rflags.sf);
    set_ZF(cpu, cpu->hvf_x86->rflags.zf);
    set_AF(cpu, cpu->hvf_x86->rflags.af);
    set_PF(cpu, cpu->hvf_x86->rflags.pf);
    set_CF(cpu, cpu->hvf_x86->rflags.cf);
}

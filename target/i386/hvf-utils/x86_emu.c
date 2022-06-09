/*
 * Copyright (C) 2016 Veertu Inc,
 * Copyright (C) 2017 Google Inc,
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 or
 * (at your option) version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2001-2012  The Bochs Project
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

#include "qemu/osdep.h"

#include "qemu-common.h"
#include "x86_decode.h"
#include "x86.h"
#include "x86_emu.h"
#include "x86_mmu.h"
#include "vmcs.h"
#include "vmx.h"

static void print_debug(struct CPUState *cpu);
void hvf_handle_io(uint16_t port, void *data, int direction, int size, uint32_t count);

#define EXEC_2OP_LOGIC_CMD(cpu, decode, cmd, FLAGS_FUNC, save_res) \
{                                                       \
    fetch_operands(cpu, decode, 2, true, true, false);  \
    switch (decode->operand_size) {                     \
    case 1:                                         \
    {                                               \
        uint8_t v1 = (uint8_t)decode->op[0].val;    \
        uint8_t v2 = (uint8_t)decode->op[1].val;    \
        uint8_t diff = v1 cmd v2;                   \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 1);  \
        FLAGS_FUNC##_8(diff);                       \
        break;                                      \
    }                                               \
    case 2:                                        \
    {                                               \
        uint16_t v1 = (uint16_t)decode->op[0].val;  \
        uint16_t v2 = (uint16_t)decode->op[1].val;  \
        uint16_t diff = v1 cmd v2;                  \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 2); \
        FLAGS_FUNC##_16(diff);                      \
        break;                                      \
    }                                               \
    case 4:                                        \
    {                                               \
        uint32_t v1 = (uint32_t)decode->op[0].val;  \
        uint32_t v2 = (uint32_t)decode->op[1].val;  \
        uint32_t diff = v1 cmd v2;                  \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 4); \
        FLAGS_FUNC##_32(diff);                      \
        break;                                      \
    }                                               \
    case 8:                                        \
    {                                               \
        uint64_t v1 = (uint64_t)decode->op[0].val;  \
        uint64_t v2 = (uint64_t)decode->op[1].val;  \
        uint64_t diff = v1 cmd v2;                  \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 8); \
        FLAGS_FUNC##_64(diff);                      \
        break;                                      \
    }                                               \
    default:                                        \
        VM_PANIC("bad size\n");                    \
    }                                                   \
}                                                       \


#define EXEC_2OP_ARITH_CMD(cpu, decode, cmd, FLAGS_FUNC, save_res) \
{                                                       \
    fetch_operands(cpu, decode, 2, true, true, false);  \
    switch (decode->operand_size) {                     \
    case 1:                                         \
    {                                               \
        uint8_t v1 = (uint8_t)decode->op[0].val;    \
        uint8_t v2 = (uint8_t)decode->op[1].val;    \
        uint8_t diff = v1 cmd v2;                   \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 1);  \
        FLAGS_FUNC##_8(v1, v2, diff);               \
        break;                                      \
    }                                               \
    case 2:                                        \
    {                                               \
        uint16_t v1 = (uint16_t)decode->op[0].val;  \
        uint16_t v2 = (uint16_t)decode->op[1].val;  \
        uint16_t diff = v1 cmd v2;                  \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 2); \
        FLAGS_FUNC##_16(v1, v2, diff);              \
        break;                                      \
    }                                               \
    case 4:                                        \
    {                                               \
        uint32_t v1 = (uint32_t)decode->op[0].val;  \
        uint32_t v2 = (uint32_t)decode->op[1].val;  \
        uint32_t diff = v1 cmd v2;                  \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 4); \
        FLAGS_FUNC##_32(v1, v2, diff);              \
        break;                                      \
    }                                               \
    case 8:                                        \
    {                                               \
        uint64_t v1 = (uint32_t)decode->op[0].val;  \
        uint64_t v2 = (uint32_t)decode->op[1].val;  \
        uint64_t diff = v1 cmd v2;                  \
        if (save_res)                               \
            write_val_ext(cpu, decode->op[0].ptr, diff, 8); \
        FLAGS_FUNC##_64(v1, v2, diff);              \
        break;                                      \
    }                                               \
    default:                                        \
        VM_PANIC("bad size\n");                    \
    }                                                   \
}

addr_t read_reg(struct CPUState* cpu, int reg, int size)
{
    switch (size) {
        case 1:
            return cpu->hvf_x86->regs[reg].lx;
        case 2:
            return cpu->hvf_x86->regs[reg].rx;
        case 4:
            return cpu->hvf_x86->regs[reg].erx;
        case 8:
            return cpu->hvf_x86->regs[reg].rrx;
        default:
            VM_PANIC_ON("read_reg size");
    }
    return 0;
}

void write_reg(struct CPUState* cpu, int reg, addr_t val, int size)
{
    switch (size) {
        case 1:
            cpu->hvf_x86->regs[reg].lx = val;
            break;
        case 2:
            cpu->hvf_x86->regs[reg].rx = val;
            break;
        case 4:
            cpu->hvf_x86->regs[reg].rrx = (uint32_t)val;
            break;
        case 8:
            cpu->hvf_x86->regs[reg].rrx = val;
            break;
        default:
            VM_PANIC_ON("write_reg size");
    }
}

addr_t read_val_from_reg(addr_t reg_ptr, int size)
{
    addr_t val;
    
    switch (size) {
        case 1:
            val = *(uint8_t*)reg_ptr;
            break;
        case 2:
            val = *(uint16_t*)reg_ptr;
            break;
        case 4:
            val = *(uint32_t*)reg_ptr;
            break;
        case 8:
            val = *(uint64_t*)reg_ptr;
            break;
        default:
            VM_PANIC_ON_EX(1, "read_val: Unknown size %d\n", size);
            break;
    }
    return val;
}

void write_val_to_reg(addr_t reg_ptr, addr_t val, int size)
{
    switch (size) {
        case 1:
            *(uint8_t*)reg_ptr = val;
            break;
        case 2:
            *(uint16_t*)reg_ptr = val;
            break;
        case 4:
            *(uint64_t*)reg_ptr = (uint32_t)val;
            break;
        case 8:
            *(uint64_t*)reg_ptr = val;
            break;
        default:
            VM_PANIC("write_val: Unknown size\n");
            break;
    }
}

static bool is_host_reg(struct CPUState* cpu, addr_t ptr) {
    return (ptr > (addr_t)cpu && ptr < (addr_t)cpu + sizeof(struct CPUState)) ||
           (ptr > (addr_t)cpu->hvf_x86 && ptr < (addr_t)(cpu->hvf_x86 + sizeof(struct hvf_x86_state)));
}

void write_val_ext(struct CPUState* cpu, addr_t ptr, addr_t val, int size)
{
    if (is_host_reg(cpu, ptr)) {
        write_val_to_reg(ptr, val, size);
        return;
    }
    vmx_write_mem(cpu, ptr, &val, size);
}

uint8_t *read_mmio(struct CPUState* cpu, addr_t ptr, int bytes)
{
    vmx_read_mem(cpu, cpu->hvf_x86->mmio_buf, ptr, bytes);
    return cpu->hvf_x86->mmio_buf;
}

addr_t read_val_ext(struct CPUState* cpu, addr_t ptr, int size)
{
    addr_t val;
    uint8_t *mmio_ptr;
    
    if (is_host_reg(cpu, ptr)) {
        return read_val_from_reg(ptr, size);
    }
    
    mmio_ptr = read_mmio(cpu, ptr, size);
    switch (size) {
        case 1:
            val = *(uint8_t*)mmio_ptr;
            break;
        case 2:
            val = *(uint16_t*)mmio_ptr;
            break;
        case 4:
            val = *(uint32_t*)mmio_ptr;
            break;
        case 8:
            val = *(uint64_t*)mmio_ptr;
            break;
        default:
            VM_PANIC("bad size\n");
            break;
    }
    return val;
}

static void fetch_operands(struct CPUState *cpu, struct x86_decode *decode, int n, bool val_op0, bool val_op1, bool val_op2)
{
    int i;
    bool calc_val[3] = {val_op0, val_op1, val_op2};

    for (i = 0; i < n; i++) {
        switch (decode->op[i].type) {
            case X86_VAR_IMMEDIATE:
                break;
            case X86_VAR_REG:
                VM_PANIC_ON(!decode->op[i].ptr);
                if (calc_val[i])
                    decode->op[i].val = read_val_from_reg(decode->op[i].ptr, decode->operand_size);
                break;
            case X86_VAR_RM:
                calc_modrm_operand(cpu, decode, &decode->op[i]);
                if (calc_val[i])
                    decode->op[i].val = read_val_ext(cpu, decode->op[i].ptr, decode->operand_size);
                break;
            case X86_VAR_OFFSET:
                decode->op[i].ptr = decode_linear_addr(cpu, decode, decode->op[i].ptr, REG_SEG_DS);
                if (calc_val[i])
                    decode->op[i].val = read_val_ext(cpu, decode->op[i].ptr, decode->operand_size);
                break;
            default:
                break;
        }
    }
}

static void exec_mov(struct CPUState *cpu, struct x86_decode *decode)
{
    fetch_operands(cpu, decode, 2, false, true, false);
    write_val_ext(cpu, decode->op[0].ptr, decode->op[1].val, decode->operand_size);

    RIP(cpu) += decode->len;
}

static void exec_add(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_ARITH_CMD(cpu, decode, +, SET_FLAGS_OSZAPC_ADD, true);
    RIP(cpu) += decode->len;
}

static void exec_or(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_LOGIC_CMD(cpu, decode, |, SET_FLAGS_OSZAPC_LOGIC, true);
    RIP(cpu) += decode->len;
}

static void exec_adc(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_ARITH_CMD(cpu, decode, +get_CF(cpu)+, SET_FLAGS_OSZAPC_ADD, true);
    RIP(cpu) += decode->len;
}

static void exec_sbb(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_ARITH_CMD(cpu, decode, -get_CF(cpu)-, SET_FLAGS_OSZAPC_SUB, true);
    RIP(cpu) += decode->len;
}

static void exec_and(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_LOGIC_CMD(cpu, decode, &, SET_FLAGS_OSZAPC_LOGIC, true);
    RIP(cpu) += decode->len;
}

static void exec_sub(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_ARITH_CMD(cpu, decode, -, SET_FLAGS_OSZAPC_SUB, true);
    RIP(cpu) += decode->len;
}

static void exec_xor(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_LOGIC_CMD(cpu, decode, ^, SET_FLAGS_OSZAPC_LOGIC, true);
    RIP(cpu) += decode->len;
}

static void exec_neg(struct CPUState *cpu, struct x86_decode *decode)
{
    //EXEC_2OP_ARITH_CMD(cpu, decode, -, SET_FLAGS_OSZAPC_SUB, false);
    int64_t val;
    fetch_operands(cpu, decode, 2, true, true, false);

    val = 0 - sign(decode->op[1].val, decode->operand_size);
    write_val_ext(cpu, decode->op[1].ptr, val, decode->operand_size);

    if (8 == decode->operand_size) {
        SET_FLAGS_OSZAPC_SUB_64(0, 0 - val, val);
    }
    else if (4 == decode->operand_size) {
        SET_FLAGS_OSZAPC_SUB_32(0, 0 - val, val);
    }
    else if (2 == decode->operand_size) {
        SET_FLAGS_OSZAPC_SUB_16(0, 0 - val, val);
    }
    else if (1 == decode->operand_size) {
        SET_FLAGS_OSZAPC_SUB_8(0, 0 - val, val);
    } else {
        VM_PANIC("bad op size\n");
    }

    //lflags_to_rflags(cpu);
    RIP(cpu) += decode->len;
}

static void exec_cmp(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_ARITH_CMD(cpu, decode, -, SET_FLAGS_OSZAPC_SUB, false);
    RIP(cpu) += decode->len;
}

static void exec_inc(struct CPUState *cpu, struct x86_decode *decode)
{
    decode->op[1].type = X86_VAR_IMMEDIATE;
    decode->op[1].val = 0;

    EXEC_2OP_ARITH_CMD(cpu, decode, +1+, SET_FLAGS_OSZAP_ADD, true);

    RIP(cpu) += decode->len;
}

static void exec_dec(struct CPUState *cpu, struct x86_decode *decode)
{
    decode->op[1].type = X86_VAR_IMMEDIATE;
    decode->op[1].val = 0;

    EXEC_2OP_ARITH_CMD(cpu, decode, -1-, SET_FLAGS_OSZAP_SUB, true);
    RIP(cpu) += decode->len;
}

static void exec_tst(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_LOGIC_CMD(cpu, decode, &, SET_FLAGS_OSZAPC_LOGIC, false);
    RIP(cpu) += decode->len;
}

static void exec_not(struct CPUState *cpu, struct x86_decode *decode)
{
    fetch_operands(cpu, decode, 1, true, false, false);

    write_val_ext(cpu, decode->op[0].ptr, ~decode->op[0].val, decode->operand_size);
    RIP(cpu) += decode->len;
}

void exec_movzx(struct CPUState *cpu, struct x86_decode *decode)
{
    int src_op_size;
    int op_size = decode->operand_size;

    fetch_operands(cpu, decode, 1, false, false, false);

    if (0xb6 == decode->opcode[1])
        src_op_size = 1;
    else
        src_op_size = 2;
    decode->operand_size = src_op_size;
    calc_modrm_operand(cpu, decode, &decode->op[1]);
    decode->op[1].val = read_val_ext(cpu, decode->op[1].ptr, src_op_size);
    write_val_ext(cpu, decode->op[0].ptr, decode->op[1].val, op_size);

    RIP(cpu) += decode->len;
}

static void exec_out(struct CPUState *cpu, struct x86_decode *decode)
{
    switch (decode->opcode[0]) {
        case 0xe6:
            hvf_handle_io(decode->op[0].val, &AL(cpu), 1, 1, 1);
            break;
        case 0xe7:
            hvf_handle_io(decode->op[0].val, &RAX(cpu), 1, decode->operand_size, 1);
            break;
        case 0xee:
            hvf_handle_io(DX(cpu), &AL(cpu), 1, 1, 1);
            break;
        case 0xef:
            hvf_handle_io(DX(cpu), &RAX(cpu), 1, decode->operand_size, 1);
            break;
        default:
            VM_PANIC("Bad out opcode\n");
            break;
    }
    RIP(cpu) += decode->len;
}

static void exec_in(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t val = 0;
    switch (decode->opcode[0]) {
        case 0xe4:
            hvf_handle_io(decode->op[0].val, &AL(cpu), 0, 1, 1);
            break;
        case 0xe5:
            hvf_handle_io(decode->op[0].val, &val, 0, decode->operand_size, 1);
            if (decode->operand_size == 2)
                AX(cpu) = val;
            else
                RAX(cpu) = (uint32_t)val;
            break;
        case 0xec:
            hvf_handle_io(DX(cpu), &AL(cpu), 0, 1, 1);
            break;
        case 0xed:
            hvf_handle_io(DX(cpu), &val, 0, decode->operand_size, 1);
            if (decode->operand_size == 2)
                AX(cpu) = val;
            else
                RAX(cpu) = (uint32_t)val;

            break;
        default:
            VM_PANIC("Bad in opcode\n");
            break;
    }

    RIP(cpu) += decode->len;
}

static inline void string_increment_reg(struct CPUState * cpu, int reg, struct x86_decode *decode)
{
    addr_t val = read_reg(cpu, reg, decode->addressing_size);
    if (cpu->hvf_x86->rflags.df)
        val -= decode->operand_size;
    else
        val += decode->operand_size;
    write_reg(cpu, reg, val, decode->addressing_size);
}

static inline void string_rep(struct CPUState * cpu, struct x86_decode *decode, void (*func)(struct CPUState *cpu, struct x86_decode *ins), int rep)
{
    addr_t rcx = read_reg(cpu, REG_RCX, decode->addressing_size);
    while (rcx--) {
        func(cpu, decode);
        write_reg(cpu, REG_RCX, rcx, decode->addressing_size);
        if ((PREFIX_REP == rep) && !get_ZF(cpu))
            break;
        if ((PREFIX_REPN == rep) && get_ZF(cpu))
            break;
    }
}

static void exec_ins_single(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t addr = linear_addr_size(cpu, RDI(cpu), decode->addressing_size, REG_SEG_ES);

    hvf_handle_io(DX(cpu), cpu->hvf_x86->mmio_buf, 0, decode->operand_size, 1);
    vmx_write_mem(cpu, addr, cpu->hvf_x86->mmio_buf, decode->operand_size);

    string_increment_reg(cpu, REG_RDI, decode);
}

static void exec_ins(struct CPUState *cpu, struct x86_decode *decode)
{
    if (decode->rep)
        string_rep(cpu, decode, exec_ins_single, 0);
    else
        exec_ins_single(cpu, decode);

    RIP(cpu) += decode->len;
}

static void exec_outs_single(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t addr = decode_linear_addr(cpu, decode, RSI(cpu), REG_SEG_DS);

    vmx_read_mem(cpu, cpu->hvf_x86->mmio_buf, addr, decode->operand_size);
    hvf_handle_io(DX(cpu), cpu->hvf_x86->mmio_buf, 1, decode->operand_size, 1);

    string_increment_reg(cpu, REG_RSI, decode);
}

static void exec_outs(struct CPUState *cpu, struct x86_decode *decode)
{
    if (decode->rep)
        string_rep(cpu, decode, exec_outs_single, 0);
    else
        exec_outs_single(cpu, decode);
    
    RIP(cpu) += decode->len;
}

static void exec_movs_single(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t src_addr;
    addr_t dst_addr;
    addr_t val;
    
    src_addr = decode_linear_addr(cpu, decode, RSI(cpu), REG_SEG_DS);
    dst_addr = linear_addr_size(cpu, RDI(cpu), decode->addressing_size, REG_SEG_ES);
    
    val = read_val_ext(cpu, src_addr, decode->operand_size);
    write_val_ext(cpu, dst_addr, val, decode->operand_size);

    string_increment_reg(cpu, REG_RSI, decode);
    string_increment_reg(cpu, REG_RDI, decode);
}

static void exec_movs(struct CPUState *cpu, struct x86_decode *decode)
{
    if (decode->rep) {
        string_rep(cpu, decode, exec_movs_single, 0);
    }
    else
        exec_movs_single(cpu, decode);

    RIP(cpu) += decode->len;
}

static void exec_cmps_single(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t src_addr;
    addr_t dst_addr;

    src_addr = decode_linear_addr(cpu, decode, RSI(cpu), REG_SEG_DS);
    dst_addr = linear_addr_size(cpu, RDI(cpu), decode->addressing_size, REG_SEG_ES);

    decode->op[0].type = X86_VAR_IMMEDIATE;
    decode->op[0].val = read_val_ext(cpu, src_addr, decode->operand_size);
    decode->op[1].type = X86_VAR_IMMEDIATE;
    decode->op[1].val = read_val_ext(cpu, dst_addr, decode->operand_size);

    EXEC_2OP_ARITH_CMD(cpu, decode, -, SET_FLAGS_OSZAPC_SUB, false);

    string_increment_reg(cpu, REG_RSI, decode);
    string_increment_reg(cpu, REG_RDI, decode);
}

static void exec_cmps(struct CPUState *cpu, struct x86_decode *decode)
{
    if (decode->rep) {
        string_rep(cpu, decode, exec_cmps_single, decode->rep);
    }
    else
        exec_cmps_single(cpu, decode);
    RIP(cpu) += decode->len;
}


static void exec_stos_single(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t addr;
    addr_t val;

    addr = linear_addr_size(cpu, RDI(cpu), decode->addressing_size, REG_SEG_ES);
    val = read_reg(cpu, REG_RAX, decode->operand_size);
    vmx_write_mem(cpu, addr, &val, decode->operand_size);

    string_increment_reg(cpu, REG_RDI, decode);
}


static void exec_stos(struct CPUState *cpu, struct x86_decode *decode)
{
    if (decode->rep) {
        string_rep(cpu, decode, exec_stos_single, 0);
    }
    else
        exec_stos_single(cpu, decode);

    RIP(cpu) += decode->len;
}

static void exec_scas_single(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t addr;
    
    addr = linear_addr_size(cpu, RDI(cpu), decode->addressing_size, REG_SEG_ES);
    decode->op[1].type = X86_VAR_IMMEDIATE;
    vmx_read_mem(cpu, &decode->op[1].val, addr, decode->operand_size);

    EXEC_2OP_ARITH_CMD(cpu, decode, -, SET_FLAGS_OSZAPC_SUB, false);
    string_increment_reg(cpu, REG_RDI, decode);
}

static void exec_scas(struct CPUState *cpu, struct x86_decode *decode)
{
    decode->op[0].type = X86_VAR_REG;
    decode->op[0].reg = REG_RAX;
    if (decode->rep) {
        string_rep(cpu, decode, exec_scas_single, decode->rep);
    }
    else
        exec_scas_single(cpu, decode);

    RIP(cpu) += decode->len;
}

static void exec_lods_single(struct CPUState *cpu, struct x86_decode *decode)
{
    addr_t addr;
    addr_t val = 0;
    
    addr = decode_linear_addr(cpu, decode, RSI(cpu), REG_SEG_DS);
    vmx_read_mem(cpu, &val, addr,  decode->operand_size);
    write_reg(cpu, REG_RAX, val, decode->operand_size);

    string_increment_reg(cpu, REG_RSI, decode);
}

static void exec_lods(struct CPUState *cpu, struct x86_decode *decode)
{
    if (decode->rep) {
        string_rep(cpu, decode, exec_lods_single, 0);
    }
    else
        exec_lods_single(cpu, decode);

    RIP(cpu) += decode->len;
}

#define MSR_IA32_UCODE_REV 		0x00000017

void simulate_rdmsr(struct CPUState *cpu)
{
    X86CPU *x86_cpu = X86_CPU(cpu);
    CPUX86State *env = &x86_cpu->env;
    uint32_t msr = ECX(cpu);
    uint64_t val = 0;

    switch (msr) {
        case MSR_IA32_TSC:
            val = rdtscp() + rvmcs(cpu->hvf_fd, VMCS_TSC_OFFSET);
            break;
        case MSR_IA32_APICBASE:
            val = cpu_get_apic_base(X86_CPU(cpu)->apic_state);
            break;
        case MSR_IA32_UCODE_REV:
            val = (0x100000000ULL << 32) | 0x100000000ULL;
            break;
        case MSR_EFER:
            val = rvmcs(cpu->hvf_fd, VMCS_GUEST_IA32_EFER);
            break;
        case MSR_FSBASE:
            val = rvmcs(cpu->hvf_fd, VMCS_GUEST_FS_BASE);
            break;
        case MSR_GSBASE:
            val = rvmcs(cpu->hvf_fd, VMCS_GUEST_GS_BASE);
            break;
        case MSR_KERNELGSBASE:
            val = rvmcs(cpu->hvf_fd, VMCS_HOST_FS_BASE);
            break;
        case MSR_STAR:
            abort();
            break;
        case MSR_LSTAR:
            abort();
            break;
        case MSR_CSTAR:
            abort();
            break;
        case MSR_IA32_MISC_ENABLE:
            val = env->msr_ia32_misc_enable;
            break;
        case MSR_MTRRphysBase(0):
        case MSR_MTRRphysBase(1):
        case MSR_MTRRphysBase(2):
        case MSR_MTRRphysBase(3):
        case MSR_MTRRphysBase(4):
        case MSR_MTRRphysBase(5):
        case MSR_MTRRphysBase(6):
        case MSR_MTRRphysBase(7):
            val = env->mtrr_var[(ECX(cpu) - MSR_MTRRphysBase(0)) / 2].base;
            break;
        case MSR_MTRRphysMask(0):
        case MSR_MTRRphysMask(1):
        case MSR_MTRRphysMask(2):
        case MSR_MTRRphysMask(3):
        case MSR_MTRRphysMask(4):
        case MSR_MTRRphysMask(5):
        case MSR_MTRRphysMask(6):
        case MSR_MTRRphysMask(7):
            val = env->mtrr_var[(ECX(cpu) - MSR_MTRRphysMask(0)) / 2].mask;
            break;
        case MSR_MTRRfix64K_00000:
            val = env->mtrr_fixed[0];
            break;
        case MSR_MTRRfix16K_80000:
        case MSR_MTRRfix16K_A0000:
            val = env->mtrr_fixed[ECX(cpu) - MSR_MTRRfix16K_80000 + 1];
            break;
        case MSR_MTRRfix4K_C0000:
        case MSR_MTRRfix4K_C8000:
        case MSR_MTRRfix4K_D0000:
        case MSR_MTRRfix4K_D8000:
        case MSR_MTRRfix4K_E0000:
        case MSR_MTRRfix4K_E8000:
        case MSR_MTRRfix4K_F0000:
        case MSR_MTRRfix4K_F8000:
            val = env->mtrr_fixed[ECX(cpu) - MSR_MTRRfix4K_C0000 + 3];
            break;
        case MSR_MTRRdefType:
            val = env->mtrr_deftype;
            break;
        default:
            // fprintf(stderr, "%s: unknown msr 0x%x\n", __func__, msr);
            val = 0;
            break;
    }

    RAX(cpu) = (uint32_t)val;
    RDX(cpu) = (uint32_t)(val >> 32);
}

static void exec_rdmsr(struct CPUState *cpu, struct x86_decode *decode)
{
    simulate_rdmsr(cpu);
    RIP(cpu) += decode->len;
}

void simulate_wrmsr(struct CPUState *cpu)
{
    X86CPU *x86_cpu = X86_CPU(cpu);
    CPUX86State *env = &x86_cpu->env;
    uint32_t msr = ECX(cpu);
    uint64_t data = ((uint64_t)EDX(cpu) << 32) | EAX(cpu);

    switch (msr) {
        case MSR_IA32_TSC:
            // if (!osx_is_sierra())
            //     wvmcs(cpu->hvf_fd, VMCS_TSC_OFFSET, data - rdtscp());
            //hv_vm_sync_tsc(data);
            break;
        case MSR_IA32_APICBASE:
            cpu_set_apic_base(X86_CPU(cpu)->apic_state, data);
            break;
        case MSR_FSBASE:
            wvmcs(cpu->hvf_fd, VMCS_GUEST_FS_BASE, data);
            break;
        case MSR_GSBASE:
            wvmcs(cpu->hvf_fd, VMCS_GUEST_GS_BASE, data);
            break;
        case MSR_KERNELGSBASE:
            wvmcs(cpu->hvf_fd, VMCS_HOST_FS_BASE, data);
            break;
        case MSR_STAR:
            abort();
            break;
        case MSR_LSTAR:
            abort();
            break;
        case MSR_CSTAR:
            abort();
            break;
        case MSR_EFER:
            cpu->hvf_x86->efer.efer = data;
            //printf("new efer %llx\n", EFER(cpu));
            wvmcs(cpu->hvf_fd, VMCS_GUEST_IA32_EFER, data);
            if (data & EFER_NXE)
                hv_vcpu_invalidate_tlb(cpu->hvf_fd);
            break;
        case MSR_MTRRphysBase(0):
        case MSR_MTRRphysBase(1):
        case MSR_MTRRphysBase(2):
        case MSR_MTRRphysBase(3):
        case MSR_MTRRphysBase(4):
        case MSR_MTRRphysBase(5):
        case MSR_MTRRphysBase(6):
        case MSR_MTRRphysBase(7):
            env->mtrr_var[(ECX(cpu) - MSR_MTRRphysBase(0)) / 2].base = data;
            break;
        case MSR_MTRRphysMask(0):
        case MSR_MTRRphysMask(1):
        case MSR_MTRRphysMask(2):
        case MSR_MTRRphysMask(3):
        case MSR_MTRRphysMask(4):
        case MSR_MTRRphysMask(5):
        case MSR_MTRRphysMask(6):
        case MSR_MTRRphysMask(7):
            env->mtrr_var[(ECX(cpu) - MSR_MTRRphysMask(0)) / 2].mask = data;
            break;
        case MSR_MTRRfix64K_00000:
            env->mtrr_fixed[ECX(cpu) - MSR_MTRRfix64K_00000] = data;
            break;
        case MSR_MTRRfix16K_80000:
        case MSR_MTRRfix16K_A0000:
            env->mtrr_fixed[ECX(cpu) - MSR_MTRRfix16K_80000 + 1] = data;
            break;
        case MSR_MTRRfix4K_C0000:
        case MSR_MTRRfix4K_C8000:
        case MSR_MTRRfix4K_D0000:
        case MSR_MTRRfix4K_D8000:
        case MSR_MTRRfix4K_E0000:
        case MSR_MTRRfix4K_E8000:
        case MSR_MTRRfix4K_F0000:
        case MSR_MTRRfix4K_F8000:
            env->mtrr_fixed[ECX(cpu) - MSR_MTRRfix4K_C0000 + 3] = data;
            break;
        case MSR_MTRRdefType:
            env->mtrr_deftype = data;
            break;
        default:
            break;
    }

    /* Related to support known hypervisor interface */
    // if (g_hypervisor_iface)
    //     g_hypervisor_iface->wrmsr_handler(cpu, msr, data);

    //printf("write msr %llx\n", RCX(cpu));
}

static void exec_wrmsr(struct CPUState *cpu, struct x86_decode *decode)
{
    simulate_wrmsr(cpu);
    RIP(cpu) += decode->len;
}

/*
 * flag:
 * 0 - bt, 1 - btc, 2 - bts, 3 - btr
 */
static void do_bt(struct CPUState *cpu, struct x86_decode *decode, int flag)
{
    int32_t displacement;
    uint8_t index;
    bool cf;
    int mask = (4 == decode->operand_size) ? 0x1f : 0xf;

    VM_PANIC_ON(decode->rex.rex);

    fetch_operands(cpu, decode, 2, false, true, false);
    index = decode->op[1].val & mask;

    if (decode->op[0].type != X86_VAR_REG) {
        if (4 == decode->operand_size) {
            displacement = ((int32_t) (decode->op[1].val & 0xffffffe0)) / 32;
            decode->op[0].ptr += 4 * displacement;
        } else if (2 == decode->operand_size) {
            displacement = ((int16_t) (decode->op[1].val & 0xfff0)) / 16;
            decode->op[0].ptr += 2 * displacement;
        } else {
            VM_PANIC("bt 64bit\n");
        }
    }
    decode->op[0].val = read_val_ext(cpu, decode->op[0].ptr, decode->operand_size);
    cf = (decode->op[0].val >> index) & 0x01;

    switch (flag) {
        case 0:
            set_CF(cpu, cf);
            return;
        case 1:
            decode->op[0].val ^= (1u << index);
            break;
        case 2:
            decode->op[0].val |= (1u << index);
            break;
        case 3:
            decode->op[0].val &= ~(1u << index);
            break;
    }
    write_val_ext(cpu, decode->op[0].ptr, decode->op[0].val, decode->operand_size);
    set_CF(cpu, cf);
}

static void exec_bt(struct CPUState *cpu, struct x86_decode *decode)
{
    do_bt(cpu, decode, 0);
    RIP(cpu) += decode->len;
}

static void exec_btc(struct CPUState *cpu, struct x86_decode *decode)
{
    do_bt(cpu, decode, 1);
    RIP(cpu) += decode->len;
}

static void exec_btr(struct CPUState *cpu, struct x86_decode *decode)
{
    do_bt(cpu, decode, 3);
    RIP(cpu) += decode->len;
}

static void exec_bts(struct CPUState *cpu, struct x86_decode *decode)
{
    do_bt(cpu, decode, 2);
    RIP(cpu) += decode->len;
}

void exec_shl(struct CPUState *cpu, struct x86_decode *decode)
{
    uint8_t count;
    int64_t of = 0, cf = 0;

    fetch_operands(cpu, decode, 2, true, true, false);

    count = decode->op[1].val;
    count &= 0x1f;      // count is masked to 5 bits
    if (!count)
        goto exit;

    switch (decode->operand_size) {
        case 1:
        {
            uint8_t res = 0;
            if (count <= 8) {
                res = (decode->op[0].val << count);
                cf = (decode->op[0].val >> (8 - count)) & 0x1;
                of = cf ^ (res >> 7);
            }

            write_val_ext(cpu, decode->op[0].ptr, res, 1);
            SET_FLAGS_OSZAPC_LOGIC_8(res);
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 2:
        {
            uint16_t res = 0;

            /* from bochs */
            if (count <= 16) {
                res = (decode->op[0].val << count);
                cf = (decode->op[0].val >> (16 - count)) & 0x1;
                of = cf ^ (res >> 15); // of = cf ^ result15
            }

            write_val_ext(cpu, decode->op[0].ptr, res, 2);
            SET_FLAGS_OSZAPC_LOGIC_16(res);
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 4:
        {
            uint32_t res = decode->op[0].val << count;
            
            write_val_ext(cpu, decode->op[0].ptr, res, 4);
            SET_FLAGS_OSZAPC_LOGIC_32(res);
            cf = (decode->op[0].val >> (32 - count)) & 0x1;
            of = cf ^ (res >> 31); // of = cf ^ result31
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 8:
        {
            uint32_t res = decode->op[0].val << count;
            
            write_val_ext(cpu, decode->op[0].ptr, res, 8);
            SET_FLAGS_OSZAPC_LOGIC_64(res);
            cf = (decode->op[0].val >> (64 - count)) & 0x1;
            of = cf ^ (res >> 63); // of = cf ^ result63
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        default:
            abort();
    }

exit:
    //lflags_to_rflags(cpu);
    RIP(cpu) += decode->len;
}

void exec_movsx(struct CPUState *cpu, struct x86_decode *decode)
{
    int src_op_size;
    int op_size = decode->operand_size;

    fetch_operands(cpu, decode, 2, false, false, false);

    if (0xbe == decode->opcode[1])
        src_op_size = 1;
    else
        src_op_size = 2;

    decode->operand_size = src_op_size;
    calc_modrm_operand(cpu, decode, &decode->op[1]);
    decode->op[1].val = sign(read_val_ext(cpu, decode->op[1].ptr, src_op_size), src_op_size);

    write_val_ext(cpu, decode->op[0].ptr, decode->op[1].val, op_size);

    RIP(cpu) += decode->len;
}

void exec_ror(struct CPUState *cpu, struct x86_decode *decode)
{
    uint8_t count;

    fetch_operands(cpu, decode, 2, true, true, false);
    count = decode->op[1].val;

    switch (decode->operand_size) {
        case 1:
        {
            uint32_t bit6, bit7;
            uint8_t res;

            if ((count & 0x07) == 0) {
                if (count & 0x18) {
                    bit6 = ((uint8_t)decode->op[0].val >> 6) & 1;
                    bit7 = ((uint8_t)decode->op[0].val >> 7) & 1;
                    SET_FLAGS_OxxxxC(cpu, bit6 ^ bit7, bit7);
                 }
            } else {
                count &= 0x7; /* use only bottom 3 bits */
                res = ((uint8_t)decode->op[0].val >> count) | ((uint8_t)decode->op[0].val << (8 - count));
                write_val_ext(cpu, decode->op[0].ptr, res, 1);
                bit6 = (res >> 6) & 1;
                bit7 = (res >> 7) & 1;
                /* set eflags: ROR count affects the following flags: C, O */
                SET_FLAGS_OxxxxC(cpu, bit6 ^ bit7, bit7);
            }
            break;
        }
        case 2:
        {
            uint32_t bit14, bit15;
            uint16_t res;

            if ((count & 0x0f) == 0) {
                if (count & 0x10) {
                    bit14 = ((uint16_t)decode->op[0].val >> 14) & 1;
                    bit15 = ((uint16_t)decode->op[0].val >> 15) & 1;
                    // of = result14 ^ result15
                    SET_FLAGS_OxxxxC(cpu, bit14 ^ bit15, bit15);
                }
            } else {
                count &= 0x0f;  // use only 4 LSB's
                res = ((uint16_t)decode->op[0].val >> count) | ((uint16_t)decode->op[0].val << (16 - count));
                write_val_ext(cpu, decode->op[0].ptr, res, 2);

                bit14 = (res >> 14) & 1;
                bit15 = (res >> 15) & 1;
                // of = result14 ^ result15
                SET_FLAGS_OxxxxC(cpu, bit14 ^ bit15, bit15);
            }
            break;
        }
        case 4:
        {
            uint32_t bit31, bit30;
            uint32_t res;

            count &= 0x1f;
            if (count) {
                res = ((uint32_t)decode->op[0].val >> count) | ((uint32_t)decode->op[0].val << (32 - count));
                write_val_ext(cpu, decode->op[0].ptr, res, 4);

                bit31 = (res >> 31) & 1;
                bit30 = (res >> 30) & 1;
                // of = result30 ^ result31
                SET_FLAGS_OxxxxC(cpu, bit30 ^ bit31, bit31);
            }
            break;
        }
        case 8:
        {
            uint64_t bit63, bit62;
            uint64_t res;

            count &= 0x3f;
            if (count) {
                res = ((uint64_t)decode->op[0].val >> count) | ((uint64_t)decode->op[0].val << (64 - count));
                write_val_ext(cpu, decode->op[0].ptr, res, 8);

                bit63 = (res >> 63) & 1;
                bit62 = (res >> 30) & 1;
                // of = result62 ^ result63
                SET_FLAGS_OxxxxC(cpu, bit62 ^ bit63, bit63);
            }
            break;
        }
        default:
        {
            abort();
        }
    }
    RIP(cpu) += decode->len;
}

void exec_rol(struct CPUState *cpu, struct x86_decode *decode)
{
    uint8_t count;

    fetch_operands(cpu, decode, 2, true, true, false);
    count = decode->op[1].val;

    switch (decode->operand_size) {
        case 1:
        {
            uint32_t bit0, bit7;
            uint8_t res;

            if ((count & 0x07) == 0) {
                if (count & 0x18) {
                    bit0 = ((uint8_t)decode->op[0].val & 1);
                    bit7 = ((uint8_t)decode->op[0].val >> 7);
                    SET_FLAGS_OxxxxC(cpu, bit0 ^ bit7, bit0);
                }
            }  else {
                count &= 0x7; // use only lowest 3 bits
                res = ((uint8_t)decode->op[0].val << count) | ((uint8_t)decode->op[0].val >> (8 - count));

                write_val_ext(cpu, decode->op[0].ptr, res, 1);
                /* set eflags:
                 * ROL count affects the following flags: C, O
                 */
                bit0 = (res &  1);
                bit7 = (res >> 7);
                SET_FLAGS_OxxxxC(cpu, bit0 ^ bit7, bit0);
            }
            break;
        }
        case 2:
        {
            uint32_t bit0, bit15;
            uint16_t res;

            if ((count & 0x0f) == 0) {
                if (count & 0x10) {
                    bit0  = ((uint16_t)decode->op[0].val & 0x1);
                    bit15 = ((uint16_t)decode->op[0].val >> 15);
                    // of = cf ^ result15
                    SET_FLAGS_OxxxxC(cpu, bit0 ^ bit15, bit0);
                }
            } else {
                count &= 0x0f; // only use bottom 4 bits
                res = ((uint16_t)decode->op[0].val << count) | ((uint16_t)decode->op[0].val >> (16 - count));

                write_val_ext(cpu, decode->op[0].ptr, res, 2);
                bit0  = (res & 0x1);
                bit15 = (res >> 15);
                // of = cf ^ result15
                SET_FLAGS_OxxxxC(cpu, bit0 ^ bit15, bit0);
            }
            break;
        }
        case 4:
        {
            uint32_t bit0, bit31;
            uint32_t res;

            count &= 0x1f;
            if (count) {
                res = ((uint32_t)decode->op[0].val << count) | ((uint32_t)decode->op[0].val >> (32 - count));

                write_val_ext(cpu, decode->op[0].ptr, res, 4);
                bit0  = (res & 0x1);
                bit31 = (res >> 31);
                // of = cf ^ result31
                SET_FLAGS_OxxxxC(cpu, bit0 ^ bit31, bit0);
            }
            break;
        }
        case 8:
        {
            uint64_t bit0, bit63;
            uint64_t res;

            count &= 0x3f;
            if (count) {
                res = ((uint64_t)decode->op[0].val << count) | ((uint64_t)decode->op[0].val >> (64 - count));

                write_val_ext(cpu, decode->op[0].ptr, res, 8);
                bit0  = (res & 0x1);
                bit63 = (res >> 63);
                // of = cf ^ result63
                SET_FLAGS_OxxxxC(cpu, bit0 ^ bit63, bit0);
            }
            break;
        }
        default:
        {
            abort();
        }
    }
    RIP(cpu) += decode->len;
}


void exec_rcl(struct CPUState *cpu, struct x86_decode *decode)
{
    uint8_t count;
    int of = 0, cf = 0;

    fetch_operands(cpu, decode, 2, true, true, false);
    count = decode->op[1].val & 0x1f;

    switch(decode->operand_size) {
        case 1:
        {
            uint8_t op1_8 = decode->op[0].val;
            uint8_t res;
            count %= 9;
            if (!count)
                break;

            if (1 == count)
                res = (op1_8 << 1) | get_CF(cpu);
            else
                res = (op1_8 << count) | (get_CF(cpu) << (count - 1)) | (op1_8 >> (9 - count));

            write_val_ext(cpu, decode->op[0].ptr, res, 1);

            cf = (op1_8 >> (8 - count)) & 0x01;
            of = cf ^ (res >> 7); // of = cf ^ result7
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 2:
        {
            uint16_t res;
            uint16_t op1_16 = decode->op[0].val;

            count %= 17;
            if (!count)
                break;

            if (1 == count)
                res = (op1_16 << 1) | get_CF(cpu);
            else if (count == 16)
                res = (get_CF(cpu) << 15) | (op1_16 >> 1);
            else  // 2..15
                res = (op1_16 << count) | (get_CF(cpu) << (count - 1)) | (op1_16 >> (17 - count));
            
            write_val_ext(cpu, decode->op[0].ptr, res, 2);
            
            cf = (op1_16 >> (16 - count)) & 0x1;
            of = cf ^ (res >> 15); // of = cf ^ result15
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 4:
        {
            uint32_t res;
            uint32_t op1_32 = decode->op[0].val;

            if (!count)
                break;

            if (1 == count)
                res = (op1_32 << 1) | get_CF(cpu);
            else
                res = (op1_32 << count) | (get_CF(cpu) << (count - 1)) | (op1_32 >> (33 - count));

            write_val_ext(cpu, decode->op[0].ptr, res, 4);

            cf = (op1_32 >> (32 - count)) & 0x1;
            of = cf ^ (res >> 31); // of = cf ^ result31
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 8:
        {
            uint64_t res;
            uint64_t op1_64 = decode->op[0].val;

            if (!count)
                break;

            if (1 == count)
                res = (op1_64 << 1) | get_CF(cpu);
            else
                res = (op1_64 << count) | (get_CF(cpu) << (count - 1)) | (op1_64 >> (65 - count));

            write_val_ext(cpu, decode->op[0].ptr, res, 8);

            cf = (op1_64 >> (64 - count)) & 0x1;
            of = cf ^ (res >> 63); // of = cf ^ result63
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        default:
        {
            abort();
        }
    }
    RIP(cpu) += decode->len;
}

void exec_rcr(struct CPUState *cpu, struct x86_decode *decode)
{
    uint8_t count;
    int of = 0, cf = 0;

    fetch_operands(cpu, decode, 2, true, true, false);
    count = decode->op[1].val & 0x1f;

    switch(decode->operand_size) {
        case 1:
        {
            uint8_t op1_8 = decode->op[0].val;
            uint8_t res;

            count %= 9;
            if (!count)
                break;
            res = (op1_8 >> count) | (get_CF(cpu) << (8 - count)) | (op1_8 << (9 - count));

            write_val_ext(cpu, decode->op[0].ptr, res, 1);

            cf = (op1_8 >> (count - 1)) & 0x1;
            of = (((res << 1) ^ res) >> 7) & 0x1; // of = result6 ^ result7
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 2:
        {
            uint16_t op1_16 = decode->op[0].val;
            uint16_t res;

            count %= 17;
            if (!count)
                break;
            res = (op1_16 >> count) | (get_CF(cpu) << (16 - count)) | (op1_16 << (17 - count));

            write_val_ext(cpu, decode->op[0].ptr, res, 2);

            cf = (op1_16 >> (count - 1)) & 0x1;
            of = ((uint16_t)((res << 1) ^ res) >> 15) & 0x1; // of = result15 ^ result14
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 4:
        {
            uint32_t res;
            uint32_t op1_32 = decode->op[0].val;

            if (!count)
                break;
 
            if (1 == count)
                res = (op1_32 >> 1) | (get_CF(cpu) << 31);
            else
                res = (op1_32 >> count) | (get_CF(cpu) << (32 - count)) | (op1_32 << (33 - count));

            write_val_ext(cpu, decode->op[0].ptr, res, 4);

            cf = (op1_32 >> (count - 1)) & 0x1;
            of = ((res << 1) ^ res) >> 31; // of = result30 ^ result31
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        case 8:
        {
            uint64_t res;
            uint64_t op1_64 = decode->op[0].val;

            if (!count)
                break;
 
            if (1 == count)
                res = (op1_64 >> 1) | (get_CF(cpu) << 63);
            else
                res = (op1_64 >> count) | (get_CF(cpu) << (64 - count)) | (op1_64 << (65 - count));

            write_val_ext(cpu, decode->op[0].ptr, res, 8);

            cf = (op1_64 >> (count - 1)) & 0x1;
            of = ((res << 1) ^ res) >> 63; // of = result62 ^ result63
            SET_FLAGS_OxxxxC(cpu, of, cf);
            break;
        }
        default:
        {
            abort();
        }
    }
    RIP(cpu) += decode->len;
}

static void exec_xchg(struct CPUState *cpu, struct x86_decode *decode)
{
    fetch_operands(cpu, decode, 2, true, true, false);

    write_val_ext(cpu, decode->op[0].ptr, decode->op[1].val, decode->operand_size);
    write_val_ext(cpu, decode->op[1].ptr, decode->op[0].val, decode->operand_size);

    RIP(cpu) += decode->len;
}

static void exec_xadd(struct CPUState *cpu, struct x86_decode *decode)
{
    EXEC_2OP_ARITH_CMD(cpu, decode, +, SET_FLAGS_OSZAPC_ADD, true);
    write_val_ext(cpu, decode->op[1].ptr, decode->op[0].val, decode->operand_size);

    RIP(cpu) += decode->len;
}

static struct cmd_handler {
    enum x86_decode_cmd cmd;
    void (*handler)(struct CPUState *cpu, struct x86_decode *ins);
} handlers[] = {
    {X86_DECODE_CMD_INVL, NULL,},
    {X86_DECODE_CMD_MOV, exec_mov},
    {X86_DECODE_CMD_ADD, exec_add},
    {X86_DECODE_CMD_OR, exec_or},
    {X86_DECODE_CMD_ADC, exec_adc},
    {X86_DECODE_CMD_SBB, exec_sbb},
    {X86_DECODE_CMD_AND, exec_and},
    {X86_DECODE_CMD_SUB, exec_sub},
    {X86_DECODE_CMD_NEG, exec_neg},
    {X86_DECODE_CMD_XOR, exec_xor},
    {X86_DECODE_CMD_CMP, exec_cmp},
    {X86_DECODE_CMD_INC, exec_inc},
    {X86_DECODE_CMD_DEC, exec_dec},
    {X86_DECODE_CMD_TST, exec_tst},
    {X86_DECODE_CMD_NOT, exec_not},
    {X86_DECODE_CMD_MOVZX, exec_movzx},
    {X86_DECODE_CMD_OUT, exec_out},
    {X86_DECODE_CMD_IN, exec_in},
    {X86_DECODE_CMD_INS, exec_ins},
    {X86_DECODE_CMD_OUTS, exec_outs},
    {X86_DECODE_CMD_RDMSR, exec_rdmsr},
    {X86_DECODE_CMD_WRMSR, exec_wrmsr},
    {X86_DECODE_CMD_BT, exec_bt},
    {X86_DECODE_CMD_BTR, exec_btr},
    {X86_DECODE_CMD_BTC, exec_btc},
    {X86_DECODE_CMD_BTS, exec_bts},
    {X86_DECODE_CMD_SHL, exec_shl},
    {X86_DECODE_CMD_ROL, exec_rol},
    {X86_DECODE_CMD_ROR, exec_ror},
    {X86_DECODE_CMD_RCR, exec_rcr},
    {X86_DECODE_CMD_RCL, exec_rcl},
    /*{X86_DECODE_CMD_CPUID, exec_cpuid},*/
    {X86_DECODE_CMD_MOVS, exec_movs},
    {X86_DECODE_CMD_CMPS, exec_cmps},
    {X86_DECODE_CMD_STOS, exec_stos},
    {X86_DECODE_CMD_SCAS, exec_scas},
    {X86_DECODE_CMD_LODS, exec_lods},
    {X86_DECODE_CMD_MOVSX, exec_movsx},
    {X86_DECODE_CMD_XCHG, exec_xchg},
    {X86_DECODE_CMD_XADD, exec_xadd},
};

static struct cmd_handler _cmd_handler[X86_DECODE_CMD_LAST];

static void init_cmd_handler(CPUState *cpu)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(handlers); i++)
        _cmd_handler[handlers[i].cmd] = handlers[i];
}

static void print_debug(struct CPUState *cpu)
{
    printf("%llx: eax %llx ebx %llx ecx %llx edx %llx esi %llx edi %llx ebp %llx esp %llx flags %llx\n", RIP(cpu), RAX(cpu), RBX(cpu), RCX(cpu), RDX(cpu), RSI(cpu), RDI(cpu), RBP(cpu), RSP(cpu), EFLAGS(cpu));
}

void load_regs(struct CPUState *cpu)
{
    int i = 0;
    RRX(cpu, REG_RAX) = rreg(cpu->hvf_fd, HV_X86_RAX);
    RRX(cpu, REG_RBX) = rreg(cpu->hvf_fd, HV_X86_RBX);
    RRX(cpu, REG_RCX) = rreg(cpu->hvf_fd, HV_X86_RCX);
    RRX(cpu, REG_RDX) = rreg(cpu->hvf_fd, HV_X86_RDX);
    RRX(cpu, REG_RSI) = rreg(cpu->hvf_fd, HV_X86_RSI);
    RRX(cpu, REG_RDI) = rreg(cpu->hvf_fd, HV_X86_RDI);
    RRX(cpu, REG_RSP) = rreg(cpu->hvf_fd, HV_X86_RSP);
    RRX(cpu, REG_RBP) = rreg(cpu->hvf_fd, HV_X86_RBP);
    for (i = 8; i < 16; i++)
        RRX(cpu, i) = rreg(cpu->hvf_fd, HV_X86_RAX + i);
    
    RFLAGS(cpu) = rreg(cpu->hvf_fd, HV_X86_RFLAGS);
    rflags_to_lflags(cpu);
    RIP(cpu) = rreg(cpu->hvf_fd, HV_X86_RIP);

    //print_debug(cpu);
}

void store_regs(struct CPUState *cpu)
{
    int i = 0;
    wreg(cpu->hvf_fd, HV_X86_RAX, RAX(cpu));
    wreg(cpu->hvf_fd, HV_X86_RBX, RBX(cpu));
    wreg(cpu->hvf_fd, HV_X86_RCX, RCX(cpu));
    wreg(cpu->hvf_fd, HV_X86_RDX, RDX(cpu));
    wreg(cpu->hvf_fd, HV_X86_RSI, RSI(cpu));
    wreg(cpu->hvf_fd, HV_X86_RDI, RDI(cpu));
    wreg(cpu->hvf_fd, HV_X86_RBP, RBP(cpu));
    wreg(cpu->hvf_fd, HV_X86_RSP, RSP(cpu));
    for (i = 8; i < 16; i++)
        wreg(cpu->hvf_fd, HV_X86_RAX + i, RRX(cpu, i));
    
    lflags_to_rflags(cpu);
    wreg(cpu->hvf_fd, HV_X86_RFLAGS, RFLAGS(cpu));
    macvm_set_rip(cpu, RIP(cpu));

    //print_debug(cpu);
}

bool exec_instruction(struct CPUState *cpu, struct x86_decode *ins)
{
    //if (hvf_vcpu_id(cpu))
    //printf("%d, %llx: exec_instruction %s\n", hvf_vcpu_id(cpu),  RIP(cpu), decode_cmd_to_string(ins->cmd));
    
    if (0 && ins->is_fpu) {
        VM_PANIC("emulate fpu\n");
    } else {
        if (!_cmd_handler[ins->cmd].handler) {
            printf("Unimplemented handler (%llx) for %d (%x %x) \n", RIP(cpu), ins->cmd, ins->opcode[0],
                   ins->opcode_len > 1 ? ins->opcode[1] : 0);
            RIP(cpu) += ins->len;
            return true;
        }
        
        VM_PANIC_ON_EX(!_cmd_handler[ins->cmd].handler, "Unimplemented handler (%llx) for %d (%x %x) \n", RIP(cpu), ins->cmd, ins->opcode[0], ins->opcode_len > 1 ? ins->opcode[1] : 0);
        _cmd_handler[ins->cmd].handler(cpu, ins);
    }
    return true;
}

void init_emu(struct CPUState *cpu)
{
    init_cmd_handler(cpu);
}

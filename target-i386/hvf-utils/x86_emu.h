#ifndef __X86_EMU_H__
#define __X86_EMU_H__

#include "x86.h"
#include "x86_decode.h"

void init_emu(struct CPUState *cpu);
bool exec_instruction(struct CPUState *cpu, struct x86_decode *ins);

void load_regs(struct CPUState *cpu);
void store_regs(struct CPUState *cpu);

void simulate_rdmsr(struct CPUState *cpu);
void simulate_wrmsr(struct CPUState *cpu);

#endif

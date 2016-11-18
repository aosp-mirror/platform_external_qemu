#ifndef QEMU_MIPS_CPU_QOM_H
#define QEMU_MIPS_CPU_QOM_H

#include "qemu/osdep.h"
#include "qom/cpu.h"

typedef struct MIPSCPU {
    CPUState parent_obj;

    CPUMIPSState env;
} MIPSCPU;


static inline MIPSCPU *mips_env_get_cpu(CPUMIPSState *env)
{
    return container_of(env, MIPSCPU, env);
}

#define ENV_GET_CPU(e)  CPU(mips_env_get_cpu(e))
#define ENV_OFFSET offsetof(MIPSCPU, env)

#endif  // QEMU_MIPS_CPU_QOM_H

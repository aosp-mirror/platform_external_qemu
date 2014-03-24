#ifndef QEMU_ARM_CPU_QOM_H
#define QEMU_ARM_CPU_QOM_H

#include "qemu/osdep.h"
#include "qom/cpu.h"

typedef struct ARMCPU {
    CPUState parent_obj;

    CPUARMState env;
} ARMCPU;

static inline ARMCPU *arm_env_get_cpu(CPUARMState *env)
{
    return container_of(env, ARMCPU, env);
}

#define ENV_GET_CPU(e)  CPU(arm_env_get_cpu(e))
#define ENV_OFFSET offsetof(ARMCPU, env)

#endif  // QEMU_ARM_CPU_QOM_H

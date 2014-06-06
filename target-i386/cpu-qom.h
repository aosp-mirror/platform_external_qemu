#ifndef QEMU_X86_CPU_QOM_H
#define QEMU_X86_CPU_QOM_H

#include "qemu/osdep.h"
#include "qom/cpu.h"

typedef struct X86CPU {
    CPUState parent_obj;

    CPUX86State env;
} X86CPU;

static inline X86CPU *x86_env_get_cpu(CPUX86State *env)
{
    return container_of(env, X86CPU, env);
}

#define ENV_GET_CPU(e)  CPU(x86_env_get_cpu(e))
#define ENV_OFFSET offsetof(X86CPU, env)

#endif  // QEMU_X86_CPU_QOM_H

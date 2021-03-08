/*
 * QEMU GVM x86 specific function stubs
 *
 * Copyright Linaro Limited 2012
 *
 * Author: Peter Maydell <peter.maydell@linaro.org>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */
#include "qemu/osdep.h"
#include "qemu-common.h"
#include "cpu.h"
#include "gvm_i386.h"

#ifndef __OPTIMIZE__
/* This function is only called inside conditionals which we
 * rely on the compiler to optimize out when CONFIG_GVM is not
 * defined.
 */
uint32_t gvm_arch_get_supported_cpuid(GVMState *env, uint32_t function,
                                      uint32_t index, int reg)
{
    abort();
}
#endif

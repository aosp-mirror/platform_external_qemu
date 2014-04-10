#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/gdbstub.h"
#include "exec/def-helper.h"
#include "helper-android.h"
#include "qemu-common.h"

/* copy a string from the simulated virtual space to a buffer in QEMU */
void vstrcpy(target_ulong ptr, char *buf, int max)
{
    int  index;

    if (buf == NULL) return;

    for (index = 0; index < max; index += 1) {
        cpu_physical_memory_read(ptr + index, (uint8_t*)buf + index, 1);
        if (buf[index] == 0)
            break;
    }
}

#ifdef CONFIG_ANDROID_MEMCHECK
#include "android/qemu/memcheck/memcheck_api.h"

void HELPER(on_call)(target_ulong pc, target_ulong ret) {
    memcheck_on_call(pc, ret);
}

void HELPER(on_ret)(target_ulong ret) {
    memcheck_on_ret(ret);
}
#endif  // CONFIG_ANDROID_MEMCHECK

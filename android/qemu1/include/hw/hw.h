/* Declarations for use by hardware emulation.  */
#ifndef QEMU_HW_H
#define QEMU_HW_H

#include "qemu-common.h"
#include "hw/irq.h"
#include "migration/qemu-file.h"
#include "migration/vmstate.h"

#ifdef NEED_CPU_H
#include "cpu.h"
#endif


#ifdef CONFIG_ANDROID
void qemu_put_string(QEMUFile *f, const char* str);
char* qemu_get_string(QEMUFile *f);
#endif

#ifdef NEED_CPU_H
#if TARGET_LONG_BITS == 64
#define qemu_put_betl qemu_put_be64
#define qemu_get_betl qemu_get_be64
#define qemu_put_betls qemu_put_be64s
#define qemu_get_betls qemu_get_be64s
#define qemu_put_sbetl qemu_put_sbe64
#define qemu_get_sbetl qemu_get_sbe64
#define qemu_put_sbetls qemu_put_sbe64s
#define qemu_get_sbetls qemu_get_sbe64s
#else
#define qemu_put_betl qemu_put_be32
#define qemu_get_betl qemu_get_be32
#define qemu_put_betls qemu_put_be32s
#define qemu_get_betls qemu_get_be32s
#define qemu_put_sbetl qemu_put_sbe32
#define qemu_get_sbetl qemu_get_sbe32
#define qemu_put_sbetls qemu_put_sbe32s
#define qemu_get_sbetls qemu_get_sbe32s
#endif
#endif

typedef void QEMUResetHandler(void *opaque);

void qemu_register_reset(QEMUResetHandler *func, int order, void *opaque);

/* handler to set the boot_device for a specific type of QEMUMachine */
/* return 0 if success */
typedef int QEMUBootSetHandler(void *opaque, const char *boot_device);
void qemu_register_boot_set(QEMUBootSetHandler *func, void *opaque);

#endif

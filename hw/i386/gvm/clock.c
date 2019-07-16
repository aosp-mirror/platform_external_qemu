/*
 * QEMU GVM support, paravirtual clock device
 *
 * Copyright (C) 2011 Siemens AG
 *
 * Authors:
 *  Jan Kiszka        <jan.kiszka@siemens.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2.
 * See the COPYING file in the top-level directory.
 *
 * Contributions after 2012-01-13 are licensed under the terms of the
 * GNU GPL, version 2 or (at your option) any later version.
 */

#include "qemu/osdep.h"
#include "qemu-common.h"
#include "cpu.h"
#include "qemu/host-utils.h"
#include "sysemu/sysemu.h"
#include "sysemu/gvm.h"
#include "gvm_i386.h"
#include "hw/sysbus.h"
#include "hw/gvm/clock.h"

#define TYPE_GVM_CLOCK "gvmclock"
#define GVM_CLOCK(obj) OBJECT_CHECK(GVMClockState, (obj), TYPE_GVM_CLOCK)

typedef struct GVMClockState {
    /*< private >*/
    SysBusDevice busdev;
    /*< public >*/

    uint64_t clock;
    bool clock_valid;
} GVMClockState;

struct pvclock_vcpu_time_info {
    uint32_t   version;
    uint32_t   pad0;
    uint64_t   tsc_timestamp;
    uint64_t   system_time;
    uint32_t   tsc_to_system_mul;
    int8_t     tsc_shift;
    uint8_t    flags;
    uint8_t    pad[2];
} __attribute__((__packed__)); /* 32 bytes */

static uint64_t gvmclock_current_nsec(GVMClockState *s)
{
    CPUState *cpu = first_cpu;
    CPUX86State *env = cpu->env_ptr;
    hwaddr gvmclock_struct_pa = env->system_time_msr & ~1ULL;
    uint64_t migration_tsc = env->tsc;
    struct pvclock_vcpu_time_info time;
    uint64_t delta;
    uint64_t nsec_lo;
    uint64_t nsec_hi;
    uint64_t nsec;

    if (!(env->system_time_msr & 1ULL)) {
        /* GVM clock not active */
        return 0;
    }

    cpu_physical_memory_read(gvmclock_struct_pa, &time, sizeof(time));

    assert(time.tsc_timestamp <= migration_tsc);
    delta = migration_tsc - time.tsc_timestamp;
    if (time.tsc_shift < 0) {
        delta >>= -time.tsc_shift;
    } else {
        delta <<= time.tsc_shift;
    }

    mulu64(&nsec_lo, &nsec_hi, delta, time.tsc_to_system_mul);
    nsec = (nsec_lo >> 32) | (nsec_hi << 32);
    return nsec + time.system_time;
}

static void gvmclock_vm_state_change(void *opaque, int running,
                                     RunState state)
{
    GVMClockState *s = opaque;
    CPUState *cpu;
    int cap_clock_ctrl = gvm_check_extension(gvm_state, GVM_CAP_GVMCLOCK_CTRL);
    int ret;

    if (running) {
        struct gvm_clock_data data = {};
        uint64_t time_at_migration = gvmclock_current_nsec(s);

        s->clock_valid = false;

        /* We can't rely on the migrated clock value, just discard it */
        if (time_at_migration) {
            s->clock = time_at_migration;
        }

        data.clock = s->clock;
        ret = gvm_vm_ioctl(gvm_state, GVM_SET_CLOCK,
                &data, sizeof(data), NULL, 0);
        if (ret < 0) {
            fprintf(stderr, "GVM_SET_CLOCK failed: %s\n", strerror(ret));
            abort();
        }

        if (!cap_clock_ctrl) {
            return;
        }
        CPU_FOREACH(cpu) {
            ret = gvm_vcpu_ioctl(cpu, GVM_GVMCLOCK_CTRL,
                    NULL, 0, NULL, 0);
            if (ret) {
                if (ret != -EINVAL) {
                    fprintf(stderr, "%s: %s\n", __func__, strerror(-ret));
                }
                return;
            }
        }
    } else {
        struct gvm_clock_data data;
        int ret;

        if (s->clock_valid) {
            return;
        }

        gvm_synchronize_all_tsc();

        ret = gvm_vm_ioctl(gvm_state, GVM_GET_CLOCK,
                &data, sizeof(data), NULL, 0);
        if (ret < 0) {
            fprintf(stderr, "GVM_GET_CLOCK failed: %s\n", strerror(ret));
            abort();
        }
        s->clock = data.clock;

        /*
         * If the VM is stopped, declare the clock state valid to
         * avoid re-reading it on next vmsave (which would return
         * a different value). Will be reset when the VM is continued.
         */
        s->clock_valid = true;
    }
}

static void gvmclock_realize(DeviceState *dev, Error **errp)
{
    GVMClockState *s = GVM_CLOCK(dev);

    qemu_add_vm_change_state_handler(gvmclock_vm_state_change, s);
}

static const VMStateDescription gvmclock_vmsd = {
    .name = "gvmclock",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(clock, GVMClockState),
        VMSTATE_END_OF_LIST()
    }
};

static void gvmclock_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = gvmclock_realize;
    dc->vmsd = &gvmclock_vmsd;
}

static const TypeInfo gvmclock_info = {
    .name          = TYPE_GVM_CLOCK,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GVMClockState),
    .class_init    = gvmclock_class_init,
};

/* Note: Must be called after VCPU initialization. */
void gvmclock_create(void)
{
#if 0
    X86CPU *cpu = X86_CPU(first_cpu);

    if (gvm_enabled() &&
        cpu->env.features[FEAT_GVM] & ((1ULL << GVM_FEATURE_CLOCKSOURCE) |
                                       (1ULL << GVM_FEATURE_CLOCKSOURCE2))) {
        sysbus_create_simple(TYPE_GVM_CLOCK, -1, NULL);
    }
#endif
}

static void gvmclock_register_types(void)
{
    type_register_static(&gvmclock_info);
}

type_init(gvmclock_register_types)

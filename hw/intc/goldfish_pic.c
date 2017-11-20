/* Copyright (C) 2007-2008 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/ram_addr.h"
#include "migration/qemu-file.h"
#include "sysemu/char.h"
#include "exec/address-spaces.h"

#include "migration/qemu-file.h"
#include "hw/intc/goldfish_pic.h"
#include "hw/irq.h"
#include "hw/hw.h"
#include "hw/sysbus.h"

static qemu_irq* goldfish_pic = NULL;
static qemu_irq pic_parent_irq = NULL;
static qemu_irq pic_parent_fiq = NULL;

enum {
    INTERRUPT_STATUS        = 0x00, // number of pending interrupts
    INTERRUPT_NUMBER        = 0x04,
    INTERRUPT_DISABLE_ALL   = 0x08,
    INTERRUPT_DISABLE       = 0x0c,
    INTERRUPT_ENABLE        = 0x10
};

struct goldfish_int_state {
    SysBusDevice parent;

    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t level;
    uint32_t pending_count;
    uint32_t irq_enabled;
    uint32_t fiq_enabled;
    qemu_irq parent_irq;
    qemu_irq parent_fiq;
};

#define  GOLDFISH_INT_SAVE_VERSION  1

#define TYPE_GOLDFISH_PIC "goldfish_pic"
#define GOLDFISH_PIC(obj) OBJECT_CHECK(struct goldfish_int_state, (obj), TYPE_GOLDFISH_PIC)

static void goldfish_int_update(struct goldfish_int_state *s)
{
    uint32_t flags;
    bool locked = false;

    /* We may already have the BQL if coming from the reset path */
    if (!qemu_mutex_iothread_locked()) {
        locked = true;
        qemu_mutex_lock_iothread();
    }

    flags = (s->level & s->irq_enabled);
    qemu_set_irq(s->parent_irq, flags != 0);

    flags = (s->level & s->fiq_enabled);
    qemu_set_irq(s->parent_fiq, flags != 0);

    if (locked) {
        qemu_mutex_unlock_iothread();
    }
}

static int  goldfish_int_load(void* opaque, int  version_id)
{
    struct goldfish_int_state*  s = opaque;

    if (version_id != GOLDFISH_INT_SAVE_VERSION)
        return -1;

    goldfish_int_update(s);
    return 0;
}

static const VMStateDescription goldfish_pic_vmsd = {
    .name = "goldfish_pic",
    .version_id = GOLDFISH_INT_SAVE_VERSION,
    .minimum_version_id = GOLDFISH_INT_SAVE_VERSION,
    .minimum_version_id_old = GOLDFISH_INT_SAVE_VERSION,
    .post_load = &goldfish_int_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(level, struct goldfish_int_state),
        VMSTATE_UINT32(pending_count, struct goldfish_int_state),
        VMSTATE_UINT32(irq_enabled, struct goldfish_int_state),
        VMSTATE_UINT32(fiq_enabled, struct goldfish_int_state),
        VMSTATE_END_OF_LIST()
    }
};

static void goldfish_int_set_irq(void *opaque, int irq, int level)
{
    struct goldfish_int_state *s = (struct goldfish_int_state *)opaque;
    uint32_t mask = (1U << irq);

    if(level) {
        if(!(s->level & mask)) {
            if(s->irq_enabled & mask)
                s->pending_count++;
            s->level |= mask;
        }
    }
    else {
        if(s->level & mask) {
            if(s->irq_enabled & mask)
                s->pending_count--;
            s->level &= ~mask;
        }
    }
    goldfish_int_update(s);
}

static uint64_t goldfish_int_read(void *opaque, hwaddr offset, unsigned size)
{
    struct goldfish_int_state *s = (struct goldfish_int_state *)opaque;

    switch (offset) {
    case INTERRUPT_STATUS: /* IRQ_STATUS */ {
        return s->pending_count;
    }
    case INTERRUPT_NUMBER: {
        int i;
        uint32_t pending = s->level & s->irq_enabled;
        for(i = 0; i < 32; i++) {
            if(pending & (1U << i)) {
                return i;
            }
        }
        return 0;
    }
    default:
        cpu_abort(current_cpu,
                  "goldfish_int_read: Bad offset %" HWADDR_PRIx "\n",
                  offset);
        return 0;
    }
}

static void goldfish_int_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    struct goldfish_int_state *s = (struct goldfish_int_state *)opaque;

    uint32_t mask = (1U << value);

    switch (offset) {
        case INTERRUPT_DISABLE_ALL:
            s->pending_count = 0;
            s->level = 0;
            s->irq_enabled = 0;
            break;

        case INTERRUPT_DISABLE:
            if(s->irq_enabled & mask) {
                if(s->level & mask)
                    s->pending_count--;
                s->irq_enabled &= ~mask;
            }
            break;
        case INTERRUPT_ENABLE:
            if(!(s->irq_enabled & mask)) {
                s->irq_enabled |= mask;
                if(s->level & mask)
                    s->pending_count++;
            }
            break;

    default:
        cpu_abort(current_cpu,
                  "goldfish_int_write: Bad offset %" HWADDR_PRIx "\n",
                  offset);
        return;
    }
    goldfish_int_update(s);
}

static const MemoryRegionOps mips_qemu_int_ops = {
    .read = goldfish_int_read,
    .write = goldfish_int_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

qemu_irq*  goldfish_interrupt_init(uint32_t base, qemu_irq parent_irq, qemu_irq parent_fiq)
{
    pic_parent_irq = parent_irq;
    pic_parent_fiq = parent_fiq;
    sysbus_create_simple("goldfish_pic", base, NULL);

    return goldfish_pic;
}

static void goldfish_int_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct goldfish_int_state *s = GOLDFISH_PIC(dev);

    goldfish_pic = qemu_allocate_irqs(goldfish_int_set_irq, s, GFD_MAX_IRQ);
    s->parent_irq = pic_parent_irq;
    s->parent_fiq = pic_parent_fiq;

    memory_region_init_io(&s->iomem, OBJECT(s), &mips_qemu_int_ops, s,
            "goldfish_pic", 0x1000);
    sysbus_init_mmio(sbdev, &s->iomem);
}

static void goldfish_int_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_int_realize;
    dc->vmsd = &goldfish_pic_vmsd;
    dc->desc = "goldfish PIC";
}

static const TypeInfo goldfish_int_info = {
    .name          = TYPE_GOLDFISH_PIC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct goldfish_int_state),
    .class_init    = goldfish_int_class_init,
};

static void goldfish_int_register(void)
{
    type_register_static(&goldfish_int_info);
}

type_init(goldfish_int_register);

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
#include "exec/address-spaces.h"
#include "exec/ram_addr.h"
#include "hw/hw.h"
#include "hw/intc/goldfish_pic.h"
#include "hw/irq.h"
#include "hw/sysbus.h"
#include "migration/qemu-file.h"
#include "migration/qemu-file.h"
#include "chardev/char.h"

enum {
    /* Number of pending interrupts         */
    GFPIC_REG_IRQ_STATUS        = 0x00,
    /* Highest priority pending irq number  */
    GFPIC_REG_IRQ_PENDING       = 0x04,
    /* Disable all irq lines                */
    GFPIC_REG_IRQ_DISABLE_ALL   = 0x08,
    /* Mask specific interrupt lines        */
    GFPIC_REG_IRQ_DISABLE       = 0x0c,
    /* Unmask specific interrupt lines      */
    GFPIC_REG_IRQ_ENABLE        = 0x10
};

struct goldfish_pic_state {
    SysBusDevice parent;
    MemoryRegion iomem;

    uint32_t level;
    uint32_t pending_count;
    uint32_t irq_enabled;
    qemu_irq parent_irq;
};

#define  GOLDFISH_PIC_SAVE_VERSION  2

#define TYPE_GOLDFISH_PIC "goldfish_pic"
#define GOLDFISH_PIC(obj) \
OBJECT_CHECK(struct goldfish_pic_state, (obj), TYPE_GOLDFISH_PIC)

static void goldfish_pic_update(struct goldfish_pic_state *s)
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

    if (locked) {
        qemu_mutex_unlock_iothread();
    }
}

static int goldfish_pic_load(void *opaque, int  version_id)
{
    struct goldfish_pic_state *s = opaque;

    if (version_id != GOLDFISH_PIC_SAVE_VERSION) {
        return -1;
    }

    goldfish_pic_update(s);
    return 0;
}

static const VMStateDescription goldfish_pic_vmsd = {
    .name = "goldfish_pic",
    .version_id = GOLDFISH_PIC_SAVE_VERSION,
    .minimum_version_id = GOLDFISH_PIC_SAVE_VERSION,
    .minimum_version_id_old = GOLDFISH_PIC_SAVE_VERSION,
    .post_load = &goldfish_pic_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(level, struct goldfish_pic_state),
        VMSTATE_UINT32(pending_count, struct goldfish_pic_state),
        VMSTATE_UINT32(irq_enabled, struct goldfish_pic_state),
        VMSTATE_END_OF_LIST()
    }
};

static void goldfish_pic_set_irq(void *opaque, int irq, int level)
{
    struct goldfish_pic_state *s = (struct goldfish_pic_state *)opaque;
    uint32_t mask = (1U << irq);

    if (level) {
        if (!(s->level & mask)) {
            if (s->irq_enabled & mask) {
                s->pending_count++;
            }
            s->level |= mask;
        }
    } else {
        if (s->level & mask) {
            if (s->irq_enabled & mask) {
                s->pending_count--;
            }
            s->level &= ~mask;
        }
    }
    goldfish_pic_update(s);
}

static uint64_t goldfish_pic_read(void *opaque, hwaddr offset, unsigned size)
{
    struct goldfish_pic_state *s = (struct goldfish_pic_state *)opaque;

    switch (offset) {
    case GFPIC_REG_IRQ_STATUS:
        /* return number of pending interrupts */
        return s->pending_count;

    case GFPIC_REG_IRQ_PENDING:
        /* return pending interrupts mask */
        return s->level & s->irq_enabled;

    default:
        cpu_abort(current_cpu,
                  "goldfish_pic_read: Bad offset %" HWADDR_PRIx "\n",
                  offset);
        return 0;
    }
}

static void goldfish_pic_write(void *opaque, hwaddr offset,
                               uint64_t value, unsigned size)
{
    struct goldfish_pic_state *s = (struct goldfish_pic_state *)opaque;

    uint32_t mask = value;

    switch (offset) {
    case GFPIC_REG_IRQ_DISABLE_ALL:
        s->pending_count = 0;
        s->level = 0;
        s->irq_enabled = 0;
        break;

    case GFPIC_REG_IRQ_DISABLE:
        if (s->irq_enabled & mask) {
            if (s->level & mask) {
                s->pending_count--;
            }
            s->irq_enabled &= ~mask;
        }
        break;

    case GFPIC_REG_IRQ_ENABLE:
        if (!(s->irq_enabled & mask)) {
            s->irq_enabled |= mask;
            if (s->level & mask) {
                s->pending_count++;
            }
        }
        break;

    default:
        cpu_abort(current_cpu,
                  "goldfish_pic_write: Bad offset %" HWADDR_PRIx "\n",
                  offset);
        return;
    }
    goldfish_pic_update(s);
}

static const MemoryRegionOps goldfish_pic_ops = {
    .read = goldfish_pic_read,
    .write = goldfish_pic_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

qemu_irq *goldfish_pic_init(uint32_t base, qemu_irq parent_irq)
{
    struct goldfish_pic_state *s;
    DeviceState *dev;

    dev = sysbus_create_simple(TYPE_GOLDFISH_PIC, base, NULL);

    s = GOLDFISH_PIC(dev);
    s->parent_irq = parent_irq;

    return qemu_allocate_irqs(goldfish_pic_set_irq, s, GFD_MAX_IRQ);
}

static void goldfish_pic_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct goldfish_pic_state *s = GOLDFISH_PIC(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &goldfish_pic_ops, s,
            TYPE_GOLDFISH_PIC, 0x1000);
    sysbus_init_mmio(sbdev, &s->iomem);
}

static void goldfish_pic_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_pic_realize;
    dc->vmsd = &goldfish_pic_vmsd;
    dc->desc = "goldfish PIC";
}

static const TypeInfo goldfish_pic_info = {
    .name          = TYPE_GOLDFISH_PIC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct goldfish_pic_state),
    .class_init    = goldfish_pic_class_init,
};

static void goldfish_pic_register(void)
{
    type_register_static(&goldfish_pic_info);
}

type_init(goldfish_pic_register);

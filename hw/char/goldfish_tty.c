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
#include "qemu/rcu.h"
#include "qapi/error.h"
#include "cpu.h"
#include "exec/cpu-all.h"
#include "exec/memory.h"
#include "exec/ram_addr.h"
#include "migration/qemu-file.h"
#include "chardev/char-fe.h"
#include "hw/hw.h"
#include "exec/address-spaces.h"
#include "hw/sysbus.h"
#include "sysemu/sysemu.h"

#define TTY_DEVICE_VERSION 1

enum {
    TTY_PUT_CHAR       = 0x00,
    TTY_BYTES_READY    = 0x04,
    TTY_CMD            = 0x08,

    TTY_DATA_PTR       = 0x10,
    TTY_DATA_LEN       = 0x14,
    TTY_DATA_PTR_HIGH  = 0x18,

    TTY_VERSION        = 0x20,

    TTY_CMD_INT_DISABLE    = 0,
    TTY_CMD_INT_ENABLE     = 1,
    TTY_CMD_WRITE_BUFFER   = 2,
    TTY_CMD_READ_BUFFER    = 3,
};

struct tty_state {
    SysBusDevice parent;
    CharBackend chr_be;

    MemoryRegion iomem;
    qemu_irq irq;

    Chardev *cs;
    uint64_t ptr;
    uint32_t ptr_len;
    uint32_t ready;
    uint8_t data[128];
    uint32_t data_count;
    uint32_t index;
};

#define  GOLDFISH_TTY_SAVE_VERSION  2

#define TYPE_GOLDFISH_TTY "goldfish_tty"
#define GOLDFISH_TTY(obj) OBJECT_CHECK(struct tty_state, (obj), TYPE_GOLDFISH_TTY)

static const VMStateDescription vmstate_goldfish_tty = {
    .name = "goldfish_tty",
    .version_id = GOLDFISH_TTY_SAVE_VERSION,
    .minimum_version_id = GOLDFISH_TTY_SAVE_VERSION,
    .minimum_version_id_old = GOLDFISH_TTY_SAVE_VERSION,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(ptr,         struct tty_state),
        VMSTATE_UINT32(ptr_len,     struct tty_state),
        VMSTATE_UINT32(ready,       struct tty_state),
        VMSTATE_UINT32(data_count,  struct tty_state),
        VMSTATE_UINT32(index,       struct tty_state),
        VMSTATE_UINT8_ARRAY(data,   struct tty_state, 128),
        VMSTATE_END_OF_LIST()
    }
};

static Property goldfish_tty_properties[] = {
    DEFINE_PROP_UINT32("index",  struct tty_state, index,   -1),
    DEFINE_PROP_CHR("chardev",   struct tty_state, chr_be),
    DEFINE_PROP_END_OF_LIST(),
};

/* Number of instantiated TTYs */
static int  instance_id = 0;

static uint64_t goldfish_tty_read(void *opaque, hwaddr offset, unsigned size)
{
    struct tty_state *s = (struct tty_state *)opaque;

    switch (offset) {
        case TTY_BYTES_READY:
            return s->data_count;
        case TTY_VERSION:
            return TTY_DEVICE_VERSION;
    default:
        cpu_abort(current_cpu,
                  "goldfish_tty_read: Bad offset %" HWADDR_PRIx "\n",
                  offset);
        return 0;
    }
}

static void goldfish_tty_write(void *opaque, hwaddr offset,
                               uint64_t value, unsigned size)
{
    struct tty_state *s = (struct tty_state *)opaque;

    switch(offset) {
        case TTY_PUT_CHAR: {
            uint8_t ch = value;
            if(s->cs)
                qemu_chr_fe_write(s->cs->be, &ch, 1);
        } break;

        case TTY_CMD:
            switch(value) {
                case TTY_CMD_INT_DISABLE:
                    if(s->ready) {
                        if(s->data_count > 0)
                            qemu_set_irq(s->irq, 0);
                        s->ready = 0;
                    }
                    break;

                case TTY_CMD_INT_ENABLE:
                    if(!s->ready) {
                        if(s->data_count > 0)
                            qemu_set_irq(s->irq, 1);
                        s->ready = 1;
                    }
                    break;

                case TTY_CMD_WRITE_BUFFER:
                    if(s->cs) {
                        hwaddr l = s->ptr_len;
                        void *ptr;

                        ptr = cpu_physical_memory_map(s->ptr, &l, 0);
                        qemu_chr_fe_write(s->cs->be, (const uint8_t*)ptr, l);
                        cpu_physical_memory_unmap(ptr, l, 0, 0);
                    }
                    break;

                case TTY_CMD_READ_BUFFER:
                    {
                        hwaddr l = s->ptr_len;
                        void *ptr;

                        if(s->ptr_len > s->data_count)
                            cpu_abort(current_cpu,
                                      "goldfish_tty_write: reading"
                                      " more data than available %d %d\n",
                                      s->ptr_len, s->data_count);

                        ptr = cpu_physical_memory_map(s->ptr, &l, 1);
                        memcpy(ptr, s->data, l);
                        cpu_physical_memory_unmap(ptr, l, 1, l);

                        if(s->data_count > l)
                            memmove(s->data, s->data + l, s->data_count - l);
                        s->data_count -= l;
                        if(s->data_count == 0 && s->ready)
                            qemu_set_irq(s->irq, 0);
                    }
                    break;

                default:
                    cpu_abort(current_cpu,
                              "goldfish_tty_write: Bad command %" PRIx64 "\n",
                              value);
            };
            break;

        case TTY_DATA_PTR:
#if defined(TARGET_MIPS64)
            s->ptr = (int32_t)deposit64(s->ptr, 0, 32, value);
#else
            s->ptr = deposit64(s->ptr, 0, 32, value);
#endif
            break;

        case TTY_DATA_PTR_HIGH:
            s->ptr = deposit64(s->ptr, 32, 32, value);
            break;

        case TTY_DATA_LEN:
            s->ptr_len = value;
            break;

        default:
            cpu_abort(current_cpu,
                      "goldfish_tty_write: Bad offset %" HWADDR_PRIx "\n",
                      offset);
    }
}

static int tty_can_receive(void *opaque)
{
    struct tty_state *s = opaque;

    return (sizeof(s->data) - s->data_count);
}

static void tty_receive(void *opaque, const uint8_t *buf, int size)
{
    struct tty_state *s = opaque;

    memcpy(s->data + s->data_count, buf, size);
    s->data_count += size;
    if(s->data_count > 0 && s->ready)
        qemu_set_irq(s->irq, 1);
}

static const MemoryRegionOps mips_qemu_ops = {
    .read = goldfish_tty_read,
    .write = goldfish_tty_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void goldfish_tty_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct tty_state *s = GOLDFISH_TTY(dev);

    if ((instance_id + 1) == MAX_SERIAL_PORTS) {
        cpu_abort(current_cpu,
                  "goldfish_tty: MAX_SERIAL_PORTS(%d) reached\n",
                  MAX_SERIAL_PORTS);
    }

    s->index = instance_id;

    memory_region_init_io(&s->iomem, OBJECT(s), &mips_qemu_ops, s,
            TYPE_GOLDFISH_TTY, 0x1000);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    if (serial_hds[s->index]) {
        s->cs = serial_hds[s->index];
        s->cs->be = NULL;

        qemu_chr_fe_init(&s->chr_be, s->cs, &error_abort);
        qemu_chr_fe_set_handlers(s->cs->be, tty_can_receive,
                                 tty_receive, NULL, NULL, s, NULL, false);
    }

    instance_id++;
}

static void goldfish_tty_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_tty_realize;
    dc->vmsd = &vmstate_goldfish_tty;
    dc->props = goldfish_tty_properties;
    dc->desc = "goldfish tty";
}

static const TypeInfo goldfish_tty_info = {
    .name          = TYPE_GOLDFISH_TTY,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct tty_state),
    .class_init    = goldfish_tty_class_init,
};

static void goldfish_tty_register(void)
{
    type_register_static(&goldfish_tty_info);
}

type_init(goldfish_tty_register);

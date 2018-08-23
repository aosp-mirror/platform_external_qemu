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
#include "qemu-common.h"
#include "qemu/timer.h"
#include "cpu.h"
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "migration/register.h"

enum {
    TIMER_TIME_LOW          = 0x00, /* get low bits of current time        */
                                    /* and update TIMER_TIME_HIGH          */
    TIMER_TIME_HIGH         = 0x04, /* get high bits of time at            */
                                    /* last TIMER_TIME_LOW read            */
    TIMER_ALARM_LOW         = 0x08, /* set low bits of alarm               */
                                    /* and activate it                     */
    TIMER_ALARM_HIGH        = 0x0c, /* set high bits of next alarm         */
    TIMER_IRQ_ENABLED       = 0x10,
    TIMER_CLEAR_ALARM       = 0x14,
    TIMER_ALARM_STATUS      = 0x18,
    TIMER_CLEAR_INTERRUPT   = 0x1c
};

struct rtc_state {
    SysBusDevice parent;

    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t alarm_low_ns;
    int32_t alarm_high_ns;
    int64_t time_base;
    int64_t now_ns;
    char irq_enabled;
    char armed;
    QEMUTimer *timer;
};

#define  GOLDFISH_RTC_SAVE_VERSION  1

#define TYPE_GOLDFISH_RTC "goldfish_rtc"
#define GOLDFISH_RTC(obj) \
        OBJECT_CHECK(struct rtc_state, (obj), TYPE_GOLDFISH_RTC)

static void goldfish_rtc_save(QEMUFile *f, void *opaque)
{
    struct rtc_state *s = opaque;

    qemu_put_be64(f, s->now_ns);
    /* in case the kernel is in the middle of a timer read */
    qemu_put_byte(f, s->armed);
    if (s->armed) {
        int64_t now_ns   = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        int64_t alarm_ns = (s->alarm_low_ns | (int64_t)s->alarm_high_ns << 32);
        qemu_put_be64(f, alarm_ns - now_ns);
    }
}

static int goldfish_rtc_load(QEMUFile *f, void *opaque, int version_id)
{
    struct rtc_state *s = opaque;

    if (version_id != GOLDFISH_RTC_SAVE_VERSION) {
        return -1;
    }

    s->now_ns = qemu_get_be64(f);
    s->armed  = qemu_get_byte(f);
    if (s->armed) {
        int64_t now_tks   = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        int64_t diff_tks  = qemu_get_be64(f);
        int64_t alarm_tks = now_tks + diff_tks;

        if (alarm_tks <= now_tks) {
            qemu_set_irq(s->irq, 1);
            s->armed = 0;
        } else {
            qemu_set_irq(s->irq, 0);
            timer_mod(s->timer, alarm_tks);
        }
    }
    return 0;
}

static uint64_t goldfish_rtc_read(void *opaque, hwaddr offset, unsigned size)
{
    struct rtc_state *s = (struct rtc_state *)opaque;
    switch (offset) {
    case TIMER_TIME_LOW:
        s->now_ns = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + s->time_base;
        return s->now_ns;
    case TIMER_TIME_HIGH:
        return s->now_ns >> 32;
    case TIMER_ALARM_LOW:
        return s->alarm_low_ns;
    case TIMER_ALARM_HIGH:
        return s->alarm_high_ns;
    case TIMER_ALARM_STATUS:
        return s->armed;
    default:
        cpu_abort(current_cpu,
                  "goldfish_rtc_read: Bad offset %" HWADDR_PRIx "\n",
                  offset);
        return 0;
    }
}

static void goldfish_rtc_write(void *opaque, hwaddr offset, uint64_t value,
                               unsigned size)
{
    struct rtc_state *s = (struct rtc_state *)opaque;
    int64_t alarm_ns, now_ns;
    switch (offset) {
    case TIMER_ALARM_LOW:
        s->alarm_low_ns = value;
        alarm_ns = (s->alarm_low_ns | (int64_t)s->alarm_high_ns << 32);
        now_ns = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + s->time_base;
        if (alarm_ns <= now_ns) {
            if (s->irq_enabled) {
                qemu_set_irq(s->irq, 1);
            }
        } else {
            timer_mod(s->timer, alarm_ns - s->time_base);
            s->armed = 1;
            s->irq_enabled = 1;
        }
        break;
    case TIMER_ALARM_HIGH:
        s->alarm_high_ns = value;
        break;
    case TIMER_TIME_LOW:
        now_ns = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        s->time_base = (((int64_t)s->time_base << 32) | value) - now_ns;
        break;
    case TIMER_TIME_HIGH:
        s->time_base = value;
        break;
    case TIMER_CLEAR_ALARM:
        timer_del(s->timer);
        s->armed = 0;
        /* fall through */
    case TIMER_CLEAR_INTERRUPT:
        qemu_set_irq(s->irq, 0);
        break;
    case TIMER_IRQ_ENABLED:
        s->irq_enabled = value ? 1 : 0;
        break;
    default:
        cpu_abort(current_cpu,
                  "goldfish_rtc_write: Bad offset %" HWADDR_PRIx "\n",
                  offset);
    }
}

static void goldfish_rtc_tick(void *opaque)
{
    struct rtc_state *s = (struct rtc_state *)opaque;
    s->armed = 0;
    if (s->irq_enabled) {
        qemu_set_irq(s->irq, 1);
    }
}

static const MemoryRegionOps mips_qemu_timer_ops = {
    .read = goldfish_rtc_read,
    .write = goldfish_rtc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};
static SaveVMHandlers goldfish_rtc_vmhandlers = {
    .save_state = goldfish_rtc_save,
    .load_state = goldfish_rtc_load
};

static void goldfish_rtc_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct rtc_state *s = GOLDFISH_RTC(dev);

    s->timer = timer_new(QEMU_CLOCK_VIRTUAL, SCALE_NS, goldfish_rtc_tick, s);

    memory_region_init_io(&s->iomem, OBJECT(s), &mips_qemu_timer_ops, s,
            "goldfish_rtc", 0x1000);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);
    register_savevm_live(NULL,
                    "goldfish_rtc",
                    0,
                    GOLDFISH_RTC_SAVE_VERSION,
                    &goldfish_rtc_vmhandlers,
                    s);
}

static void goldfish_rtc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_rtc_realize;
    dc->desc = "goldfish RTC";
}

static const TypeInfo goldfish_rtc_info = {
    .name          = TYPE_GOLDFISH_RTC,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct rtc_state),
    .class_init    = goldfish_rtc_class_init,
};

static void goldfish_rtc_register(void)
{
    type_register_static(&goldfish_rtc_info);
}

type_init(goldfish_rtc_register);

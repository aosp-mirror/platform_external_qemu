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

enum {
    TIMER_TIME_LOW          = 0x00, // get low bits of current time and update TIMER_TIME_HIGH
    TIMER_TIME_HIGH         = 0x04, // get high bits of time at last TIMER_TIME_LOW read
    TIMER_ALARM_LOW         = 0x08, // set low bits of alarm and activate it
    TIMER_ALARM_HIGH        = 0x0c, // set high bits of next alarm
    TIMER_CLEAR_INTERRUPT   = 0x10,
    TIMER_CLEAR_ALARM       = 0x14
};

struct timer_state {
    SysBusDevice parent;

    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t alarm_low_ns;
    int32_t alarm_high_ns;
    int64_t now_ns;
    char armed;
    QEMUTimer *timer;
};

#define  GOLDFISH_TIMER_SAVE_VERSION  1

#define TYPE_GOLDFISH_TIMER "goldfish_timer"
#define GOLDFISH_TIMER(obj) OBJECT_CHECK(struct timer_state, (obj), TYPE_GOLDFISH_TIMER)

static void  goldfish_timer_save(QEMUFile*  f, void*  opaque)
{
    struct timer_state* s = opaque;

    qemu_put_be64(f, s->now_ns);  /* in case the kernel is in the middle of a timer read */
    qemu_put_byte(f, s->armed);
    if (s->armed) {
        int64_t  now_ns   = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        int64_t  alarm_ns = (s->alarm_low_ns | (int64_t)s->alarm_high_ns << 32);
        qemu_put_be64(f, alarm_ns - now_ns);
    }
}

static int  goldfish_timer_load(QEMUFile*  f, void*  opaque, int  version_id)
{
    struct timer_state*  s   = opaque;

    if (version_id != GOLDFISH_TIMER_SAVE_VERSION)
        return -1;

    s->now_ns = qemu_get_be64(f);
    s->armed  = qemu_get_byte(f);
    if (s->armed) {
        int64_t  now_tks   = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        int64_t  diff_tks  = qemu_get_be64(f);
        int64_t  alarm_tks = now_tks + diff_tks;

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

static uint64_t goldfish_timer_read(void *opaque, hwaddr offset, unsigned size)
{
    struct timer_state *s = (struct timer_state *)opaque;
    switch(offset) {
        case TIMER_TIME_LOW:
            s->now_ns = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
            return s->now_ns;
        case TIMER_TIME_HIGH:
            return s->now_ns >> 32;
        default:
            cpu_abort(current_cpu,
                      "goldfish_timer_read: Bad offset %" HWADDR_PRIx "\n",
                      offset);
            return 0;
    }
}

static void goldfish_timer_write(void *opaque, hwaddr offset, uint64_t value_ns, unsigned size)
{
    struct timer_state *s = (struct timer_state *)opaque;
    int64_t alarm_ns, now_ns;
    switch(offset) {
        case TIMER_ALARM_LOW:
            s->alarm_low_ns = value_ns;
            alarm_ns = (s->alarm_low_ns | (int64_t)s->alarm_high_ns << 32);
            now_ns   = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
            if (alarm_ns <= now_ns) {
                qemu_set_irq(s->irq, 1);
            } else {
                timer_mod(s->timer, alarm_ns);
                s->armed = 1;
            }
            break;
        case TIMER_ALARM_HIGH:
            s->alarm_high_ns = value_ns;
            break;
        case TIMER_CLEAR_ALARM:
            timer_del(s->timer);
            s->armed = 0;
            /* fall through */
        case TIMER_CLEAR_INTERRUPT:
            qemu_set_irq(s->irq, 0);
            break;
        default:
            cpu_abort(current_cpu,
                      "goldfish_timer_write: Bad offset %" HWADDR_PRIx "\n",
                      offset);
    }
}

static void goldfish_timer_tick(void *opaque)
{
    struct timer_state *s = (struct timer_state *)opaque;
    s->armed = 0;
    qemu_set_irq(s->irq, 1);
}

static const MemoryRegionOps mips_qemu_timer_ops = {
    .read = goldfish_timer_read,
    .write = goldfish_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void goldfish_timer_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct timer_state *s = GOLDFISH_TIMER(dev);

    s->timer = timer_new(QEMU_CLOCK_VIRTUAL, SCALE_NS, goldfish_timer_tick, s);

    memory_region_init_io(&s->iomem, OBJECT(s), &mips_qemu_timer_ops, s,
            "goldfish_timer", 0x1000);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);
    register_savevm(NULL,
                    "goldfish_timer",
                    0,
                    GOLDFISH_TIMER_SAVE_VERSION,
                    goldfish_timer_save,
                    goldfish_timer_load,
                    s);
}

static void goldfish_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_timer_realize;
    dc->desc = "goldfish timer";
}

static const TypeInfo goldfish_timer_info = {
    .name          = TYPE_GOLDFISH_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct timer_state),
    .class_init    = goldfish_timer_class_init,
};

static void goldfish_timer_register(void)
{
    type_register_static(&goldfish_timer_info);
}

type_init(goldfish_timer_register);

/*****************************************************************************
 * Goldfish Real Time Clock device implementation
 *****************************************************************************/

struct rtc_state {
    SysBusDevice parent;

    MemoryRegion iomem;
    qemu_irq irq;

    uint32_t alarm_low;
    int32_t alarm_high;
    int64_t now;
};

/* we save the RTC for the case where the kernel is in the middle of a rtc_read
 * (i.e. it has read the low 32-bit of s->now, but not the high 32-bits yet */
#define  GOLDFISH_RTC_SAVE_VERSION  1

#define TYPE_GOLDFISH_RTC "goldfish_rtc"
#define GOLDFISH_RTC(obj) OBJECT_CHECK(struct rtc_state, (obj), TYPE_GOLDFISH_RTC)

static void  goldfish_rtc_save(QEMUFile*  f, void*  opaque)
{
    struct rtc_state*  s = opaque;

    qemu_put_be64(f, s->now);
}

static int  goldfish_rtc_load(QEMUFile*  f, void*  opaque, int  version_id)
{
    struct  rtc_state*  s = opaque;

    if (version_id != GOLDFISH_RTC_SAVE_VERSION)
        return -1;

    /* this is an old value that is not correct. but that's ok anyway */
    s->now = qemu_get_be64(f);
    return 0;
}

static uint64_t goldfish_rtc_read(void *opaque, hwaddr offset, unsigned size)
{
    struct rtc_state *s = (struct rtc_state *)opaque;
    switch(offset) {
        case 0x0:
            s->now = (int64_t)time(NULL) * 1000000000;
            return s->now;
        case 0x4:
            return s->now >> 32;
        default:
            cpu_abort(current_cpu,
                      "goldfish_rtc_read: Bad offset %" HWADDR_PRIx "\n",
                      offset);
            return 0;
    }
}

static void goldfish_rtc_write(void *opaque, hwaddr offset, uint64_t value, unsigned size)
{
    struct rtc_state *s = (struct rtc_state *)opaque;
    switch(offset) {
        case 0x8:
            s->alarm_low = value;
            break;
        case 0xc:
            s->alarm_high = value;
            break;
        case 0x10:
            qemu_set_irq(s->irq, 0);
            break;
        default:
            cpu_abort(current_cpu,
                      "goldfish_rtc_write: Bad offset %" HWADDR_PRIx "\n",
                      offset);
    }
}

static const MemoryRegionOps mips_qemu_rtc_ops = {
    .read = goldfish_rtc_read,
    .write = goldfish_rtc_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void goldfish_rtc_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct rtc_state *s = GOLDFISH_RTC(dev);

    memory_region_init_io(&s->iomem, OBJECT(s), &mips_qemu_rtc_ops, s,
            "goldfish_rtc", 0x1000);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);
    register_savevm(NULL,
                    "goldfish_rtc",
                    0,
                    GOLDFISH_RTC_SAVE_VERSION,
                    goldfish_rtc_save,
                    goldfish_rtc_load,
                    s);
}

static void goldfish_rtc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_rtc_realize;
    dc->desc = "goldfish battery";
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

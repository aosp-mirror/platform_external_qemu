/*
 * Goldfish 'events' device model, used for a rotary encoder device.
 *
 * Copyright (C) 2007-2008 The Android Open Source Project
 * Copyright (c) 2014 Linaro Limited
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/sysbus.h"
#include "ui/input.h"
#include "ui/console.h"
#include "hw/input/android_keycodes.h"
#include "hw/input/linux_keycodes.h"
#include "hw/input/goldfish_events_common.h"

/* NOTE: The ev_bits arrays are used to indicate to the kernel
 *       which events can be sent by the emulated hardware.
 */

#define TYPE_ROTARYEVDEV "goldfish_rotary"

/* Pointer to the global device instance. Also serves as an initialization
 * flag in goldfish_rotary_send_rotate() to filter-out events that are sent from
 * the UI before the device was properly realized.
 */
static GoldfishEvDevState* s_evdev = NULL;

static const VMStateDescription vmstate_rotary_evdev =
        GOLDFISHEVDEV_VM_STATE_DESCRIPTION("goldfish_rotary");

int goldfish_rotary_send_rotate(int value)
{
    GoldfishEvDevState *dev = s_evdev;

    if (dev) {
        goldfish_enqueue_event(dev, EV_REL, REL_WHEEL, value);
        goldfish_enqueue_event(dev, 0, 0, value);
    }

    return 0;
}

static const MemoryRegionOps rotary_evdev_ops = {
    .read = goldfish_events_read,
    .write = goldfish_events_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void rotary_evdev_init(Object *obj)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(obj, TYPE_ROTARYEVDEV);
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &rotary_evdev_ops, s,
                          "goldfish_rotary", 0x1000);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void rotary_evdev_realize(DeviceState *dev, Error **errp)
{

    GoldfishEvDevState *s = GOLDFISHEVDEV(dev, TYPE_ROTARYEVDEV);

    /* Initialize the device ID so the event dev can be looked up during
     * monitor commands.
     */
    dev->id = g_strdup(TYPE_ROTARYEVDEV);

    /* now set the events capability bits depending on hardware configuration */
    /* apparently, the EV_SYN array is used to indicate which other
     * event classes to consider.
     */

    s->name = "goldfish_rotary";

    /* configure EV_KEY array */
    goldfish_events_set_bit(s, EV_SYN, EV_KEY);

    /* configure EV_REL array */
    goldfish_events_set_bit(s, EV_SYN, EV_REL);
    goldfish_events_set_bit(s, EV_REL, REL_WHEEL);

    /* Register global variable. */
    assert(s_evdev == NULL);
    assert(s->state == 0);
    s_evdev = s;
}

static void rotary_evdev_reset(DeviceState *dev)
{
    GoldfishEvDevState *s = GOLDFISHEVDEV(dev, TYPE_ROTARYEVDEV);

    s->first = 0;
    s->last = 0;
}

static void rotary_evdev_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = rotary_evdev_realize;
    dc->reset = rotary_evdev_reset;
    dc->vmsd = &vmstate_rotary_evdev;
}

static const TypeInfo rotary_evdev_info = {
    .name = TYPE_ROTARYEVDEV,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(GoldfishEvDevState),
    .instance_init = rotary_evdev_init,
    .class_init = rotary_evdev_class_init,
};

static void rotary_evdev_register_types(void)
{
    type_register_static(&rotary_evdev_info);
}

type_init(rotary_evdev_register_types)

/* Copyright (C) 2007-2015 The Android Open Source Project
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
#include "hw/hw.h"
#include "hw/sysbus.h"
#include "monitor/monitor.h"
#include "hw/misc/goldfish_battery.h"

static int sBatteryIsRealized = 0;
static int sDeviceHasBattery = 0;

#include <assert.h>

enum {
    /* status register */
    BATTERY_INT_STATUS = 0x00,
    /* set this to enable IRQ */
    BATTERY_INT_ENABLE = 0x04,
    BATTERY_AC_ONLINE = 0x08,
    BATTERY_STATUS = 0x0C,
    BATTERY_HEALTH = 0x10,
    BATTERY_PRESENT = 0x14,
    BATTERY_CAPACITY = 0x18,
    BATTERY_VOLTAGE = 0x1C,
    BATTERY_TEMP = 0x20,
    BATTERY_CHARGE_COUNTER = 0x24,
    BATTERY_VOLTAGE_MAX = 0x28,
    BATTERY_CURRENT_MAX = 0x2C,
    BATTERY_CURRENT_NOW = 0x30,
    BATTERY_CURRENT_AVG = 0x34,
    BATTERY_CHARGE_FULL_UAH = 0x38,
    BATTERY_CYCLE_COUNT = 0x40,

    BATTERY_STATUS_CHANGED = 1U << 0,
    AC_STATUS_CHANGED = 1U << 1,
    BATTERY_INT_MASK = BATTERY_STATUS_CHANGED | AC_STATUS_CHANGED,
};

#define TYPE_GOLDFISH_BATTERY "goldfish_battery"
#define GOLDFISH_BATTERY(obj) OBJECT_CHECK(struct goldfish_battery_state, (obj), TYPE_GOLDFISH_BATTERY)

struct goldfish_battery_state {
    SysBusDevice parent;

    MemoryRegion iomem;
    qemu_irq irq;

    // IRQs
    uint32_t int_status;
    // irq enable mask for int_status
    uint32_t int_enable;

    uint32_t ac_online;
    uint32_t status;
    uint32_t health;
    uint32_t present;
    uint32_t capacity;
    uint32_t hw_has_battery;
    uint32_t voltage;
    uint32_t temp;
    uint32_t charge_counter;
    uint32_t voltage_max;
    uint32_t current_max;
    uint32_t current_now;
    uint32_t current_avg;
    uint32_t charge_full_uah;
    uint32_t cycle_count;
};

/* update this each time you update the battery_state struct */
#define  BATTERY_STATE_SAVE_VERSION  1

static const VMStateDescription goldfish_battery_vmsd = {
    .name = "goldfish_battery",
    .version_id = BATTERY_STATE_SAVE_VERSION,
    .minimum_version_id = BATTERY_STATE_SAVE_VERSION,
    .minimum_version_id_old = BATTERY_STATE_SAVE_VERSION,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(int_status, struct goldfish_battery_state),
        VMSTATE_UINT32(int_enable, struct goldfish_battery_state),
        VMSTATE_UINT32(ac_online, struct goldfish_battery_state),
        VMSTATE_UINT32(status, struct goldfish_battery_state),
        VMSTATE_UINT32(health, struct goldfish_battery_state),
        VMSTATE_UINT32(present, struct goldfish_battery_state),
        VMSTATE_UINT32(capacity, struct goldfish_battery_state),
        VMSTATE_UINT32(hw_has_battery, struct goldfish_battery_state),
        VMSTATE_END_OF_LIST()
    }
};

void goldfish_battery_display_cb(void* opaque, BatteryLineCallback callback)
{
    DeviceState *dev = qdev_find_recursive(sysbus_get_default(),
                                           TYPE_GOLDFISH_BATTERY);
    struct goldfish_battery_state *s = GOLDFISH_BATTERY(dev);
    const char *value;
    int size;
    char buf[128] = {0};

    size = snprintf(buf, sizeof(buf) - 1,
                    "AC: %s\n", (s->ac_online) ? "online" : "offline");
    assert(size > 0);
    callback(opaque, buf, size);

    switch (s->status) {
    case POWER_SUPPLY_STATUS_CHARGING:
        value = "Charging";
        break;
    case POWER_SUPPLY_STATUS_DISCHARGING:
        value = "Discharging";
        break;
    case POWER_SUPPLY_STATUS_NOT_CHARGING:
        value = "Not charging";
        break;
    case POWER_SUPPLY_STATUS_FULL:
        value = "Full";
        break;
    default:
        value = "Unknown";
    }

    size = snprintf(buf, sizeof(buf) - 1, "status: %s\n", value);
    assert(size > 0);
    callback(opaque, buf, size);

    switch (s->health) {
    case POWER_SUPPLY_HEALTH_GOOD:
        value = "Good";
        break;
    case POWER_SUPPLY_HEALTH_OVERHEAT:
        value = "Overheat";
        break;
    case POWER_SUPPLY_HEALTH_DEAD:
        value = "Dead";
        break;
    case POWER_SUPPLY_HEALTH_OVERVOLTAGE:
        value = "Overvoltage";
        break;
    case POWER_SUPPLY_HEALTH_UNSPEC_FAILURE:
        value = "Unspecified failure";
        break;
    default:
        value = "Unknown";
    }
    size = snprintf(buf, sizeof(buf) - 1, "health: %s\n", value);
    assert(size > 0);
    callback(opaque, buf, size);

    size = snprintf(buf, sizeof(buf) - 1,
                    "present: %s\n", (s->present) ? "true" : "false");
    assert(size > 0);
    callback(opaque, buf, size);

    size = snprintf(buf, sizeof(buf) - 1, "capacity: %d\n", s->capacity);
    assert(size > 0);
    callback(opaque, buf, size);
}

static void monitor_print_callback(void* opaque, const char* buf, int size)
{
    Monitor* mon = (Monitor*)opaque;
    monitor_printf(mon, "%.*s", size, buf);
}

void goldfish_battery_display(Monitor *mon)
{
    goldfish_battery_display_cb(mon, &monitor_print_callback);
}

int goldfish_battery_read_prop(int property)
{
    int retVal = 0;

    DeviceState *dev = qdev_find_recursive(sysbus_get_default(),
                                           TYPE_GOLDFISH_BATTERY);
    struct goldfish_battery_state *battery_state = GOLDFISH_BATTERY(dev);

    if (!battery_state) {
        return 0;
    }

    switch (property) {
        case POWER_SUPPLY_PROP_ONLINE:
            retVal = battery_state->ac_online;
            break;
        case POWER_SUPPLY_PROP_STATUS:
            retVal = battery_state->status;
            break;
        case POWER_SUPPLY_PROP_HEALTH:
            retVal = battery_state->health;
            break;
        case POWER_SUPPLY_PROP_HAS_BATTERY:
            retVal =  battery_state->hw_has_battery;
            break;
        case POWER_SUPPLY_PROP_PRESENT:
            retVal = battery_state->present;
            break;
        case POWER_SUPPLY_PROP_CAPACITY:
            retVal = battery_state->capacity;
            break;
        case POWER_SUPPLY_PROP_VOLTAGE_NOW:
            retVal = battery_state->voltage;
            break;
        case POWER_SUPPLY_PROP_TEMP:
            retVal = battery_state->temp;
            break;
        case POWER_SUPPLY_PROP_CHARGE_COUNTER:
            retVal = battery_state->charge_counter;
            break;
        case POWER_SUPPLY_PROP_VOLTAGE_MAX:
            retVal = battery_state->voltage_max;
            break;
        case POWER_SUPPLY_PROP_CURRENT_MAX:
            retVal = battery_state->current_max;
            break;
        case POWER_SUPPLY_PROP_CURRENT_NOW:
            retVal = battery_state->current_now;
            break;
        case POWER_SUPPLY_PROP_CURRENT_AVG:
            retVal = battery_state->current_avg;
            break;
        case POWER_SUPPLY_PROP_CHARGE_FULL:
            retVal = battery_state->charge_full_uah;
            break;
        case POWER_SUPPLY_PROP_CYCLE_COUNT:
            retVal = battery_state->cycle_count;
            break;
        default:
            retVal = 0;
            break;
    }
    return retVal;
}

void goldfish_battery_set_prop(int ac, int property, int value)
{
    if (!sBatteryIsRealized) {
        // The only thing we can set before the battery has been
        // 'realized' is whether the device uses a battery at all.
        if (property == POWER_SUPPLY_PROP_HAS_BATTERY) {
            sDeviceHasBattery = value;
        }
        return;
    }

    DeviceState *dev = qdev_find_recursive(sysbus_get_default(),
                                           TYPE_GOLDFISH_BATTERY);
    struct goldfish_battery_state *battery_state = GOLDFISH_BATTERY(dev);
    int new_status = (ac ? AC_STATUS_CHANGED : BATTERY_STATUS_CHANGED);

    if (!battery_state || !battery_state->hw_has_battery) {
        return;
    }

    if (ac) {
        switch (property) {
        case POWER_SUPPLY_PROP_ONLINE:
            battery_state->ac_online = value;
            break;
        }
    } else {
        switch (property) {
        case POWER_SUPPLY_PROP_STATUS:
            battery_state->status = value;
            break;
        case POWER_SUPPLY_PROP_HEALTH:
            battery_state->health = value;
            break;
        case POWER_SUPPLY_PROP_PRESENT:
            battery_state->present = value;
            break;
        case POWER_SUPPLY_PROP_CAPACITY:
            battery_state->capacity = value;
            break;
        case POWER_SUPPLY_PROP_CURRENT_NOW:
            battery_state->current_now = value;
            break;
        case POWER_SUPPLY_PROP_CURRENT_AVG:
            battery_state->current_avg = value;
            break;
        case POWER_SUPPLY_PROP_CHARGE_FULL:
            battery_state->charge_full_uah = value;
            break;
        case POWER_SUPPLY_PROP_CYCLE_COUNT:
            battery_state->cycle_count = value;
            break;
        }
    }

    if (new_status != battery_state->int_status) {
        battery_state->int_status |= new_status;
        qemu_set_irq(battery_state->irq,
                     (battery_state->int_status &
                     battery_state->int_enable));
    }
}

static uint64_t goldfish_battery_read(void *opaque, hwaddr offset, unsigned size)
{
    uint64_t ret;
    struct goldfish_battery_state *s = opaque;

    switch(offset) {
        case BATTERY_INT_STATUS:
            // return current buffer status flags
            ret = s->int_status & s->int_enable;
            if (ret) {
                qemu_irq_lower(s->irq);
                s->int_status = 0;
            }
            return ret;

        case BATTERY_INT_ENABLE:
            return s->int_enable;
        case BATTERY_AC_ONLINE:
            return s->ac_online;
        case BATTERY_STATUS:
            return s->status;
        case BATTERY_HEALTH:
            return s->health;
        case BATTERY_PRESENT:
            return s->present;
        case BATTERY_CAPACITY:
            return s->capacity;
        case BATTERY_VOLTAGE:
            return s->voltage;
        case BATTERY_TEMP:
            return s->temp;
        case BATTERY_CHARGE_COUNTER:
            return s->charge_counter;
        case BATTERY_VOLTAGE_MAX:
            return s->voltage_max;
        case BATTERY_CURRENT_MAX:
            return s->current_max;
        case BATTERY_CURRENT_NOW:
            return s->current_now;
        case BATTERY_CURRENT_AVG:
            return s->current_avg;
        case BATTERY_CHARGE_FULL_UAH:
            return s->charge_full_uah;
        case BATTERY_CYCLE_COUNT:
            return s->cycle_count;
        default:
            return 0;
    }
}

static void goldfish_battery_write(void *opaque, hwaddr offset, uint64_t val,
        unsigned size)
{
    struct goldfish_battery_state *s = opaque;

    switch(offset) {
        case BATTERY_INT_ENABLE:
            /* enable interrupts */
            s->int_enable = val;

            uint32_t now_active = (s->int_enable & s->int_status);
            if (now_active != 0) {
                // Some interrupt is now unmasked, signal IRQ
                qemu_set_irq(s->irq, now_active);
            }
            break;
    }
}

static const MemoryRegionOps goldfish_battery_iomem_ops = {
    .read = goldfish_battery_read,
    .write = goldfish_battery_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .impl.min_access_size = 4,
    .impl.max_access_size = 4,
};

static void goldfish_battery_realize(DeviceState *dev, Error **errp)
{
    SysBusDevice *sbdev = SYS_BUS_DEVICE(dev);
    struct goldfish_battery_state *s = GOLDFISH_BATTERY(dev);

    /* Initialize the device ID so the battery can be looked up during monitor
     * commands.
     */
    dev->id = g_strdup("goldfish_battery");

    memory_region_init_io(&s->iomem, OBJECT(s), &goldfish_battery_iomem_ops, s,
            "goldfish_battery", 0x1000);
    sysbus_init_mmio(sbdev, &s->iomem);
    sysbus_init_irq(sbdev, &s->irq);

    // default values for the battery
    s->ac_online = 1;
    s->voltage = 5000000;  // 5 volt
    s->temp = 250;         // 25 celsuis
    s->voltage_max = 5000000;  // 5 volt
    s->current_max = 5000000;  // 5 amp
    s->current_now = 900000;   // 0.9A
    s->current_avg = 900000;   // 0.9A
    s->charge_full_uah = 3000000;  // 3AH
    s->cycle_count = 10;

    if (sDeviceHasBattery) {
        s->hw_has_battery = 1;
        s->status = POWER_SUPPLY_STATUS_CHARGING;
        s->health = POWER_SUPPLY_HEALTH_GOOD;
        s->present = 1;
        s->capacity = 100;   // 100% charged
        s->charge_counter = 10000;
    } else {
        s->hw_has_battery = 0;
        s->status = POWER_SUPPLY_STATUS_UNKNOWN;
        s->health = POWER_SUPPLY_HEALTH_UNKNOWN;
        s->present = 0;
        s->capacity = 0;
        s->charge_counter = 0;
    }

    sBatteryIsRealized = 1;
}

static void goldfish_battery_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = goldfish_battery_realize;
    dc->vmsd = &goldfish_battery_vmsd;
    dc->desc = "goldfish battery";
}

static const TypeInfo goldfish_battery_info = {
    .name          = TYPE_GOLDFISH_BATTERY,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(struct goldfish_battery_state),
    .class_init    = goldfish_battery_class_init,
};

static void goldfish_battery_register(void)
{
    type_register_static(&goldfish_battery_info);
}

type_init(goldfish_battery_register);

/*
 * Delta U50SU8P167 proportional Power-Blade module
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/i2c/pmbus_device.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qemu/log.h"

#define TYPE_U50SU8P167 "u50su8p167"
OBJECT_DECLARE_SIMPLE_TYPE(U50SU8P167State, U50SU8P167)

#define U50SU8P167_NUM_PAGES       1
#define DEFAULT_OPERATION       0x80
#define DEFAULT_CAPABILITY      0x20

typedef struct U50SU8P167State {
    PMBusDevice parent;
} U50SU8P167State;

static uint8_t u50su8p167_read_byte(PMBusDevice *pmdev)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s: reading from unimplemented register: 0x%02x\n",
                  __func__, pmdev->code);
    return 0xFF;
}

static int u50su8p167_write_data(PMBusDevice *pmdev, const uint8_t *buf,
                                 uint8_t len)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s: writing to unimplemented register: 0x%02x\n",
                  __func__, pmdev->code);
    return 0;
}

static void u50su8p167_exit_reset(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);

    pmdev->capability = DEFAULT_CAPABILITY;
    pmdev->pages[0].operation = DEFAULT_OPERATION;
}

static const VMStateDescription vmstate_u50su8p167 = {
    .name = TYPE_U50SU8P167,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]){
        VMSTATE_PMBUS_DEVICE(parent, U50SU8P167State),
        VMSTATE_END_OF_LIST()
    }
};

static void u50su8p167_init(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint64_t psu_flags = PB_HAS_VIN | PB_HAS_VOUT | PB_HAS_VOUT_MARGIN |
                         PB_HAS_IOUT | PB_HAS_TEMPERATURE | PB_HAS_MFR_INFO;

    pmbus_page_config(pmdev, 0, psu_flags);
}

static void u50su8p167_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    PMBusDeviceClass *k = PMBUS_DEVICE_CLASS(klass);

    dc->desc = "Delta U50SU8P167 Power-Blade module";
    dc->vmsd = &vmstate_u50su8p167;
    k->write_data = u50su8p167_write_data;
    k->receive_byte = u50su8p167_read_byte;
    k->device_num_pages = U50SU8P167_NUM_PAGES;
    rc->phases.exit = u50su8p167_exit_reset;
}

static const TypeInfo u50su8p167_info = {
    .name = TYPE_U50SU8P167,
    .parent = TYPE_PMBUS_DEVICE,
    .instance_size = sizeof(U50SU8P167State),
    .instance_init = u50su8p167_init,
    .class_init = u50su8p167_class_init,
};

static void u50su8p167_register_types(void)
{
    type_register_static(&u50su8p167_info);
}

type_init(u50su8p167_register_types)

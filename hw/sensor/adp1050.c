/*
 * Analog Devices ADP1050 PMBus Digital Conroller for Power Supply
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/i2c/pmbus_device.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qemu/log.h"

#define TYPE_ADP1050 "adp1050"
OBJECT_DECLARE_SIMPLE_TYPE(ADP1050State, ADP1050)

#define ADP1050_NUM_PAGES       1
#define DEFAULT_OPERATION       0x80
#define DEFAULT_CAPABILITY      0x20

typedef struct ADP1050State {
    PMBusDevice parent;
} ADP1050State;

static uint8_t adp1050_read_byte(PMBusDevice *pmdev)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s: reading from unimplemented register: 0x%02x\n",
                  __func__, pmdev->code);
    return 0xFF;
}

static int adp1050_write_data(PMBusDevice *pmdev, const uint8_t *buf,
                              uint8_t len)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s: writing to unimplemented register: 0x%02x\n",
                  __func__, pmdev->code);
    return 0;
}

static void adp1050_exit_reset(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);

    pmdev->capability = DEFAULT_CAPABILITY;
    pmdev->pages[0].operation = DEFAULT_OPERATION;
}

static const VMStateDescription vmstate_adp1050 = {
    .name = TYPE_ADP1050,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]){
        VMSTATE_PMBUS_DEVICE(parent, ADP1050State),
        VMSTATE_END_OF_LIST()
    }
};

static void adp1050_init(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint64_t psu_flags = PB_HAS_VIN | PB_HAS_VOUT | PB_HAS_VOUT_MARGIN |
                         PB_HAS_IIN | PB_HAS_TEMPERATURE | PB_HAS_MFR_INFO;

    pmbus_page_config(pmdev, 0, psu_flags);
}

static void adp1050_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    PMBusDeviceClass *k = PMBUS_DEVICE_CLASS(klass);

    dc->desc = "Analog Devices ADP1050 Power Supply Contoller";
    dc->vmsd = &vmstate_adp1050;
    k->write_data = adp1050_write_data;
    k->receive_byte = adp1050_read_byte;
    k->device_num_pages = ADP1050_NUM_PAGES;
    rc->phases.exit = adp1050_exit_reset;
}

static const TypeInfo adp1050_info = {
    .name = TYPE_ADP1050,
    .parent = TYPE_PMBUS_DEVICE,
    .instance_size = sizeof(ADP1050State),
    .instance_init = adp1050_init,
    .class_init = adp1050_class_init,
};

static void adp1050_register_types(void)
{
    type_register_static(&adp1050_info);
}

type_init(adp1050_register_types)

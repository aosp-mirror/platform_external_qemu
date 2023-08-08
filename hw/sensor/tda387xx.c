/*
 * Infineon TDA38740/25 PMBus Digital Conroller for Power Supply
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/i2c/pmbus_device.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/log.h"

#define TYPE_TDA387XX "tda387xx"
OBJECT_DECLARE_SIMPLE_TYPE(TDA387xxState, TDA387XX)


#define TDA387XX_VENDOR_INFO                0xC1

#define TDA387XX_NUM_PAGES                  2
#define TDA387XX_DEFAULT_CAPABILITY         0x20
#define TDA387XX_DEFAULT_OPERATION          0x80
#define TDA387XX_DEFAULT_VOUT_MODE          0x16 /* ulinear16 vout exponent */
#define TDA387XX_DEFAULT_DEVICE_ID          0x84
#define TDA387XX_DEFAULT_DEVICE_REV         0x1
#define TDA387XX_PB_REVISION                0x22

/* Random defaults */
#define TDA387XX_DEFAULT_ON_OFF_CONFIG      0x1a
#define TDA387XX_DEFAULT_VOUT_COMMAND       0x400
#define TDA387XX_DEFAULT_VOUT_MAX           0x800
#define TDA387XX_DEFAULT_VOUT_MARGIN_HIGH   0x600
#define TDA387XX_DEFAULT_VOUT_MARGIN_LOW    0x200
#define TDA387XX_DEFAULT_VOUT_MIN           0x100
#define TDA387XX_DEFAULT_VIN                0x10
#define TDA387XX_DEFAULT_VOUT               0x400
#define TDA387XX_DEFAULT_IIN                0x8
#define TDA387XX_DEFAULT_IOUT               0x8
#define TDA387XX_DEFAULT_PIN                0x10
#define TDA387XX_DEFAULT_POUT               0x10
#define TDA387XX_DEFAULT_TEMPERATURE_1      55

static const uint8_t tda38740_ic_device_id[] = {0x1, 0x84};
static const uint8_t tda38740_ic_device_rev[] = {0x1, 0x1};

typedef struct TDA387xxState {
    PMBusDevice parent;
    uint16_t vendor;
} TDA387xxState;

static uint8_t tda387xx_read_byte(PMBusDevice *pmdev)
{
    TDA387xxState *s = TDA387XX(pmdev);

    switch (pmdev->code) {
    case PMBUS_IC_DEVICE_ID:
        pmbus_send(pmdev, tda38740_ic_device_id, sizeof(tda38740_ic_device_id));
        break;

    case PMBUS_IC_DEVICE_REV:
        pmbus_send(pmdev,
                   tda38740_ic_device_rev,
                   sizeof(tda38740_ic_device_rev));
        break;

    case TDA387XX_VENDOR_INFO:
        pmbus_send16(pmdev, s->vendor);
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: %s: reading from unimplemented register: 0x%02x\n",
                      DEVICE(s)->canonical_path, __func__, pmdev->code);
        return 0xFF;
    }
    return 0;
}

static int tda387xx_write_data(PMBusDevice *pmdev, const uint8_t *buf,
                              uint8_t len)
{
    switch (pmdev->code) {
    case PMBUS_IC_DEVICE_ID:
    case PMBUS_IC_DEVICE_REV:
    case TDA387XX_VENDOR_INFO:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: %s: writing to read-only register: 0x%02x\n",
                      DEVICE(pmdev)->canonical_path, __func__, pmdev->code);
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: %s: writing to unimplemented register: 0x%02x\n",
                      DEVICE(pmdev)->canonical_path, __func__, pmdev->code);
        break;
    }
    return 0;
}

static void tda387xx_exit_reset(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);

    pmdev->capability = TDA387XX_DEFAULT_CAPABILITY;
    for (int i = 0; i < pmdev->num_pages; i++) {
        pmdev->pages[i].operation = TDA387XX_DEFAULT_OPERATION;
        pmdev->pages[i].revision = TDA387XX_PB_REVISION;
        pmdev->pages[i].vout_mode = TDA387XX_DEFAULT_VOUT_MODE;
        pmdev->pages[i].on_off_config = TDA387XX_DEFAULT_ON_OFF_CONFIG;
        pmdev->pages[i].vout_command = TDA387XX_DEFAULT_VOUT_COMMAND;
        pmdev->pages[i].vout_max = TDA387XX_DEFAULT_VOUT_MAX;
        pmdev->pages[i].vout_margin_high = TDA387XX_DEFAULT_VOUT_MARGIN_HIGH;
        pmdev->pages[i].vout_margin_low = TDA387XX_DEFAULT_VOUT_MARGIN_LOW;
        pmdev->pages[i].vout_min = TDA387XX_DEFAULT_VOUT_MIN;
        pmdev->pages[i].read_vin = TDA387XX_DEFAULT_VIN;
        pmdev->pages[i].read_vout = TDA387XX_DEFAULT_VOUT;
        pmdev->pages[i].read_iin = TDA387XX_DEFAULT_IIN;
        pmdev->pages[i].read_iout = TDA387XX_DEFAULT_IOUT;
        pmdev->pages[i].read_pin = TDA387XX_DEFAULT_PIN;
        pmdev->pages[i].read_pout = TDA387XX_DEFAULT_POUT;
        pmdev->pages[i].read_temperature_1 = TDA387XX_DEFAULT_TEMPERATURE_1;
    }
}

static const VMStateDescription vmstate_tda387xx = {
    .name = TYPE_TDA387XX,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]){
        VMSTATE_PMBUS_DEVICE(parent, TDA387xxState),
        VMSTATE_END_OF_LIST()
    }
};

static void tda387xx_get(Object *obj, Visitor *v, const char *name,
                         void *opaque, Error **errp)
{
    uint32_t value;

    value = *(uint16_t *)opaque;

    /* scale to milli-units */
    if (strcmp(name, "pout") != 0 && strcmp(name, "pin") != 0) {
        value *= 1000;
    }

    visit_type_uint32(v, name, &value, errp);
}

static void tda387xx_set(Object *obj, Visitor *v, const char *name,
                         void *opaque, Error **errp)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint16_t *internal = opaque;
    uint32_t value;
    if (!visit_type_uint32(v, name, &value, errp)) {
        return;
    }

    /* inputs match kernel driver which scales to milliunits except power */
    if (strcmp(name, "pout") != 0 && strcmp(name, "pin") != 0) {
        value /= 1000;
    }

    *internal = (uint16_t)value;

    pmbus_check_limits(pmdev);
}

static void tda387xx_init(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint64_t flags = PB_HAS_VIN | PB_HAS_VOUT | PB_HAS_VOUT_MARGIN
                     | PB_HAS_VOUT_MODE | PB_HAS_VOUT_RATING | PB_HAS_IIN
                     | PB_HAS_IOUT | PB_HAS_PIN | PB_HAS_POUT
                     | PB_HAS_TEMPERATURE | PB_HAS_MFR_INFO;

    pmbus_page_config(pmdev, 0, flags);
    pmbus_page_config(pmdev, 1, flags);

    object_property_add(obj, "vin", "uint32", tda387xx_get, tda387xx_set, NULL,
                        &pmdev->pages[0].read_vin);

    object_property_add(obj, "vout", "uint32", tda387xx_get, tda387xx_set,
                        NULL, &pmdev->pages[0].read_vout);

    object_property_add(obj, "iin", "uint32", tda387xx_get, tda387xx_set, NULL,
                        &pmdev->pages[0].read_iin);

    object_property_add(obj, "iout", "uint32", tda387xx_get, tda387xx_set,
                        NULL, &pmdev->pages[0].read_iout);

    object_property_add(obj, "pin", "uint32", tda387xx_get, tda387xx_set, NULL,
                        &pmdev->pages[0].read_pin);

    object_property_add(obj, "pout", "uint32", tda387xx_get, tda387xx_set,
                        NULL, &pmdev->pages[0].read_pout);

    object_property_add(obj, "temperature", "uint32",
                        tda387xx_get, tda387xx_set,
                        NULL, &pmdev->pages[0].read_temperature_1);
}

static void tda387xx_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    PMBusDeviceClass *k = PMBUS_DEVICE_CLASS(klass);

    dc->desc = "Infineon TDA38740/25 Buck Regulator";
    dc->vmsd = &vmstate_tda387xx;
    k->write_data = tda387xx_write_data;
    k->receive_byte = tda387xx_read_byte;
    k->device_num_pages = 2;
    rc->phases.exit = tda387xx_exit_reset;
}

static const TypeInfo tda387xx_types[] = {
    {
        .name = TYPE_TDA387XX,
        .parent = TYPE_PMBUS_DEVICE,
        .instance_size = sizeof(TDA387xxState),
        .instance_init = tda387xx_init,
        .class_init = tda387xx_class_init,
    }
};

DEFINE_TYPES(tda387xx_types)

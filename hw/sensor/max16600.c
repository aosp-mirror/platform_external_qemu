/*
 * MAX16600 VR13.HC Dual-Output Voltage Regulator Chipset
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "qemu/osdep.h"
#include "hw/i2c/pmbus_device.h"
#include "qapi/visitor.h"
#include "qemu/log.h"
#include "hw/sensor/max16600.h"

static uint8_t max16600_read_byte(PMBusDevice *pmdev)
{
    MAX16600State *s = MAX16600(pmdev);

    switch (pmdev->code) {
    case PMBUS_IC_DEVICE_ID:
        pmbus_send_string(pmdev, s->ic_device_id);
        break;

    case MAX16600_PHASE_ID:
        pmbus_send8(pmdev, s->phase_id);
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: reading from unsupported register: 0x%02x\n",
                      __func__, pmdev->code);
        break;
    }
    return 0xFF;
}

static int max16600_write_data(PMBusDevice *pmdev, const uint8_t *buf,
                               uint8_t len)
{
    qemu_log_mask(LOG_GUEST_ERROR,
                  "%s: write to unsupported register: 0x%02x\n", __func__,
                  pmdev->code);
    return 0xFF;
}

static void max16600_exit_reset(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    MAX16600State *s = MAX16600(obj);

    pmdev->capability = MAX16600_CAPABILITY_DEFAULT;
    pmdev->page = 0;

    pmdev->pages[0].operation = MAX16600_OPERATION_DEFAULT;
    pmdev->pages[0].on_off_config = MAX16600_ON_OFF_CONFIG_DEFAULT;
    pmdev->pages[0].vout_mode = MAX16600_VOUT_MODE_DEFAULT;

    pmdev->pages[0].read_vin =
        pmbus_data2linear_mode(MAX16600_READ_VIN_DEFAULT, max16600_exp.vin);
    pmdev->pages[0].read_iin =
        pmbus_data2linear_mode(MAX16600_READ_IIN_DEFAULT, max16600_exp.iin);
    pmdev->pages[0].read_pin =
        pmbus_data2linear_mode(MAX16600_READ_PIN_DEFAULT, max16600_exp.pin);
    pmdev->pages[0].read_vout = MAX16600_READ_VOUT_DEFAULT;
    pmdev->pages[0].read_iout =
        pmbus_data2linear_mode(MAX16600_READ_IOUT_DEFAULT, max16600_exp.iout);
    pmdev->pages[0].read_pout =
        pmbus_data2linear_mode(MAX16600_READ_PIN_DEFAULT, max16600_exp.pout);
    pmdev->pages[0].read_temperature_1 =
        pmbus_data2linear_mode(MAX16600_READ_TEMP_DEFAULT, max16600_exp.temp);

    s->ic_device_id = "MAX16601";
    s->phase_id = MAX16600_PHASE_ID_DEFAULT;
}

static void max16600_get(Object *obj, Visitor *v, const char *name,
                         void *opaque, Error **errp)
{
    uint16_t value;

    if (strcmp(name, "vin") == 0) {
        value = pmbus_linear_mode2data(*(uint16_t *)opaque, max16600_exp.vin);
    } else if (strcmp(name, "iin") == 0) {
        value = pmbus_linear_mode2data(*(uint16_t *)opaque, max16600_exp.iin);
    } else if (strcmp(name, "pin") == 0) {
        value = pmbus_linear_mode2data(*(uint16_t *)opaque, max16600_exp.pin);
    } else if (strcmp(name, "iout") == 0) {
        value = pmbus_linear_mode2data(*(uint16_t *)opaque, max16600_exp.iout);
    } else if (strcmp(name, "pout") == 0) {
        value = pmbus_linear_mode2data(*(uint16_t *)opaque, max16600_exp.pout);
    } else if (strcmp(name, "temperature") == 0) {
        value = pmbus_linear_mode2data(*(uint16_t *)opaque, max16600_exp.temp);
    } else {
        value = *(uint16_t *)opaque;
    }

    /* scale to milli-units */
    if (strcmp(name, "pout") != 0 && strcmp(name, "pin") != 0) {
        value *= 1000;
    }

    visit_type_uint16(v, name, &value, errp);
}

static void max16600_set(Object *obj, Visitor *v, const char *name,
                         void *opaque, Error **errp)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint16_t *internal = opaque;
    uint16_t value;
    if (!visit_type_uint16(v, name, &value, errp)) {
        return;
    }

    /* inputs match kernel driver which scales to milliunits except power */
    if (strcmp(name, "pout") != 0 && strcmp(name, "pin") != 0) {
        value /= 1000;
    }

    if (strcmp(name, "vin") == 0) {
        *internal = pmbus_data2linear_mode(value, max16600_exp.vin);
    } else if (strcmp(name, "iin") == 0) {
        *internal = pmbus_data2linear_mode(value, max16600_exp.iin);
    } else if (strcmp(name, "pin") == 0) {
        *internal = pmbus_data2linear_mode(value, max16600_exp.pin);
    } else if (strcmp(name, "iout") == 0) {
        *internal = pmbus_data2linear_mode(value, max16600_exp.iout);
    } else if (strcmp(name, "pout") == 0) {
        *internal = pmbus_data2linear_mode(value, max16600_exp.pout);
    } else if (strcmp(name, "temperature") == 0) {
        *internal = pmbus_data2linear_mode(value, max16600_exp.temp);
    } else {
        *internal = value;
    }

    pmbus_check_limits(pmdev);
}

static void max16600_init(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint64_t flags = PB_HAS_VOUT_MODE | PB_HAS_VIN | PB_HAS_IIN | PB_HAS_PIN |
                     PB_HAS_IOUT | PB_HAS_POUT | PB_HAS_VOUT |
                     PB_HAS_TEMPERATURE | PB_HAS_MFR_INFO;
    pmbus_page_config(pmdev, 0, flags);

    object_property_add(obj, "vin", "uint16", max16600_get, max16600_set, NULL,
                        &pmdev->pages[0].read_vin);

    object_property_add(obj, "iin", "uint16", max16600_get, max16600_set, NULL,
                        &pmdev->pages[0].read_iin);

    object_property_add(obj, "pin", "uint16", max16600_get, max16600_set, NULL,
                        &pmdev->pages[0].read_pin);

    object_property_add(obj, "vout", "uint16", max16600_get, max16600_set,
                        NULL, &pmdev->pages[0].read_vout);

    object_property_add(obj, "iout", "uint16", max16600_get, max16600_set,
                        NULL, &pmdev->pages[0].read_iout);

    object_property_add(obj, "pout", "uint16", max16600_get, max16600_set,
                        NULL, &pmdev->pages[0].read_pout);

    object_property_add(obj, "temperature", "uint16",
                        max16600_get, max16600_set,
                        NULL, &pmdev->pages[0].read_temperature_1);
}

static void max16600_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    PMBusDeviceClass *k = PMBUS_DEVICE_CLASS(klass);

    dc->desc = "MAX16600 Dual-Output Voltage Regulator";
    k->write_data = max16600_write_data;
    k->receive_byte = max16600_read_byte;
    k->device_num_pages = 1;

    rc->phases.exit = max16600_exit_reset;
}

static const TypeInfo max16600_info = {
    .name = TYPE_MAX16600,
    .parent = TYPE_PMBUS_DEVICE,
    .instance_size = sizeof(MAX16600State),
    .instance_init = max16600_init,
    .class_init = max16600_class_init,
};

static void max16600_register_types(void)
{
    type_register_static(&max16600_info);
}

type_init(max16600_register_types)

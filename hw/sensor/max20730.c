/*
 * MAX20730 Integrated, Step-Down Switching Regulator
 *
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/i2c/pmbus_device.h"
#include "migration/vmstate.h"
#include "qapi/visitor.h"
#include "qemu/log.h"

#define TYPE_MAX20730 "max20730"

#define MAX20730_MFR_VOUT_MIN 0xD1
#define MAX20730_MFR_DEVSET1  0xD2
#define MAX20730_MFR_DEVSET2  0xD3

#define MAX20730_CAPABILITY_DEFAULT         0x20
#define MAX20730_OPERATION_DEFAULT          0x80
#define MAX20730_ON_OFF_CONFIG_DEFAULT      0x1F
#define MAX20730_VOUT_MODE_DEFAULT          0x17
#define MAX20730_READ_VIN_DEFAULT           1
#define MAX20730_READ_VOUT_DEFAULT          1
#define MAX20730_READ_IOUT_DEFAULT          25
#define MAX20730_READ_TEMP_DEFAULT          30
#define MAX20730_VOUT_MAX_DEFAULT           0x280
#define MAX20730_MFR_VOUT_MIN_DEFAULT       0x133
#define MAX20730_MFR_DEVSET1_DEFAULT        0x2061
#define MAX20730_MFR_DEVSET2_DEFAULT        0x3A6

struct MAX20730State {
    PMBusDevice parent;

    uint16_t mfr_vout_min;
    uint16_t mfr_devset1;
    uint16_t mfr_devset2;

    uint16_t r_fb1;
    uint16_t r_fb2;
};

OBJECT_DECLARE_SIMPLE_TYPE(MAX20730State, MAX20730)

static const struct {
    PMBusCoefficients vin;
    PMBusCoefficients iout;
    PMBusCoefficients temperature;
    int vout_exp;
} max20730_c = {
    {3609, 0, -2},
    {153, 4976, -1},
    {21, 5887, -1},
    3
};

static uint8_t max20730_read_byte(PMBusDevice *pmdev)
{
    MAX20730State *s = MAX20730(pmdev);

    switch (pmdev->code) {
    case MAX20730_MFR_VOUT_MIN:
        pmbus_send16(pmdev, s->mfr_vout_min);
        break;

    case MAX20730_MFR_DEVSET1:
        pmbus_send16(pmdev, s->mfr_devset1);
        break;

    case MAX20730_MFR_DEVSET2:
        pmbus_send16(pmdev, s->mfr_devset2);
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: reading from unimplemented register: 0x%02x\n",
                      __func__, pmdev->code);
    }
    return 0xFF;
}

static int max20730_write_data(PMBusDevice *pmdev, const uint8_t *buf,
                               uint8_t len)
{
    MAX20730State *s = MAX20730(pmdev);

    if (len == 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: writing empty data\n", __func__);
        return -1;
    }

    pmdev->code = buf[0]; /* PMBus command code */

    if (len == 1) {
        return 0;
    }

    /* Exclude command code from buffer */
    buf++;
    len--;

    switch (pmdev->code) {
    case MAX20730_MFR_VOUT_MIN:
        s->mfr_vout_min = pmbus_receive16(pmdev);
        break;

    case MAX20730_MFR_DEVSET1:
        s->mfr_devset1 = pmbus_receive16(pmdev);
        break;

    case MAX20730_MFR_DEVSET2:
        s->mfr_devset2 = pmbus_receive16(pmdev);
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: write to unimplemented register: 0x%02x\n", __func__,
                      pmdev->code);
        break;
    }
    return 0xFF;
}

static void max20730_exit_reset(Object *obj)
{
    MAX20730State *s = MAX20730(obj);
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    const char *mfr_model = "M20743";
    const char *mfr_rev = "F";
    const char *mfr_id = "MAXIM";

    s->mfr_vout_min = MAX20730_MFR_VOUT_MIN_DEFAULT;
    s->mfr_devset1 = MAX20730_MFR_DEVSET1_DEFAULT;
    s->mfr_devset2 = MAX20730_MFR_DEVSET2_DEFAULT;

    pmdev->capability = MAX20730_CAPABILITY_DEFAULT;
    pmdev->page = 0;

    pmdev->pages[0].mfr_id = mfr_id;
    pmdev->pages[0].mfr_model = mfr_model;
    pmdev->pages[0].mfr_revision = mfr_rev;

    pmdev->pages[0].operation = MAX20730_OPERATION_DEFAULT;
    pmdev->pages[0].on_off_config = MAX20730_ON_OFF_CONFIG_DEFAULT;
    pmdev->pages[0].vout_mode = MAX20730_VOUT_MODE_DEFAULT;
    pmdev->pages[0].vout_max = MAX20730_VOUT_MAX_DEFAULT;

    pmdev->pages[0].read_vin =
        pmbus_data2direct_mode(max20730_c.vin, MAX20730_READ_VIN_DEFAULT);
    pmdev->pages[0].read_vout =
        pmbus_data2linear_mode(MAX20730_READ_VOUT_DEFAULT, max20730_c.vout_exp);
    pmdev->pages[0].read_iout =
        pmbus_data2direct_mode(max20730_c.iout, MAX20730_READ_IOUT_DEFAULT);
    pmdev->pages[0].read_temperature_1 =
        pmbus_data2direct_mode(
            max20730_c.temperature, MAX20730_READ_TEMP_DEFAULT);
}

static void max20730_get(Object *obj, Visitor *v, const char *name,
                         void *opaque, Error **errp)
{
    uint16_t value;

    if (strcmp(name, "vin") == 0) {
        value = pmbus_direct_mode2data(max20730_c.vin, *(uint16_t *)opaque);
    } else if (strcmp(name, "vout") == 0) {
        value =
            pmbus_linear_mode2data(*(uint16_t *)opaque, max20730_c.vout_exp);
    } else if (strcmp(name, "iout") == 0) {
        value = pmbus_direct_mode2data(max20730_c.iout, *(uint16_t *)opaque);
    } else if (strcmp(name, "temperature") == 0) {
        value =
            pmbus_direct_mode2data(max20730_c.temperature, *(uint16_t *)opaque);
    } else {
        value = *(uint16_t *)opaque;
    }

    visit_type_uint16(v, name, &value, errp);
}

static void max20730_set(Object *obj, Visitor *v, const char *name,
                         void *opaque, Error **errp)
{
    MAX20730State *s = MAX20730(obj);
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint16_t *internal = opaque;
    uint16_t value;
    if (!visit_type_uint16(v, name, &value, errp)) {
        return;
    }

    if (strcmp(name, "vin") == 0) {
        *internal = pmbus_data2direct_mode(max20730_c.vin, value);
    } else if (strcmp(name, "vout") == 0) { /* millivolts */
        if (s->r_fb1 + s->r_fb2 != 0) {
            value = (value * s->r_fb2) / (s->r_fb1 + s->r_fb2);
            *internal = pmbus_data2linear_mode(value, max20730_c.vout_exp);
        }
    } else if (strcmp(name, "iout") == 0) {
        *internal = pmbus_data2direct_mode(max20730_c.iout, value);
    } else if (strcmp(name, "temperature") == 0) {
        *internal = pmbus_data2direct_mode(max20730_c.temperature, value);
    } else {
        *internal = value;
    }

    pmbus_check_limits(pmdev);
}

static const VMStateDescription vmstate_max20730 = {
    .name = TYPE_MAX20730,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]){
        VMSTATE_PMBUS_DEVICE(parent, MAX20730State),
        VMSTATE_UINT16(mfr_vout_min, MAX20730State),
        VMSTATE_UINT16(mfr_devset1, MAX20730State),
        VMSTATE_UINT16(mfr_devset2, MAX20730State),
        VMSTATE_END_OF_LIST()
    }
};

static void max20730_init(Object *obj)
{
    MAX20730State *s = MAX20730(obj);
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    uint64_t flags = PB_HAS_VOUT_MODE | PB_HAS_VIN |
                     PB_HAS_IOUT | PB_HAS_VOUT |
                     PB_HAS_TEMPERATURE | PB_HAS_MFR_INFO;
    pmbus_page_config(pmdev, 0, flags);

    object_property_add(obj, "vin", "uint16", max20730_get, max20730_set, NULL,
                        &pmdev->pages[0].read_vin);

    object_property_add(obj, "vout", "uint16", max20730_get, max20730_set,
                        NULL, &pmdev->pages[0].read_vout);

    object_property_add(obj, "iout", "uint16", max20730_get, max20730_set,
                        NULL, &pmdev->pages[0].read_iout);

    object_property_add(obj, "temperature", "uint16",
                        max20730_get, max20730_set,
                        NULL, &pmdev->pages[0].read_temperature_1);

    /* Voltage divider R_FB1 Ohms */
    object_property_add(obj, "r_fb1", "uint16",
                        max20730_get, max20730_set,
                        NULL, &s->r_fb1);

    /* Voltage divider R_FB2 Ohms */
    object_property_add(obj, "r_fb2", "uint16",
                        max20730_get, max20730_set,
                        NULL, &s->r_fb2);
}

static void max20730_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    PMBusDeviceClass *k = PMBUS_DEVICE_CLASS(klass);

    dc->desc = "MAX20730 Dual-Output Voltage Regulator";
    dc->vmsd = &vmstate_max20730;
    k->write_data = max20730_write_data;
    k->receive_byte = max20730_read_byte;
    k->device_num_pages = 1;

    rc->phases.exit = max20730_exit_reset;
}

static const TypeInfo max20730_info = {
    .name = TYPE_MAX20730,
    .parent = TYPE_PMBUS_DEVICE,
    .instance_init = max20730_init,
    .instance_size = sizeof(MAX20730State),
    .class_init = max20730_class_init,
};

static void max20730_register_types(void)
{
    type_register_static(&max20730_info);
}

type_init(max20730_register_types)

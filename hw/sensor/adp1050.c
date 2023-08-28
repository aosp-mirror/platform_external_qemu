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
#include "qapi/visitor.h"
#include "qemu/log.h"

#define TYPE_ADP1050 "adp1050"
OBJECT_DECLARE_SIMPLE_TYPE(ADP1050State, ADP1050)

#define ADP1050_NUM_PAGES       1

/* MFR specific registers */
#define ADP1050_VIN_SCALE_MONITOR               0xD8
#define ADP1050_IIN_SCALE_MONITOR               0xD9

#define ADP1050_DEFAULT_OPERATION               0x80
#define ADP1050_DEFAULT_CAPABILITY              0x20
#define ADP1050_DEFAULT_VOUT_MODE               0x16
#define ADP1050_DEFAULT_VOUT_TRANSITION_RATE    0x7BFF
#define ADP1050_DEFAULT_VOUT_SCALE_LOOP         0x1
#define ADP1050_DEFAULT_VOUT_SCALE_MONITOR      0x1
#define ADP1050_DEFAULT_FREQUENCY_SWITCH        0x31
#define ADP1050_DEFAULT_TON_RISE                0xC00D
#define ADP1050_DEFAULT_REVISION                0x22
#define ADP1050_DEFAULT_IC_DEVICE_ID            0x4151
#define ADP1050_DEFAULT_IC_DEVICE_REV           0x20
#define ADP1050_DEFAULT_VIN_SCALE_MONITOR       0x1
#define ADP1050_DEFAULT_IIN_SCALE_MONITOR       0x1

typedef struct ADP1050State {
    PMBusDevice parent;
    uint8_t mfr_id;
    uint8_t mfr_model;
    uint8_t mfr_revision;
    uint16_t ic_device_id;
    uint8_t ic_device_rev;
    uint16_t vin_scale_monitor;
    uint16_t iin_scale_monitor;
} ADP1050State;

static uint8_t adp1050_read_byte(PMBusDevice *pmdev)
{
    ADP1050State *s = ADP1050(pmdev);

    switch (pmdev->page) {
    case PMBUS_MFR_ID:
        pmbus_send8(pmdev, s->mfr_id);
        break;

    case PMBUS_MFR_MODEL:
        pmbus_send8(pmdev, s->mfr_model);
        break;

    case PMBUS_MFR_REVISION:
        pmbus_send8(pmdev, s->mfr_revision);
        break;

    case PMBUS_IC_DEVICE_ID:
        pmbus_send16(pmdev, s->ic_device_id);
        break;

    case PMBUS_IC_DEVICE_REV:
        pmbus_send8(pmdev, s->ic_device_rev);
        break;

    case ADP1050_VIN_SCALE_MONITOR:
        pmbus_send16(pmdev, s->vin_scale_monitor);
        break;

    case ADP1050_IIN_SCALE_MONITOR:
        pmbus_send16(pmdev, s->iin_scale_monitor);
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: %s: reading from unimplemented register: 0x%02x\n",
                      DEVICE(s)->canonical_path, __func__, pmdev->code);
        return PMBUS_ERR_BYTE;
    }
    return 0;
}

static int adp1050_write_data(PMBusDevice *pmdev, const uint8_t *buf,
                              uint8_t len)
{
    ADP1050State *s = ADP1050(pmdev);

    switch (pmdev->page) {
    case PMBUS_MFR_ID:
        s->mfr_id = pmbus_receive8(pmdev);
        break;

    case PMBUS_MFR_MODEL:
        s->mfr_model = pmbus_receive8(pmdev);
        break;

    case PMBUS_MFR_REVISION:
        s->mfr_revision = pmbus_receive8(pmdev);
        break;

    case ADP1050_VIN_SCALE_MONITOR:
        s->vin_scale_monitor = pmbus_receive16(pmdev);
        break;

    case ADP1050_IIN_SCALE_MONITOR:
        s->iin_scale_monitor = pmbus_receive16(pmdev);
        break;

    case PMBUS_IC_DEVICE_ID:
    case PMBUS_IC_DEVICE_REV:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: %s: writing to read only register: 0x%02x\n",
                      DEVICE(s)->canonical_path, __func__, pmdev->code);
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: %s: writing to unimplemented register: 0x%02x\n",
                      DEVICE(s)->canonical_path, __func__, pmdev->code);
        return 0;
    }
    return 0;
}

static void adp1050_exit_reset(Object *obj)
{
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    ADP1050State *s = ADP1050(obj);

    pmdev->capability = ADP1050_DEFAULT_CAPABILITY;
    pmdev->pages[0].operation = ADP1050_DEFAULT_OPERATION;
    pmdev->pages[0].vout_mode = ADP1050_DEFAULT_VOUT_MODE;
    pmdev->pages[0].vout_transition_rate = ADP1050_DEFAULT_VOUT_TRANSITION_RATE;
    pmdev->pages[0].vout_scale_loop = ADP1050_DEFAULT_VOUT_SCALE_LOOP;
    pmdev->pages[0].vout_scale_monitor = ADP1050_DEFAULT_VOUT_SCALE_MONITOR;
    pmdev->pages[0].frequency_switch = ADP1050_DEFAULT_FREQUENCY_SWITCH;
    pmdev->pages[0].ton_rise = ADP1050_DEFAULT_TON_RISE;
    pmdev->pages[0].revision = ADP1050_DEFAULT_REVISION;
    s->ic_device_id = ADP1050_DEFAULT_IC_DEVICE_ID;
    s->ic_device_rev = ADP1050_DEFAULT_IC_DEVICE_REV;
    s->vin_scale_monitor = ADP1050_DEFAULT_VIN_SCALE_MONITOR;
    s->iin_scale_monitor = ADP1050_DEFAULT_IIN_SCALE_MONITOR;

    /* random sensor readings */
    pmdev->pages[0].read_vin = 0x100;
    pmdev->pages[0].read_vout = 0x100;
    pmdev->pages[0].read_iin = 0x10;
    pmdev->pages[0].read_temperature_1 = 30;
}

static void adp1050_get(Object *obj, Visitor *v, const char *name, void *opaque,
                        Error **errp)
{
    uint32_t value;
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    PMBusVoutMode *mode = (PMBusVoutMode *)&pmdev->pages[0].vout_mode;

    if (strcmp(name, "vout") == 0) {
        value = pmbus_linear_mode2data(*(uint16_t *)opaque, mode->exp);
    } else {
        value = *(uint16_t *)opaque;
    }

    value *= 1000; /* use milliunits for qmp */
    visit_type_uint32(v, name, &value, errp);
}

static void adp1050_set(Object *obj, Visitor *v, const char *name, void *opaque,
                        Error **errp)
{
    uint16_t *internal = opaque;
    uint32_t value;
    PMBusDevice *pmdev = PMBUS_DEVICE(obj);
    PMBusVoutMode *mode = (PMBusVoutMode *)&pmdev->pages[0].vout_mode;

    if (!visit_type_uint32(v, name, &value, errp)) {
        return;
    }

    /* use milliunits for qmp */
    value /= 1000;
    if (strcmp(name, "vout") == 0) {
        *internal = pmbus_data2linear_mode(value, mode->exp);
    } else {
        *internal = value;
    }
    pmbus_check_limits(pmdev);
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
                         PB_HAS_VOUT_MODE | PB_HAS_IIN | PB_HAS_TEMPERATURE;

    pmbus_page_config(pmdev, 0, psu_flags);
    object_property_add(obj, "vin", "uint32", adp1050_get, adp1050_set,
                        NULL, &pmdev->pages[0].read_vin);
    object_property_add(obj, "iin", "uint32", adp1050_get, adp1050_set,
                        NULL, &pmdev->pages[0].read_iin);
    object_property_add(obj, "vout", "uint32", adp1050_get, adp1050_set,
                        NULL, &pmdev->pages[0].read_vout);
    object_property_add(obj, "temperature", "uint32", adp1050_get, adp1050_set,
                        NULL, &pmdev->pages[0].read_temperature_1);
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

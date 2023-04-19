/*
 * AMD SBI Temperature Sensor Interface (SB-TSI)
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
#include "hw/i2c/smbus_slave.h"
#include "hw/irq.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "trace.h"

#define TYPE_SBTSI "sbtsi"
#define SBTSI(obj) OBJECT_CHECK(SBTSIState, (obj), TYPE_SBTSI)

/**
 * SBTSIState:
 * temperatures are in units of 0.125 degrees
 * @temperature: Temperature
 * @limit_low: Lowest temperature
 * @limit_high: Highest temperature
 * @status: The status register
 * @config: The config register
 * @alert_config: The config for alarm_l output.
 * @addr: The address to read/write for the next cmd.
 * @alarm: The alarm_l output pin (GPIO)
 */
typedef struct SBTSIState {
    SMBusDevice parent;

    uint32_t temperature;
    uint32_t limit_low;
    uint32_t limit_high;
    uint8_t status;
    uint8_t config;
    uint8_t alert_config;
    uint8_t addr;
    qemu_irq alarm;
} SBTSIState;

/*
 * SB-TSI registers only support SMBus byte data access. "_INT" registers are
 * the integer part of a temperature value or limit, and "_DEC" registers are
 * corresponding decimal parts.
 */
#define SBTSI_REG_TEMP_INT      0x01 /* RO */
#define SBTSI_REG_STATUS        0x02 /* RO */
#define SBTSI_REG_CONFIG        0x03 /* RO */
#define SBTSI_REG_TEMP_HIGH_INT 0x07 /* RW */
#define SBTSI_REG_TEMP_LOW_INT  0x08 /* RW */
#define SBTSI_REG_CONFIG_WR     0x09 /* RW */
#define SBTSI_REG_TEMP_DEC      0x10 /* RO */
#define SBTSI_REG_TEMP_HIGH_DEC 0x13 /* RW */
#define SBTSI_REG_TEMP_LOW_DEC  0x14 /* RW */
#define SBTSI_REG_ALERT_CONFIG  0xBF /* RW */
#define SBTSI_REG_MAN           0xFE /* RO */
#define SBTSI_REG_REV           0xFF /* RO */

#define SBTSI_STATUS_HIGH_ALERT BIT(4)
#define SBTSI_STATUS_LOW_ALERT  BIT(3)
#define SBTSI_CONFIG_ALERT_MASK BIT(7)
#define SBTSI_ALARM_EN          BIT(0)

#define SBTSI_LIMIT_LOW_DEFAULT (0)
#define SBTSI_LIMIT_HIGH_DEFAULT (560)
#define SBTSI_MAN_DEFAULT (0)
#define SBTSI_REV_DEFAULT (4)
#define SBTSI_ALARM_L "alarm_l"

/* The temperature we stored are in units of 0.125 degrees. */
#define SBTSI_TEMP_UNIT_IN_MILLIDEGREE 125

/*
 * The integer part and decimal of the temperature both 8 bits.
 * Only the top 3 bits of the decimal parts are used.
 * So the max temperature is (2^8-1) + (2^3-1)/8 = 255.875 degrees.
 */
#define SBTSI_TEMP_MAX 255875

/* The integer part of the temperature in terms of 0.125 degrees. */
static uint8_t get_temp_int(uint32_t temp)
{
    return temp >> 3;
}

/*
 * The decimal part of the temperature, in terms of 0.125 degrees.
 * H/W store it in the top 3 bits so we shift it by 5.
 */
static uint8_t get_temp_dec(uint32_t temp)
{
    return (temp & 0x7) << 5;
}

/*
 * Compute the temperature using the integer and decimal part,
 * in terms of 0.125 degrees. The decimal part are only the top 3 bits
 * so we shift it by 5 here.
 */
static uint32_t compute_temp(uint8_t integer, uint8_t decimal)
{
    return (integer << 3) + (decimal >> 5);
}

/* Compute new temp with new int part of the temperature. */
static uint32_t compute_temp_int(uint32_t temp, uint8_t integer)
{
    return compute_temp(integer, get_temp_dec(temp));
}

/* Compute new temp with new dec part of the temperature. */
static uint32_t compute_temp_dec(uint32_t temp, uint8_t decimal)
{
    return compute_temp(get_temp_int(temp), decimal);
}

/* The integer part of the temperature. */
static void sbtsi_update_status(SBTSIState *s)
{
    s->status = 0;
    if (s->alert_config & SBTSI_ALARM_EN) {
        if (s->temperature >= s->limit_high) {
            s->status |= SBTSI_STATUS_HIGH_ALERT;
        }
        if (s->temperature <= s->limit_low) {
            s->status |= SBTSI_STATUS_LOW_ALERT;
        }
    }
}

static void sbtsi_update_alarm(SBTSIState *s)
{
    sbtsi_update_status(s);
    if (s->status != 0 && !(s->config & SBTSI_CONFIG_ALERT_MASK)) {
        qemu_irq_raise(s->alarm);
    } else {
        qemu_irq_lower(s->alarm);
    }
}

static uint8_t sbtsi_read_byte(SMBusDevice *d)
{
    SBTSIState *s = SBTSI(d);
    uint8_t data = 0;

    switch (s->addr) {
    case SBTSI_REG_TEMP_INT:
        data = get_temp_int(s->temperature);
        break;

    case SBTSI_REG_TEMP_DEC:
        data = get_temp_dec(s->temperature);
        break;

    case SBTSI_REG_TEMP_HIGH_INT:
        data = get_temp_int(s->limit_high);
        break;

    case SBTSI_REG_TEMP_LOW_INT:
        data = get_temp_int(s->limit_low);
        break;

    case SBTSI_REG_TEMP_HIGH_DEC:
        data = get_temp_dec(s->limit_high);
        break;

    case SBTSI_REG_TEMP_LOW_DEC:
        data = get_temp_dec(s->limit_low);
        break;

    case SBTSI_REG_CONFIG:
    case SBTSI_REG_CONFIG_WR:
        data = s->config;
        break;

    case SBTSI_REG_STATUS:
        sbtsi_update_alarm(s);
        data = s->status;
        break;

    case SBTSI_REG_ALERT_CONFIG:
        data = s->alert_config;
        break;

    case SBTSI_REG_MAN:
        data = SBTSI_MAN_DEFAULT;
        break;

    case SBTSI_REG_REV:
        data = SBTSI_REV_DEFAULT;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                "%s: reading from invalid reg: 0x%02x\n",
                __func__, s->addr);
        break;
    }

    trace_tmp_sbtsi_read_data(s->addr, data);
    return data;
}

static void sbtsi_write(SBTSIState *s, uint8_t data)
{
    trace_tmp_sbtsi_write_data(s->addr, data);
    switch (s->addr) {
    case SBTSI_REG_CONFIG_WR:
        s->config = data;
        break;

    case SBTSI_REG_TEMP_HIGH_INT:
        s->limit_high = compute_temp_int(s->limit_high, data);
        break;

    case SBTSI_REG_TEMP_LOW_INT:
        s->limit_low = compute_temp_int(s->limit_low, data);
        break;

    case SBTSI_REG_TEMP_HIGH_DEC:
        s->limit_high = compute_temp_dec(s->limit_high, data);
        break;

    case SBTSI_REG_TEMP_LOW_DEC:
        s->limit_low = compute_temp_dec(s->limit_low, data);
        break;

    case SBTSI_REG_ALERT_CONFIG:
        s->alert_config = data;
        break;

    case SBTSI_REG_TEMP_INT:
    case SBTSI_REG_TEMP_DEC:
    case SBTSI_REG_CONFIG:
    case SBTSI_REG_STATUS:
    case SBTSI_REG_MAN:
    case SBTSI_REG_REV:
        qemu_log_mask(LOG_GUEST_ERROR,
                "%s: writing to read only reg: 0x%02x data: 0x%02x\n",
                __func__, s->addr, data);
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                "%s: writing to invalid reg: 0x%02x data: 0x%02x\n",
                __func__, s->addr, data);
        break;
    }
    sbtsi_update_alarm(s);
}

static int sbtsi_write_data(SMBusDevice *d, uint8_t *buf, uint8_t len)
{
    SBTSIState *s = SBTSI(d);

    if (len == 0) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: writing empty data\n", __func__);
        return -1;
    }

    s->addr = buf[0];
    if (len > 1) {
        sbtsi_write(s, buf[1]);
        if (len > 2) {
            qemu_log_mask(LOG_GUEST_ERROR, "%s: extra data at end\n", __func__);
        }
    }
    return 0;
}

/* Units are millidegrees. */
static void sbtsi_get_temperature(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    SBTSIState *s = SBTSI(obj);
    uint32_t temp = s->temperature * SBTSI_TEMP_UNIT_IN_MILLIDEGREE;

    visit_type_uint32(v, name, &temp, errp);
}

/* Units are millidegrees. */
static void sbtsi_set_temperature(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    SBTSIState *s = SBTSI(obj);
    uint32_t temp;

    if (!visit_type_uint32(v, name, &temp, errp)) {
        return;
    }
    if (temp > SBTSI_TEMP_MAX) {
        error_setg(errp, "value %" PRIu32 ".%03" PRIu32 " C is out of range",
                   temp / 1000, temp % 1000);
        return;
    }

    s->temperature = temp / SBTSI_TEMP_UNIT_IN_MILLIDEGREE;
    sbtsi_update_alarm(s);
}

static int sbtsi_post_load(void *opaque, int version_id)
{
    SBTSIState *s = opaque;

    sbtsi_update_alarm(s);
    return 0;
}

static const VMStateDescription vmstate_sbtsi = {
    .name = "SBTSI",
    .version_id = 0,
    .minimum_version_id = 0,
    .post_load = sbtsi_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(temperature, SBTSIState),
        VMSTATE_UINT32(limit_low, SBTSIState),
        VMSTATE_UINT32(limit_high, SBTSIState),
        VMSTATE_UINT8(config, SBTSIState),
        VMSTATE_UINT8(status, SBTSIState),
        VMSTATE_UINT8(addr, SBTSIState),
        VMSTATE_END_OF_LIST()
    }
};

static void sbtsi_enter_reset(Object *obj, ResetType type)
{
    SBTSIState *s = SBTSI(obj);
    SMBusDeviceClass *sdc = SMBUS_DEVICE_GET_CLASS(&s->parent);

    s->config = 0;
    s->limit_low = SBTSI_LIMIT_LOW_DEFAULT;
    s->limit_high = SBTSI_LIMIT_HIGH_DEFAULT;
    if (sdc->parent_phases.enter) {
        sdc->parent_phases.enter(obj, type);
    }
}

static void sbtsi_hold_reset(Object *obj)
{
    SBTSIState *s = SBTSI(obj);

    qemu_irq_lower(s->alarm);
}

static void sbtsi_init(Object *obj)
{
    SBTSIState *s = SBTSI(obj);

    /* Current temperature in millidegrees. */
    object_property_add(obj, "temperature", "uint32",
                        sbtsi_get_temperature, sbtsi_set_temperature,
                        NULL, NULL);
    qdev_init_gpio_out_named(DEVICE(obj), &s->alarm, SBTSI_ALARM_L, 0);
}

static void sbtsi_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    SMBusDeviceClass *k = SMBUS_DEVICE_CLASS(klass);

    dc->desc = "SB-TSI Temperature Sensor";
    dc->vmsd = &vmstate_sbtsi;
    k->write_data = sbtsi_write_data;
    k->receive_byte = sbtsi_read_byte;
    rc->phases.enter = sbtsi_enter_reset;
    rc->phases.hold = sbtsi_hold_reset;
}

static const TypeInfo sbtsi_info = {
    .name          = TYPE_SBTSI,
    .parent        = TYPE_SMBUS_DEVICE,
    .instance_size = sizeof(SBTSIState),
    .instance_init = sbtsi_init,
    .class_init    = sbtsi_class_init,
};

static void sbtsi_register_types(void)
{
    type_register_static(&sbtsi_info);
}

type_init(sbtsi_register_types)

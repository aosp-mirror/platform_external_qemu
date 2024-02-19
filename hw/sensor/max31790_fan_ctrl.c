/*
 * MAX31790 Fan controller
 *
 * Independently control 6 fans, up to 12 tachometer inputs,
 * controlled through i2c
 *
 * This device model has read/write support for:
 * - 9-bit pwm through i2c and qom/qmp
 * - 11-bit tach_count through i2c
 * - RPM through qom/qmp
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/sensor/max31790_fan_ctrl.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qapi/visitor.h"
#include "qemu/log.h"
#include "qemu/module.h"

static uint16_t max31790_get_sr(uint8_t fan_dynamics)
{
    uint16_t sr = 1 << ((fan_dynamics >> 5) & 0b111);
    return sr > 16 ? 32 : sr;
}

static void max31790_place_bits(uint16_t *dest, uint16_t byte, uint8_t offset)
{
    uint16_t val = *dest;
    val &= ~(0x00FF << offset);
    val |= byte << offset;
    *dest = val;
}

/*
 * calculating fan speed
 *  f_TOSC/4 is the clock, 8192Hz
 *  NP = tachometer pulses per revolution (usually 2)
 *  SR = number of periods(pulses) over which the clock ticks are counted
 *  TACH Count = SR x 8192 x 60 / (NP x RPM)
 *  RPM = SR x 8192 x 60 / (NP x TACH count)
 *
 *  RPM mode - desired tach count is written to TACH Target Count
 *  PWM mode - desired duty cycle is written to PWMOUT Target Duty reg
 */
static void max31790_calculate_tach_count(MAX31790State *ms, uint8_t id)
{
    uint32_t rpm;
    uint32_t sr = max31790_get_sr(ms->fan_dynamics[id]);
    ms->pwm_duty_cycle[id] = ms->pwmout[id] >> 7;
    rpm = (ms->max_rpm[id] * ms->pwm_duty_cycle[id]) / 0x1FF;

    if (rpm) {
        ms->tach_count[id] = (sr * MAX31790_CLK_FREQ * 60) /
                             (MAX31790_PULSES_PER_REV * rpm);
    } else {
        ms->tach_count[id] = 0;
    }

}

static void max31790_update_tach_count(MAX31790State *ms)
{
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        if (ms->fan_config[i] & MAX31790_FAN_CFG_RPM_MODE) {
            ms->tach_count[i] = ms->target_count[i] >> 5;
        } else { /* PWM mode */
            max31790_calculate_tach_count(ms, i);
        }
    }
}

/* consecutive reads can increment the address up to 0xFF then wrap to 0 */
/* slave to master */
static uint8_t max31790_recv(I2CSlave *i2c)
{
    MAX31790State *ms = MAX31790(i2c);
    uint8_t data, index, rem;

    max31790_update_tach_count(ms);

    if (ms->cmd_is_new) {
        ms->cmd_is_new = false;
    } else {
        ms->command++;
    }

    switch (ms->command) {
    case MAX31790_REG_GLOBAL_CONFIG:
        data = ms->global_config;
        break;

    case MAX31790_REG_PWM_FREQ:
        data = ms->pwm_freq;
        break;

    case MAX31790_REG_FAN_CONFIG(0) ...
         MAX31790_REG_FAN_CONFIG(MAX31790_NUM_FANS - 1):
        data = ms->fan_config[ms->command - MAX31790_REG_FAN_CONFIG(0)];
        break;

    case MAX31790_REG_FAN_DYNAMICS(0) ...
         MAX31790_REG_FAN_DYNAMICS(MAX31790_NUM_FANS - 1):
        data = ms->fan_dynamics[ms->command - MAX31790_REG_FAN_DYNAMICS(0)];
        break;

    case MAX31790_REG_FAN_FAULT_STATUS_2:
        data = ms->fan_fault_status_2;
        break;

    case MAX31790_REG_FAN_FAULT_STATUS_1:
        data = ms->fan_fault_status_1;
        break;

    case MAX31790_REG_FAN_FAULT_MASK_2:
        data = ms->fan_fault_mask_2;
        break;

    case MAX31790_REG_FAN_FAULT_MASK_1:
        data = ms->fan_fault_mask_1;
        break;

    case MAX31790_REG_FAILED_FAN_OPTS_SEQ_STRT:
        data = ms->failed_fan_opts_seq_strt;
        break;

    case MAX31790_REG_TACH_COUNT_MSB(0) ...
         MAX31790_REG_TACH_COUNT_LSB(MAX31790_NUM_TACHS - 1):
        index = (ms->command - MAX31790_REG_TACH_COUNT_MSB(0)) / 2;
        rem = (ms->command - MAX31790_REG_TACH_COUNT_MSB(0)) % 2;
        if (rem) {
            data = ms->tach_count[index] << 5;
        } else {
            data = ms->tach_count[index] >> 3;
        }
        break;

    /*
     * PWM_DUTY_CYCLE is meant to be the current duty cycle while
     * PWMOUT is the requested duty cycle
     */
    case MAX31790_REG_PWM_DUTY_CYCLE_MSB(0) ...
         MAX31790_REG_PWM_DUTY_CYCLE_LSB(MAX31790_NUM_FANS - 1):
        index = (ms->command - MAX31790_REG_PWM_DUTY_CYCLE_MSB(0)) / 2;
        rem = (ms->command - MAX31790_REG_PWM_DUTY_CYCLE_MSB(0)) % 2;

        if (rem) {
            data = ms->pwm_duty_cycle[index] << 7;
        } else {
            data = ms->pwm_duty_cycle[index] >> 1;
        }
        break;

    case MAX31790_REG_PWMOUT_MSB(0) ...
         MAX31790_REG_PWMOUT_LSB(MAX31790_NUM_FANS - 1):
        index = (ms->command - MAX31790_REG_PWMOUT_MSB(0)) / 2;
        rem = (ms->command - MAX31790_REG_PWMOUT_MSB(0)) % 2;
        if (rem) {
            data = ms->pwmout[index];
        } else {
            data = ms->pwmout[index] >> 8;
        }
        break;

    case MAX31790_REG_TARGET_COUNT_MSB(0) ...
         MAX31790_REG_TARGET_COUNT_LSB(MAX31790_NUM_FANS - 1):
        index = (ms->command - MAX31790_REG_TARGET_COUNT_MSB(0)) / 2;
        rem = (ms->command - MAX31790_REG_TARGET_COUNT_MSB(0)) % 2;
        if (rem) {
            data = ms->target_count[index];
        } else {
            data = ms->target_count[index] >> 8;
        }
        break;

    case MAX31790_REG_WINDOW(0) ...
         MAX31790_REG_WINDOW(MAX31790_NUM_FANS - 1):
        data = ms->window[ms->command - MAX31790_REG_WINDOW(0)];
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                "%s: reading from unsupported register 0x%02x",
                __func__, ms->command);
        data = 0xFF;
        break;
    }

    return data;
}

/*
 * The device’s control registers are organized in rows of 8 bytes.
 * The I2C master can read or write individual bytes, or can read or write
 * multiple bytes. When writing consecutive bytes, all writes are to the same
 * row. When the final byte in the row is reached, the next byte written is the
 * row’s first byte.
 * For example, a write that starts with 02h (Fan 1 Configuration) can write to
 * 02h, 03h, 04h, 05h, 06h, and 07h. If writes continue, the next byte written
 * is 00h, and so on.
 */
/* master to slave */
static int max31790_send(I2CSlave *i2c, uint8_t data)
{
    MAX31790State *ms = MAX31790(i2c);
    uint8_t index, rem;

    if (ms->i2c_cmd_event) {
        ms->command = data;
        ms->i2c_cmd_event = false;
        ms->cmd_is_new = true;
        return 0;
    }

    if (ms->cmd_is_new) {
        ms->cmd_is_new = false;
    } else {
        if ((ms->command + 1) % 8) {
            ms->command++;
        } else {
            ms->command -= 7;
        }
    }

    switch (ms->command) {
    case MAX31790_REG_GLOBAL_CONFIG:
        ms->global_config = data;
        break;

    case MAX31790_REG_PWM_FREQ:
        ms->pwm_freq = data;
        break;

    case MAX31790_REG_FAN_CONFIG(0) ...
         MAX31790_REG_FAN_CONFIG(MAX31790_NUM_FANS - 1):
        ms->fan_config[ms->command - MAX31790_REG_FAN_CONFIG(0)] = data;
        break;

    case MAX31790_REG_FAN_DYNAMICS(0) ...
         MAX31790_REG_FAN_DYNAMICS(MAX31790_NUM_FANS - 1):
        ms->fan_dynamics[ms->command - MAX31790_REG_FAN_DYNAMICS(0)] = data;
        break;

    case MAX31790_REG_FAN_FAULT_STATUS_2:
        ms->fan_fault_status_2 = data;
        break;

    case MAX31790_REG_FAN_FAULT_STATUS_1:
        ms->fan_fault_status_1 = data;
        break;

    case MAX31790_REG_FAN_FAULT_MASK_2:
        ms->fan_fault_mask_2 = data;
        break;

    case MAX31790_REG_FAN_FAULT_MASK_1:
        ms->fan_fault_mask_1 = data;
        break;

    case MAX31790_REG_FAILED_FAN_OPTS_SEQ_STRT:
        ms->failed_fan_opts_seq_strt = data;
        break;

    case MAX31790_REG_PWMOUT_MSB(0) ...
         MAX31790_REG_PWMOUT_LSB(MAX31790_NUM_FANS - 1):
        index = (ms->command - MAX31790_REG_PWMOUT_MSB(0)) / 2;
        rem = (ms->command - MAX31790_REG_PWMOUT_MSB(0)) % 2;
        if (rem) {
            max31790_place_bits(&ms->pwmout[index], data, 0);
        } else {
            max31790_place_bits(&ms->pwmout[index], data, 8);
        }
        break;

    case MAX31790_REG_TARGET_COUNT_MSB(0) ...
         MAX31790_REG_TARGET_COUNT_LSB(MAX31790_NUM_FANS - 1):
        index = (ms->command - MAX31790_REG_TARGET_COUNT_MSB(0)) / 2;
        rem = (ms->command - MAX31790_REG_TARGET_COUNT_MSB(0)) % 2;
        if (rem) {
            max31790_place_bits(&ms->target_count[index], data, 0);
        } else {
            max31790_place_bits(&ms->target_count[index], data, 8);
        }
        break;

    case MAX31790_REG_WINDOW(0) ...
         MAX31790_REG_WINDOW(MAX31790_NUM_FANS - 1):
        ms->window[ms->command - MAX31790_REG_WINDOW(0)] = data;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: writing to unsupported register 0x%02x",
                      __func__, ms->command);
        return -1;
    }

    return 0;
}

static int max31790_event(I2CSlave *i2c, enum i2c_event event)
{
    MAX31790State *ms = MAX31790(i2c);

    if (event == I2C_START_SEND) {
        ms->i2c_cmd_event = true;
    }

    return 0;
}

/* assumes that the fans have the same speed range (SR) */
static void max31790_get_rpm(Object *obj, Visitor *v, const char *name,
                             void *opaque, Error **errp)
{
    MAX31790State *ms = MAX31790(obj);
    uint16_t tach_count = *(uint16_t *)opaque;
    uint32_t sr = max31790_get_sr(ms->fan_dynamics[0]);
    uint16_t rpm = 0;

    max31790_update_tach_count(ms);
    tach_count >>= MAX31790_TACH_SHAMT;

    if (tach_count) {
        rpm = (sr * MAX31790_CLK_FREQ * 60) /
              (MAX31790_PULSES_PER_REV * tach_count);
    }

    visit_type_uint16(v, name, &rpm, errp);
}

static void max31790_set_rpm(Object *obj, Visitor *v, const char *name,
                             void *opaque, Error **errp)
{
    MAX31790State *ms = MAX31790(obj);
    uint16_t *internal = opaque;
    uint32_t sr = max31790_get_sr(ms->fan_dynamics[0]);
    uint16_t rpm, tach_count;

    if (!visit_type_uint16(v, name, &rpm, errp)) {
        return;
    }

    if (rpm) {
        tach_count = (sr * MAX31790_CLK_FREQ * 60) /
                     (MAX31790_PULSES_PER_REV * rpm);
    } else {
        tach_count = 0;
    }

    *internal = tach_count << MAX31790_TACH_SHAMT;
}

static void max31790_get(Object *obj, Visitor *v, const char *name,
                             void *opaque, Error **errp)
{
    MAX31790State *ms = MAX31790(obj);
    uint16_t value;

    max31790_update_tach_count(ms);

    if (strncmp(name, "pwm", 3) == 0) {
        value = *(uint16_t *)opaque >> 7;
    } else {
        value = *(uint16_t *)opaque;
    }

    visit_type_uint16(v, name, &value, errp);
}

static void max31790_set(Object *obj, Visitor *v, const char *name,
                             void *opaque, Error **errp)
{
    uint16_t *internal = opaque;
    uint16_t value;

    if (!visit_type_uint16(v, name, &value, errp)) {
        return;
    }

    if (strncmp(name, "pwm", 3) == 0) {
        *internal = value << MAX31790_PWM_SHAMT;
    } else {
        *internal = value;
    }

}

static void max31790_init(Object *obj)
{
    MAX31790State *ms = MAX31790(obj);

    ms->global_config = MAX31790_GLOBAL_CONFIG_DEFAULT;
    ms->pwm_freq = MAX31790_PWM_FREQ_DEFAULT;
    ms->failed_fan_opts_seq_strt = MAX31790_FAILED_FAN_OPTS_SEQ_STRT_DEFAULT;

    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        ms->max_rpm[i] = MAX31790_MAX_RPM_DEFAULT;
        ms->fan_config[i] = 0;
        ms->fan_dynamics[i] = MAX31790_FAN_DYNAMICS_DEFAULT;
        ms->pwmout[i] = MAX31790_PWMOUT_DEFAULT;
        ms->target_count[i] = MAX31790_TARGET_COUNT_DEFAULT;
    }

    max31790_update_tach_count(ms);
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        object_property_add(obj, "target_rpm[*]", "uint16",
                            max31790_get_rpm,
                            max31790_set_rpm, NULL, &ms->target_count[i]);

        /* 9-bit PWM on this device */
        object_property_add(obj, "pwm[*]", "uint16",
                            max31790_get,
                            max31790_set, NULL, &ms->pwmout[i]);

        /* used to calculate rpm for a given pwm duty cycle */
        object_property_add(obj, "max_rpm[*]", "uint16",
                            max31790_get,
                            max31790_set, NULL, &ms->max_rpm[i]);
    }
}

static void max31790_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

    dc->desc = "Maxim MAX31790 fan controller";

    k->event = max31790_event;
    k->recv = max31790_recv;
    k->send = max31790_send;
}

static const TypeInfo max31790_info = {
    .name = TYPE_MAX31790,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(MAX31790State),
    .instance_init = max31790_init,
    .class_init = max31790_class_init,
};

static void max31790_register_types(void)
{
    type_register_static(&max31790_info);
}

type_init(max31790_register_types)

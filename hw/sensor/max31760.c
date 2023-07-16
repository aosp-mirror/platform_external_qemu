/*
 * MAX31760 Fan Controller with Nonvolatile Lookup Table
 *
 * Features:
 *  - 2 tachometer inputs
 *  - Remote temperature sensing
 *
 * Datasheet:
 * https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31760.pdf
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/sensor/max31760.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qapi/visitor.h"
#include "qemu/log.h"
#include "trace.h"

static uint8_t max31760_recv(I2CSlave *i2c)
{
    MAX31760State *ms = MAX31760(i2c);
    uint8_t data = 0;

    switch (ms->command) {
    case A_CR1 ... A_SR:
        data = ms->reg[ms->command];
        break;

    case A_EEX:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: %s: reading from write-only register 0x%02x\n",
                      __func__, DEVICE(ms)->canonical_path, ms->command);
        data = 0xFF;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: %s: reading from unsupported register 0x%02x\n",
                      __func__, DEVICE(ms)->canonical_path, ms->command);
        data = 0xFF;
        break;
    }

    trace_max31760_recv(DEVICE(ms)->canonical_path, ms->command, data);
    return data;
}

static int max31760_send(I2CSlave *i2c, uint8_t data)
{
    MAX31760State *ms = MAX31760(i2c);

    if (ms->i2c_cmd_event) {
        ms->command = data;
        ms->i2c_cmd_event = false;
        return 0;
    }

    trace_max31760_send(DEVICE(ms)->canonical_path, ms->command, data);

    switch (ms->command) {
    case A_CR1 ... A_PWMR:
        ms->reg[ms->command] = data;
        break;

    case A_PWMV ... A_SR:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: %s: writing to read-only register 0x%02x\n",
                      __func__, DEVICE(ms)->canonical_path, ms->command);
        break;

    case A_EEX: /* Load EEPROM to RAM; Write RAM to EEPROM */
        ms->reg[A_EEX] = data;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: %s: writing to unsupported register 0x%02x\n",
                      __func__, DEVICE(ms)->canonical_path, ms->command);
    }

    return 0;
}

static int max31760_event(I2CSlave *i2c, enum i2c_event event)
{
    MAX31760State *ms = MAX31760(i2c);

    switch (event) {
    case I2C_START_RECV:
        trace_max31760_event(DEVICE(ms)->canonical_path, "START_RECV");
        break;

    case I2C_START_SEND:
        ms->i2c_cmd_event = true;
        trace_max31760_event(DEVICE(ms)->canonical_path, "START_SEND");
        break;

    case I2C_START_SEND_ASYNC:
        trace_max31760_event(DEVICE(ms)->canonical_path, "START_SEND_ASYNC");
        break;

    case I2C_FINISH:
        trace_max31760_event(DEVICE(ms)->canonical_path, "FINISH");
        break;

    case I2C_NACK:
        trace_max31760_event(DEVICE(ms)->canonical_path, "NACK");
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: %s: unexpected event %u",
                      DEVICE(ms)->canonical_path, __func__, event);
        return -1;
    }

    return 0;
}

static void max31760_exit_reset(Object *obj)
{
    MAX31760State *ms = MAX31760(obj);

    memset(ms->reg, 0, sizeof ms->reg);

    ms->reg[A_CR1] = MAX31760_CR1_DEFAULT;
    ms->reg[A_CR2] = MAX31760_CR2_DEFAULT;
    ms->reg[A_CR3] = MAX31760_CR3_DEFAULT;
    ms->reg[A_FFDC] = MAX31760_FFDC_DEFAULT;
    ms->reg[A_MASK] = MAX31760_MASK_DEFAULT;
    ms->reg[A_IFR] = MAX31760_IFR_DEFAULT;
    ms->reg[A_RHSH] = MAX31760_RHSH_DEFAULT;
    ms->reg[A_RHSL] = MAX31760_RHSL_DEFAULT;
    ms->reg[A_LOTSH] = MAX31760_LOTSH_DEFAULT;
    ms->reg[A_LOTSL] = MAX31760_LOTSL_DEFAULT;
    ms->reg[A_ROTSH] = MAX31760_ROTSH_DEFAULT;
    ms->reg[A_ROTSL] = MAX31760_ROTSL_DEFAULT;
    ms->reg[A_LHSH] = MAX31760_LHSH_DEFAULT;
    ms->reg[A_LHSL] = MAX31760_LHSL_DEFAULT;
    ms->reg[A_TCTH] = MAX31760_TCTH_DEFAULT;
    ms->reg[A_TCTL] = MAX31760_TCTL_DEFAULT;

    memset(ms->reg + A_LUT, 0xFF, MAX31760_LUT_SIZE);

}

static void max31760_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

    dc->desc = "Maxim MAX31760 fan controller";

    k->event = max31760_event;
    k->recv = max31760_recv;
    k->send = max31760_send;

    rc->phases.exit = max31760_exit_reset;

}

static const TypeInfo max31760_types[] = {
    {
        .name = TYPE_MAX31760,
        .parent = TYPE_I2C_SLAVE,
        .instance_size = sizeof(MAX31760State),
        .class_init = max31760_class_init,
    },
};

DEFINE_TYPES(max31760_types)

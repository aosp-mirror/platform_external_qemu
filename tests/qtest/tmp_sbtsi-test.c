/*
 * QTests for the SBTSI temperature sensor
 *
 * Copyright 2020 Google LLC
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

#include "libqtest-single.h"
#include "libqos/qgraph.h"
#include "libqos/i2c.h"
#include "qapi/qmp/qdict.h"
#include "qemu/bitops.h"

#define TEST_ID   "sbtsi-test"
#define TEST_ADDR (0x4c)

/*
 * SB-TSI registers only support SMBus byte data access. "_INT" registers are
 * the integer part of a temperature value or limit, and "_DEC" registers are
 * corresponding decimal parts.
 */
#define REG_TEMP_INT      0x01 /* RO */
#define REG_STATUS        0x02 /* RO */
#define REG_CONFIG        0x03 /* RO */
#define REG_TEMP_HIGH_INT 0x07 /* RW */
#define REG_TEMP_LOW_INT  0x08 /* RW */
#define REG_CONFIG_WR     0x09 /* RW */
#define REG_TEMP_DEC      0x10 /* RO */
#define REG_TEMP_HIGH_DEC 0x13 /* RW */
#define REG_TEMP_LOW_DEC  0x14 /* RW */
#define REG_ALERT_CONFIG  0xBF /* RW */

#define STATUS_HIGH_ALERT BIT(4)
#define STATUS_LOW_ALERT  BIT(3)
#define CONFIG_ALERT_MASK BIT(7)
#define ALARM_EN          BIT(0)

/* The temperature stored are in units of 0.125 degrees. */
#define TEMP_UNIT_IN_MILLIDEGREE 125
#define LIMIT_LOW (10500)
#define LIMIT_HIGH (55125)

static uint32_t qmp_sbtsi_get_temperature(const char *id)
{
    QDict *response;
    int ret;

    response = qmp("{ 'execute': 'qom-get', 'arguments': { 'path': %s, "
                   "'property': 'temperature' } }", id);
    g_assert(qdict_haskey(response, "return"));
    ret = (uint32_t)qdict_get_int(response, "return");
    qobject_unref(response);
    return ret;
}

static void qmp_sbtsi_set_temperature(const char *id, uint32_t value)
{
    QDict *response;

    response = qmp("{ 'execute': 'qom-set', 'arguments': { 'path': %s, "
                   "'property': 'temperature', 'value': %d } }", id, value);
    g_assert(qdict_haskey(response, "return"));
    qobject_unref(response);
}

/*
 * Compute the temperature using the integer and decimal part and return
 * millidegrees. The decimal part are only the top 3 bits so we shift it by
 * 5 here.
 */
static uint32_t regs_to_temp(uint8_t integer, uint8_t decimal)
{
    return ((integer << 3) + (decimal >> 5)) * TEMP_UNIT_IN_MILLIDEGREE;
}

/*
 * Compute the integer and decimal parts of the temperature in millidegrees.
 * H/W store the decimal in the top 3 bits so we shift it by 5.
 */
static void temp_to_regs(uint32_t temp, uint8_t *integer, uint8_t *decimal)
{
    temp /= TEMP_UNIT_IN_MILLIDEGREE;
    *integer = temp >> 3;
    *decimal = (temp & 0x7) << 5;
}

static void tx_rx(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t value;
    uint8_t integer, decimal;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    /* Test default values */
    value = qmp_sbtsi_get_temperature(TEST_ID);
    g_assert_cmpuint(value, ==, 0);

    integer = i2c_get8(i2cdev, REG_TEMP_INT);
    decimal = i2c_get8(i2cdev, REG_TEMP_DEC);
    g_assert_cmpuint(regs_to_temp(integer, decimal), ==, 0);

    /* Test setting temperature */
    qmp_sbtsi_set_temperature(TEST_ID, 20000);
    value = qmp_sbtsi_get_temperature(TEST_ID);
    g_assert_cmpuint(value, ==, 20000);

    integer = i2c_get8(i2cdev, REG_TEMP_INT);
    decimal = i2c_get8(i2cdev, REG_TEMP_DEC);
    g_assert_cmpuint(regs_to_temp(integer, decimal), ==, 20000);

    /* Set alert mask in config */
    i2c_set8(i2cdev, REG_CONFIG_WR, CONFIG_ALERT_MASK);
    value = i2c_get8(i2cdev, REG_CONFIG);
    g_assert_cmphex(value, ==, CONFIG_ALERT_MASK);
    /* Enable alarm_en */
    i2c_set8(i2cdev, REG_ALERT_CONFIG, ALARM_EN);
    value = i2c_get8(i2cdev, REG_ALERT_CONFIG);
    g_assert_cmphex(value, ==, ALARM_EN);

    /* Test setting limits */
    /* Limit low = 10.500 */
    temp_to_regs(LIMIT_LOW, &integer, &decimal);
    i2c_set8(i2cdev, REG_TEMP_LOW_INT, integer);
    i2c_set8(i2cdev, REG_TEMP_LOW_DEC, decimal);
    integer = i2c_get8(i2cdev, REG_TEMP_LOW_INT);
    decimal = i2c_get8(i2cdev, REG_TEMP_LOW_DEC);
    g_assert_cmpuint(regs_to_temp(integer, decimal), ==, LIMIT_LOW);
    /* Limit high = 55.125 */
    temp_to_regs(LIMIT_HIGH, &integer, &decimal);
    i2c_set8(i2cdev, REG_TEMP_HIGH_INT, integer);
    i2c_set8(i2cdev, REG_TEMP_HIGH_DEC, decimal);
    integer = i2c_get8(i2cdev, REG_TEMP_HIGH_INT);
    decimal = i2c_get8(i2cdev, REG_TEMP_HIGH_DEC);
    g_assert_cmpuint(regs_to_temp(integer, decimal), ==, LIMIT_HIGH);
    /* No alert is generated. */
    value = i2c_get8(i2cdev, REG_STATUS);
    g_assert_cmphex(value, ==, 0);

    /* Test alert for low temperature */
    qmp_sbtsi_set_temperature(TEST_ID, LIMIT_LOW);
    value = i2c_get8(i2cdev, REG_STATUS);
    g_assert_cmphex(value, ==, STATUS_LOW_ALERT);

    /* Test alert for high temperature */
    qmp_sbtsi_set_temperature(TEST_ID, LIMIT_HIGH);
    value = i2c_get8(i2cdev, REG_STATUS);
    g_assert_cmphex(value, ==, STATUS_HIGH_ALERT);

    /* Disable alarm_en */
    i2c_set8(i2cdev, REG_ALERT_CONFIG, 0);
    value = i2c_get8(i2cdev, REG_ALERT_CONFIG);
    g_assert_cmphex(value, ==, 0);
    /* No alert when alarm_en is false. */
    value = i2c_get8(i2cdev, REG_STATUS);
    g_assert_cmphex(value, ==, 0);
}

static void sbtsi_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {
        .extra_device_opts = "id=" TEST_ID ",address=0x4c"
    };
    add_qi2c_address(&opts, &(QI2CAddress) { TEST_ADDR });

    qos_node_create_driver("sbtsi", i2c_device_create);
    qos_node_consumes("sbtsi", "i2c-bus", &opts);

    qos_add_test("tx-rx", "sbtsi", tx_rx, NULL);
}
libqos_init(sbtsi_register_nodes);

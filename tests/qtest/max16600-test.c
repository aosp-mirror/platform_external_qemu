/*
 * QTest for the MAX16600 VR13.HC Dual-Output Voltage Regulator Chipset
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
#include "hw/sensor/max16600.h"
#include "libqtest-single.h"
#include "libqos/qgraph.h"
#include "libqos/i2c.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qnum.h"
#include "qemu/bitops.h"

#define TEST_ID "max16600-test"
#define TEST_ADDR (0x61)

uint16_t pmbus_linear_mode2data(uint16_t value, int exp)
{
    /* D = L * 2^e */
    if (exp < 0) {
        return value >> (-exp);
    }
    return value << exp;
}

static uint16_t qmp_max16600_get(const char *id, const char *property)
{
    QDict *response;
    uint64_t ret;

    response = qmp("{ 'execute': 'qom-get', 'arguments': { 'path': %s, "
                   "'property': %s } }",
                   id, property);
    g_assert(qdict_haskey(response, "return"));
    ret = qnum_get_uint(qobject_to(QNum, qdict_get(response, "return")));
    qobject_unref(response);
    return ret;
}

static void qmp_max16600_set(const char *id, const char *property,
                             uint16_t value)
{
    QDict *response;

    response = qmp("{ 'execute': 'qom-set', 'arguments': { 'path': %s, "
                   "'property': %s, 'value': %u } }",
                   id, property, value);
    g_assert(qdict_haskey(response, "return"));
    qobject_unref(response);
}

static uint16_t max16600_i2c_get16(QI2CDevice *i2cdev, uint8_t reg)
{
    uint8_t resp[2];
    i2c_read_block(i2cdev, reg, resp, sizeof(resp));
    return (resp[1] << 8) | resp[0];
}

static void max16600_i2c_set16(QI2CDevice *i2cdev, uint8_t reg, uint16_t value)
{
    uint8_t data[2];

    data[0] = value & 255;
    data[1] = value >> 8;
    i2c_write_block(i2cdev, reg, data, sizeof(data));
}

/* test default values */
static void test_defaults(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t i2c_value, value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    i2c_value = i2c_get8(i2cdev, PMBUS_CAPABILITY);
    g_assert_cmphex(i2c_value, ==, MAX16600_CAPABILITY_DEFAULT);

    i2c_value = i2c_get8(i2cdev, PMBUS_OPERATION);
    g_assert_cmphex(i2c_value, ==, MAX16600_OPERATION_DEFAULT);

    i2c_value = i2c_get8(i2cdev, PMBUS_ON_OFF_CONFIG);
    g_assert_cmphex(i2c_value, ==, MAX16600_ON_OFF_CONFIG_DEFAULT);

    i2c_value = i2c_get8(i2cdev, PMBUS_VOUT_MODE);
    g_assert_cmphex(i2c_value, ==, MAX16600_VOUT_MODE_DEFAULT);

    value = qmp_max16600_get(TEST_ID, "vin") / 1000;
    g_assert_cmpuint(value, ==, MAX16600_READ_VIN_DEFAULT);

    value = qmp_max16600_get(TEST_ID, "iin") / 1000;
    g_assert_cmpuint(value, ==, MAX16600_READ_IIN_DEFAULT);

    value = qmp_max16600_get(TEST_ID, "pin");
    g_assert_cmpuint(value, ==, MAX16600_READ_PIN_DEFAULT);

    value = qmp_max16600_get(TEST_ID, "vout") / 1000;
    g_assert_cmpuint(value, ==, MAX16600_READ_VOUT_DEFAULT);

    value = qmp_max16600_get(TEST_ID, "iout") / 1000;
    g_assert_cmpuint(value, ==, MAX16600_READ_IOUT_DEFAULT);

    value = qmp_max16600_get(TEST_ID, "pout");
    g_assert_cmpuint(value, ==, MAX16600_READ_POUT_DEFAULT);

    value = qmp_max16600_get(TEST_ID, "temperature") / 1000;
    g_assert_cmpuint(value, ==, MAX16600_READ_TEMP_DEFAULT);
}

/* test qmp access */
static void test_tx_rx(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t value, i2c_value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    qmp_max16600_set(TEST_ID, "vin", 2000);
    value = qmp_max16600_get(TEST_ID, "vin") / 1000;
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_VIN);
    i2c_value = pmbus_linear_mode2data(i2c_value, max16600_exp.vin);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_max16600_set(TEST_ID, "iin", 3000);
    value = qmp_max16600_get(TEST_ID, "iin") / 1000;
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_IIN);
    i2c_value = pmbus_linear_mode2data(i2c_value, max16600_exp.iin);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_max16600_set(TEST_ID, "pin", 4);
    value = qmp_max16600_get(TEST_ID, "pin");
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_PIN);
    i2c_value = pmbus_linear_mode2data(i2c_value, max16600_exp.pin);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_max16600_set(TEST_ID, "vout", 5000);
    value = qmp_max16600_get(TEST_ID, "vout") / 1000;
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_VOUT);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_max16600_set(TEST_ID, "iout", 6000);
    value = qmp_max16600_get(TEST_ID, "iout") / 1000;
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_IOUT);
    i2c_value = pmbus_linear_mode2data(i2c_value, max16600_exp.iout);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_max16600_set(TEST_ID, "pout", 7);
    value = qmp_max16600_get(TEST_ID, "pout");
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_POUT);
    i2c_value = pmbus_linear_mode2data(i2c_value, max16600_exp.pout);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_max16600_set(TEST_ID, "temperature", 8000);
    value = qmp_max16600_get(TEST_ID, "temperature") / 1000;
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_TEMPERATURE_1);
    i2c_value = pmbus_linear_mode2data(i2c_value, max16600_exp.temp);
    g_assert_cmpuint(value, ==, i2c_value);
}

/* test r/w registers */
static void test_rw_regs(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t i2c_value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    max16600_i2c_set16(i2cdev, PMBUS_OPERATION, 0xA);
    i2c_value = i2c_get8(i2cdev, PMBUS_OPERATION);
    g_assert_cmphex(i2c_value, ==, 0xA);

    max16600_i2c_set16(i2cdev, PMBUS_ON_OFF_CONFIG, 0xB);
    i2c_value = i2c_get8(i2cdev, PMBUS_ON_OFF_CONFIG);
    g_assert_cmphex(i2c_value, ==, 0xB);

    max16600_i2c_set16(i2cdev, PMBUS_VOUT_MODE, 0xC);
    i2c_value = i2c_get8(i2cdev, PMBUS_VOUT_MODE);
    g_assert_cmphex(i2c_value, ==, 0xC);
}

/* test read-only registers */
static void test_ro_regs(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t i2c_init_value, i2c_value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    i2c_init_value = i2c_get8(i2cdev, PMBUS_CAPABILITY);
    max16600_i2c_set16(i2cdev, PMBUS_CAPABILITY, 0xD);
    i2c_value = i2c_get8(i2cdev, PMBUS_CAPABILITY);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);

    i2c_init_value = max16600_i2c_get16(i2cdev, PMBUS_READ_VIN);
    max16600_i2c_set16(i2cdev, PMBUS_READ_VIN, 0x1234);
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_VIN);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);

    i2c_init_value = max16600_i2c_get16(i2cdev, PMBUS_READ_IIN);
    max16600_i2c_set16(i2cdev, PMBUS_READ_IIN, 0x2234);
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_IIN);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);

    i2c_init_value = max16600_i2c_get16(i2cdev, PMBUS_READ_PIN);
    max16600_i2c_set16(i2cdev, PMBUS_READ_PIN, 0x3234);
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_PIN);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);

    i2c_init_value = max16600_i2c_get16(i2cdev, PMBUS_READ_VOUT);
    max16600_i2c_set16(i2cdev, PMBUS_READ_VOUT, 0x4234);
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_VOUT);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);

    i2c_init_value = max16600_i2c_get16(i2cdev, PMBUS_READ_IOUT);
    max16600_i2c_set16(i2cdev, PMBUS_READ_IOUT, 0x5235);
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_IOUT);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);

    i2c_init_value = max16600_i2c_get16(i2cdev, PMBUS_READ_POUT);
    max16600_i2c_set16(i2cdev, PMBUS_READ_POUT, 0x6234);
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_POUT);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);

    i2c_init_value = max16600_i2c_get16(i2cdev, PMBUS_READ_TEMPERATURE_1);
    max16600_i2c_set16(i2cdev, PMBUS_READ_TEMPERATURE_1, 0x7236);
    i2c_value = max16600_i2c_get16(i2cdev, PMBUS_READ_TEMPERATURE_1);
    g_assert_cmphex(i2c_init_value, ==, i2c_value);
}

static void max16600_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {.extra_device_opts =
                                    "id=" TEST_ID ",address=0x61"};
    add_qi2c_address(&opts, &(QI2CAddress){TEST_ADDR});

    qos_node_create_driver("max16600", i2c_device_create);
    qos_node_consumes("max16600", "i2c-bus", &opts);

    qos_add_test("test_defaults", "max16600", test_defaults, NULL);
    qos_add_test("test_tx_rx", "max16600", test_tx_rx, NULL);
    qos_add_test("test_rw_regs", "max16600", test_rw_regs, NULL);
    qos_add_test("test_ro_regs", "max16600", test_ro_regs, NULL);
}
libqos_init(max16600_register_nodes);

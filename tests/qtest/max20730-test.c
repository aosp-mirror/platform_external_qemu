/*
 * MAX20730 Integrated, Step-Down Switching Regulator
 *
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include <math.h>
#include "hw/i2c/pmbus_device.h"
#include "libqtest-single.h"
#include "libqos/qgraph.h"
#include "libqos/i2c.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qnum.h"
#include "qemu/bitops.h"

#define TEST_ID "max20730-test"
#define TEST_ADDR (0x3A)

#define TYPE_MAX20730 "max20730"

#define MAX20730_MFR_VOUT_MIN 0xD1
#define MAX20730_MFR_DEVSET1  0xD2
#define MAX20730_MFR_DEVSET2  0xD3

#define MAX20730_OPERATION_DEFAULT          0x80
#define MAX20730_ON_OFF_CONFIG_DEFAULT      0x1F
#define MAX20730_VOUT_MODE_DEFAULT          0x17
#define MAX20730_VOUT_MAX_DEFAULT           0x280
#define MAX20730_MFR_VOUT_MIN_DEFAULT       0x133
#define MAX20730_MFR_DEVSET1_DEFAULT        0x2061
#define MAX20730_MFR_DEVSET2_DEFAULT        0x3A6

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

uint16_t pmbus_data2linear_mode(uint16_t value, int exp)
{
    /* L = D * 2^(-e) */
    if (exp < 0) {
        return value << (-exp);
    }
    return value >> exp;
}

static uint16_t qmp_max20730_get(const char *id, const char *property)
{
    QDict *response;
    uint64_t ret;

    response = qmp("{ 'execute': 'qom-get', 'arguments': { 'path': %s, "
                   "'property': %s } }", id, property);
    g_assert(qdict_haskey(response, "return"));
    ret = qnum_get_uint(qobject_to(QNum, qdict_get(response, "return")));
    qobject_unref(response);
    return ret;
}

static void qmp_max20730_set(const char *id,
                            const char *property,
                            uint16_t value)
{
    QDict *response;

    response = qmp("{ 'execute': 'qom-set', 'arguments': { 'path': %s, "
                   "'property': %s, 'value': %u } }", id, property, value);
    g_assert(qdict_haskey(response, "return"));
    qobject_unref(response);
}

/* PMBus commands are little endian vs i2c_set16 in i2c.h which is big endian */
static uint16_t max20730_i2c_get16(QI2CDevice *i2cdev, uint8_t reg)
{
    uint8_t resp[2];
    i2c_read_block(i2cdev, reg, resp, sizeof(resp));
    return (resp[1] << 8) | resp[0];
}

/* PMBus commands are little endian vs i2c_set16 in i2c.h which is big endian */
static void max20730_i2c_set16(QI2CDevice *i2cdev, uint8_t reg, uint16_t value)
{
    uint8_t data[2];

    data[0] = value & 255;
    data[1] = value >> 8;
    i2c_write_block(i2cdev, reg, data, sizeof(data));
}

static void test_defaults(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t i2c_value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    i2c_value = i2c_get8(i2cdev, PMBUS_OPERATION);
    g_assert_cmphex(i2c_value, ==, MAX20730_OPERATION_DEFAULT);

    i2c_value = i2c_get8(i2cdev, PMBUS_VOUT_MODE);
    g_assert_cmphex(i2c_value, ==, MAX20730_VOUT_MODE_DEFAULT);

    i2c_value = i2c_get8(i2cdev, PMBUS_ON_OFF_CONFIG);
    g_assert_cmphex(i2c_value, ==, MAX20730_ON_OFF_CONFIG_DEFAULT);

    i2c_value = max20730_i2c_get16(i2cdev, PMBUS_VOUT_MAX);
    g_assert_cmphex(i2c_value, ==, MAX20730_VOUT_MAX_DEFAULT);

    i2c_value = max20730_i2c_get16(i2cdev, MAX20730_MFR_VOUT_MIN);
    g_assert_cmphex(i2c_value, ==, MAX20730_MFR_VOUT_MIN_DEFAULT);

    i2c_value = max20730_i2c_get16(i2cdev, MAX20730_MFR_DEVSET1);
    g_assert_cmphex(i2c_value, ==, MAX20730_MFR_DEVSET1_DEFAULT);

    i2c_value = max20730_i2c_get16(i2cdev, MAX20730_MFR_DEVSET2);
    g_assert_cmphex(i2c_value, ==, MAX20730_MFR_DEVSET2_DEFAULT);
}

static void test_qmp_access(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t i2c_raw, value, i2c_value, lossy_value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    /* Writing vin over qmp can be read back over i2c and qmp */
    lossy_value = pmbus_direct_mode2data(max20730_c.vin,
                    pmbus_data2direct_mode(max20730_c.vin, 5));
    qmp_max20730_set(TEST_ID, "vin", 5);
    value = qmp_max20730_get(TEST_ID, "vin");
    i2c_raw = max20730_i2c_get16(i2cdev, PMBUS_READ_VIN);
    i2c_value = pmbus_direct_mode2data(max20730_c.vin, i2c_raw);
    g_assert_cmpuint(value, ==, i2c_value);
    g_assert_cmpuint(i2c_value, ==, lossy_value);

    /* Test that vin is read only over i2c */
    lossy_value = pmbus_direct_mode2data(max20730_c.vin,
                    pmbus_data2direct_mode(max20730_c.vin, 6));
    max20730_i2c_set16(i2cdev, PMBUS_READ_VIN,
        pmbus_data2direct_mode(max20730_c.vin, 6));
    value = qmp_max20730_get(TEST_ID, "vin");
    i2c_raw = max20730_i2c_get16(i2cdev, PMBUS_READ_VIN);
    i2c_value = pmbus_direct_mode2data(max20730_c.vin, i2c_raw);
    g_assert_cmpuint(value, !=, lossy_value);

    /* Writing iout over qmp can be read back over i2c and qmp */
    lossy_value = pmbus_direct_mode2data(max20730_c.iout,
                    pmbus_data2direct_mode(max20730_c.iout, 40));
    qmp_max20730_set(TEST_ID, "iout", 40);
    value = qmp_max20730_get(TEST_ID, "iout");
    i2c_raw = max20730_i2c_get16(i2cdev, PMBUS_READ_IOUT);
    i2c_value = pmbus_direct_mode2data(max20730_c.iout, i2c_raw);
    g_assert_cmpuint(value, ==, i2c_value);
    g_assert_cmpuint(i2c_value, ==, lossy_value);

    /* Writing temperature over qmp can be read back over i2c and qmp */
    lossy_value = pmbus_direct_mode2data(max20730_c.temperature,
                    pmbus_data2direct_mode(max20730_c.temperature, 50));
    qmp_max20730_set(TEST_ID, "temperature", 50);
    value = qmp_max20730_get(TEST_ID, "temperature");
    i2c_raw = max20730_i2c_get16(i2cdev, PMBUS_READ_TEMPERATURE_1);
    i2c_value = pmbus_direct_mode2data(max20730_c.temperature, i2c_raw);
    g_assert_cmpuint(value, ==, i2c_value);
    g_assert_cmpuint(i2c_value, ==, lossy_value);
}

static void test_voltage_div(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t i2c_raw, value, i2c_value, r_fb1, r_fb2, test_value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    /* test v_out = v_in/2 */
    r_fb1 = 1000; /* Ohms */
    r_fb2 = 1000; /* Ohms */
    test_value = 4000; /* mV */
    qmp_max20730_set(TEST_ID, "r_fb1", r_fb1);
    qmp_max20730_set(TEST_ID, "r_fb2", r_fb2);

    qmp_max20730_set(TEST_ID, "vout", test_value);
    value = qmp_max20730_get(TEST_ID, "vout");
    i2c_raw = max20730_i2c_get16(i2cdev, PMBUS_READ_VOUT);
    i2c_value = pmbus_linear_mode2data(i2c_raw, max20730_c.vout_exp);
    g_assert_cmpuint(value, ==, i2c_value);
    g_assert_cmpuint(i2c_value, ==, test_value / 2);

    /* test v_out = v_in/4 */
    r_fb1 = 3000; /* Ohms */
    qmp_max20730_set(TEST_ID, "r_fb1", r_fb1);
    qmp_max20730_set(TEST_ID, "r_fb2", r_fb2);
    qmp_max20730_set(TEST_ID, "vout", test_value);
    value = qmp_max20730_get(TEST_ID, "vout");
    i2c_raw = max20730_i2c_get16(i2cdev, PMBUS_READ_VOUT);
    i2c_value = pmbus_linear_mode2data(i2c_raw, max20730_c.vout_exp);
    g_assert_cmpuint(value, ==, i2c_value);
    g_assert_cmpuint(i2c_value, ==, test_value / 4);
}

static void max20730_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {
        .extra_device_opts = "id=" TEST_ID ",address=0x3A"
    };
    add_qi2c_address(&opts, &(QI2CAddress) { TEST_ADDR });

    qos_node_create_driver("max20730", i2c_device_create);
    qos_node_consumes("max20730", "i2c-bus", &opts);

    qos_add_test("test_defaults", "max20730", test_defaults, NULL);
    qos_add_test("test_qmp_access", "max20730", test_qmp_access, NULL);
    qos_add_test("test_voltage_divider", "max20730", test_voltage_div, NULL);
}
libqos_init(max20730_register_nodes);

/*
 * QTests Infineon TDA38740/25 PMBus Digital Conroller for Power Supply
 *
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/i2c/pmbus_device.h"
#include "libqtest-single.h"
#include "libqos/qgraph.h"
#include "libqos/i2c.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qnum.h"
#include "qemu/bitops.h"

#define TEST_ID "tda387xx-test"
#define TEST_ADDR (0x72)

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

static uint32_t qmp_tda387xx_get(const char *id, const char *property)
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

static void qmp_tda387xx_set(const char *id, const char *property,
                             uint32_t value)
{
    QDict *response;

    response = qmp("{ 'execute': 'qom-set', 'arguments': { 'path': %s, "
                   "'property': %s, 'value': %u } }",
                   id, property, value);
    g_assert(qdict_haskey(response, "return"));
    qobject_unref(response);
}

static uint16_t tda387xx_i2c_get16(QI2CDevice *i2cdev, uint8_t reg)
{
    uint8_t resp[2];
    i2c_read_block(i2cdev, reg, resp, sizeof(resp));
    return (resp[1] << 8) | resp[0];
}

/* test qmp access */
static void test_tx_rx(void *obj, void *data, QGuestAllocator *alloc)
{
    uint16_t value, i2c_value;
    QI2CDevice *i2cdev = (QI2CDevice *)obj;

    qmp_tda387xx_set(TEST_ID, "vin", 2000);
    value = qmp_tda387xx_get(TEST_ID, "vin") / 1000;
    i2c_value = tda387xx_i2c_get16(i2cdev, PMBUS_READ_VIN);
    i2c_value = pmbus_linear_mode2data(i2c_value, 0);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_tda387xx_set(TEST_ID, "iin", 3000);
    value = qmp_tda387xx_get(TEST_ID, "iin") / 1000;
    i2c_value = tda387xx_i2c_get16(i2cdev, PMBUS_READ_IIN);
    i2c_value = pmbus_linear_mode2data(i2c_value, 0);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_tda387xx_set(TEST_ID, "pin", 4);
    value = qmp_tda387xx_get(TEST_ID, "pin");
    i2c_value = tda387xx_i2c_get16(i2cdev, PMBUS_READ_PIN);
    i2c_value = pmbus_linear_mode2data(i2c_value, 0);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_tda387xx_set(TEST_ID, "vout", 0x10000);
    value = qmp_tda387xx_get(TEST_ID, "vout") / 1000;
    i2c_value = tda387xx_i2c_get16(i2cdev, PMBUS_READ_VOUT);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_tda387xx_set(TEST_ID, "iout", 6000);
    value = qmp_tda387xx_get(TEST_ID, "iout") / 1000;
    i2c_value = tda387xx_i2c_get16(i2cdev, PMBUS_READ_IOUT);
    i2c_value = pmbus_linear_mode2data(i2c_value, 0);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_tda387xx_set(TEST_ID, "pout", 7);
    value = qmp_tda387xx_get(TEST_ID, "pout");
    i2c_value = tda387xx_i2c_get16(i2cdev, PMBUS_READ_POUT);
    i2c_value = pmbus_linear_mode2data(i2c_value, 0);
    g_assert_cmpuint(value, ==, i2c_value);

    qmp_tda387xx_set(TEST_ID, "temperature", 8000);
    value = qmp_tda387xx_get(TEST_ID, "temperature") / 1000;
    i2c_value = tda387xx_i2c_get16(i2cdev, PMBUS_READ_TEMPERATURE_1);
    i2c_value = pmbus_linear_mode2data(i2c_value, 0);
    g_assert_cmpuint(value, ==, i2c_value);
}

static void tda387xx_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {.extra_device_opts =
                                    "id=" TEST_ID ",address=0x72"};
    add_qi2c_address(&opts, &(QI2CAddress){TEST_ADDR});

    qos_node_create_driver("tda387xx", i2c_device_create);
    qos_node_consumes("tda387xx", "i2c-bus", &opts);

    qos_add_test("test_tx_rx", "tda387xx", test_tx_rx, NULL);
}

libqos_init(tda387xx_register_nodes);

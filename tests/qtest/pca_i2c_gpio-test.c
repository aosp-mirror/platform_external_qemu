/*
 * QTest for PCA I2C GPIO expanders
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/gpio/pca_i2c_gpio.h"
#include "libqtest-single.h"
#include "libqos/i2c.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qnum.h"
#include "qemu/bitops.h"


#define TEST_ID "pca_i2c_gpio-test"
#define PCA_CONFIG_BYTE     0x55
#define PCA_CONFIG_WORD     0x5555

static uint16_t qmp_pca_gpio_get(const char *id, const char *property)
{
    QDict *response;
    uint16_t ret;
    response = qmp("{ 'execute': 'qom-get', 'arguments': { 'path': %s, "
                   "'property': %s } }", id, property);
    g_assert(qdict_haskey(response, "return"));
    ret = qnum_get_uint(qobject_to(QNum, qdict_get(response, "return")));
    qobject_unref(response);
    return ret;
}

static void qmp_pca_gpio_set(const char *id, const char *property,
                             uint16_t value)
{
    QDict *response;

    response = qmp("{ 'execute': 'qom-set', 'arguments': { 'path': %s, "
                   "'property': %s, 'value': %u } }",
                   id, property, value);
    g_assert(qdict_haskey(response, "return"));
    qobject_unref(response);
}

static void test_set_input(void *obj, void *data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    uint8_t value;
    uint16_t qmp_value;
    /* configure pins to be inputs */
    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_0, 0xFF);
    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_1, 0xFF);

    qmp_pca_gpio_set(TEST_ID, "gpio_input", 0xAAAA);
    value = i2c_get8(i2cdev, PCA6416_INPUT_PORT_0);
    g_assert_cmphex(value, ==, 0xAA);
    value = i2c_get8(i2cdev, PCA6416_INPUT_PORT_1);
    g_assert_cmphex(value, ==, 0xAA);

    qmp_value = qmp_pca_gpio_get(TEST_ID, "gpio_input");
    g_assert_cmphex(qmp_value, ==, 0xAAAA);
}
static void test_config(void *obj, void *data, QGuestAllocator *alloc)
{

    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    uint8_t value;
    uint16_t qmp_value;
    /* configure half the pins to be inputs */
    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_0, PCA_CONFIG_BYTE);
    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_1, PCA_CONFIG_BYTE);
    value = i2c_get8(i2cdev, PCA6416_CONFIGURATION_PORT_0);
    g_assert_cmphex(value, ==, PCA_CONFIG_BYTE);

    value = i2c_get8(i2cdev, PCA6416_CONFIGURATION_PORT_1);
    g_assert_cmphex(value, ==, PCA_CONFIG_BYTE);

    /* the pins that match the config should be set, the rest are undef */
    qmp_pca_gpio_set(TEST_ID, "gpio_input", 0xFFFF);
    value = i2c_get8(i2cdev, PCA6416_INPUT_PORT_0);
    g_assert_cmphex(value & PCA_CONFIG_BYTE, ==, 0x55);
    value = i2c_get8(i2cdev, PCA6416_INPUT_PORT_1);
    g_assert_cmphex(value & PCA_CONFIG_BYTE, ==, 0x55);
    qmp_value = qmp_pca_gpio_get(TEST_ID, "gpio_input");
    g_assert_cmphex(qmp_value & PCA_CONFIG_WORD, ==, 0x5555);

    /*
     * i2c will return the value written to the output register, not the values
     * of the output pins, so we check only the configured pins
     */
    qmp_pca_gpio_set(TEST_ID, "gpio_output", 0xFFFF);
    value = i2c_get8(i2cdev, PCA6416_OUTPUT_PORT_0);
    g_assert_cmphex(value & ~PCA_CONFIG_BYTE, ==, 0xAA);
    value = i2c_get8(i2cdev, PCA6416_OUTPUT_PORT_1);
    g_assert_cmphex(value & ~PCA_CONFIG_BYTE, ==, 0xAA);

    qmp_value = qmp_pca_gpio_get(TEST_ID, "gpio_output");
    g_assert_cmphex(qmp_value & ~PCA_CONFIG_WORD, ==, 0xAAAA);
}

static void test_set_output(void *obj, void *data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    uint8_t value;
    uint16_t qmp_value;
    /* configure pins to be outputs */
    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_0, 0);
    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_1, 0);

    qmp_pca_gpio_set(TEST_ID, "gpio_output", 0x5555);
    value = i2c_get8(i2cdev, PCA6416_OUTPUT_PORT_0);
    g_assert_cmphex(value, ==, 0x55);
    value = i2c_get8(i2cdev, PCA6416_OUTPUT_PORT_1);
    g_assert_cmphex(value, ==, 0x55);

    qmp_value = qmp_pca_gpio_get(TEST_ID, "gpio_output");
    g_assert_cmphex(qmp_value, ==, 0x5555);
}

static void test_tx_rx(void *obj, void *data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    uint8_t value;

    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_0, 0xFF);
    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_1, 0xFF);
    i2c_set8(i2cdev, PCA6416_POLARITY_INVERSION_PORT_0, 0);
    i2c_set8(i2cdev, PCA6416_POLARITY_INVERSION_PORT_1, 0);

    value = i2c_get8(i2cdev, PCA6416_CONFIGURATION_PORT_0);
    g_assert_cmphex(value, ==, 0xFF);

    value = i2c_get8(i2cdev, PCA6416_CONFIGURATION_PORT_1);
    g_assert_cmphex(value, ==, 0xFF);

    value = i2c_get8(i2cdev, PCA6416_POLARITY_INVERSION_PORT_0);
    g_assert_cmphex(value, ==, 0);

    value = i2c_get8(i2cdev, PCA6416_POLARITY_INVERSION_PORT_1);
    g_assert_cmphex(value, ==, 0);


    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_0, 0xAB);
    value = i2c_get8(i2cdev, PCA6416_CONFIGURATION_PORT_0);
    g_assert_cmphex(value, ==, 0xAB);

    i2c_set8(i2cdev, PCA6416_CONFIGURATION_PORT_1, 0xBC);
    value = i2c_get8(i2cdev, PCA6416_CONFIGURATION_PORT_1);
    g_assert_cmphex(value, ==, 0xBC);

}

static void pca_i2c_gpio_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {
        .extra_device_opts = "id=" TEST_ID ",address=0x78"
    };
    add_qi2c_address(&opts, &(QI2CAddress) { 0x78 });
    g_test_set_nonfatal_assertions();

    qos_node_create_driver("pca6416", i2c_device_create);
    qos_node_consumes("pca6416", "i2c-bus", &opts);

    qos_add_test("tx-rx", "pca6416", test_tx_rx, NULL);
    qos_add_test("set_output_gpio", "pca6416", test_set_output, NULL);
    qos_add_test("set_input_gpio", "pca6416", test_set_input, NULL);
    qos_add_test("follow_gpio_config", "pca6416", test_config, NULL);
}
libqos_init(pca_i2c_gpio_register_nodes);

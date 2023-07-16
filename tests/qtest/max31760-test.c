/*
 * MAX31760 Fan Controller with Nonvolatile Lookup Table
 *
 * Features:
 *  - 2 tachometer inputs
 *  - Remote temperature sensing
 * Datasheet:
 * https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31760.pdf
 *
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/sensor/max31760.h"
#include "libqtest-single.h"
#include "libqos/qgraph.h"
#include "libqos/i2c.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qnum.h"

#define TEST_ID "max31760-test"

/* Test read write registers + QMP */
static void test_read_write(void *obj, void *data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    uint8_t value;

    /* R/W Control Register 1 */
    i2c_set8(i2cdev, R_CR1, 0xA0);
    value = i2c_get8(i2cdev, R_CR1);
    g_assert_cmphex(value, ==, 0xA0);

    /* R/W Control Register 2 */
    i2c_set8(i2cdev, R_CR2, 0xA1);
    value = i2c_get8(i2cdev, R_CR2);
    g_assert_cmphex(value, ==, 0xA1);

    /* R/W Control Register 3 */
    i2c_set8(i2cdev, R_CR3, 0xA2);
    value = i2c_get8(i2cdev, R_CR3);
    g_assert_cmphex(value, ==, 0xA2);

    /* R/W Fan Fault Duty Cycle */
    i2c_set8(i2cdev, R_FFDC, 0xA3);
    value = i2c_get8(i2cdev, R_FFDC);
    g_assert_cmphex(value, ==, 0xA3);

    /* R/W Alert Mask Register */
    i2c_set8(i2cdev, R_MASK, 0xA4);
    value = i2c_get8(i2cdev, R_MASK);
    g_assert_cmphex(value, ==, 0xA4);

    /* R/W Ideality Factor Register */
    i2c_set8(i2cdev, R_IFR, 0xA5);
    value = i2c_get8(i2cdev, R_IFR);
    g_assert_cmphex(value, ==, 0xA5);

    /* R/W Remote High Set-point MSB */
    i2c_set8(i2cdev, R_RHSH, 0xA6);
    value = i2c_get8(i2cdev, R_RHSH);
    g_assert_cmphex(value, ==, 0xA6);

    /* R/W Remote High Set-point LSB */
    i2c_set8(i2cdev, R_RHSL, 0xB7);
    value = i2c_get8(i2cdev, R_RHSL);
    g_assert_cmphex(value, ==, 0xB7);

    /* R/W Local Overtemperature Set-point MSB */
    i2c_set8(i2cdev, R_LOTSH, 0xB8);
    value = i2c_get8(i2cdev, R_LOTSH);
    g_assert_cmphex(value, ==, 0xB8);

    /* R/W Local Overtemperature Set-point LSB */
    i2c_set8(i2cdev, R_LOTSL, 0xB9);
    value = i2c_get8(i2cdev, R_LOTSL);
    g_assert_cmphex(value, ==, 0xB9);

    /* R/W Remote Overtemperature Set-point MSB */
    i2c_set8(i2cdev, R_ROTSH, 0xCA);
    value = i2c_get8(i2cdev, R_ROTSH);
    g_assert_cmphex(value, ==, 0xCA);

    /* R/W Remote Overtemperature Set-point LSB */
    i2c_set8(i2cdev, R_ROTSL, 0xCB);
    value = i2c_get8(i2cdev, R_ROTSL);
    g_assert_cmphex(value, ==, 0xCB);

    /* R/W Local High Set-point MSB */
    i2c_set8(i2cdev, R_LHSH, 0xCC);
    value = i2c_get8(i2cdev, R_LHSH);
    g_assert_cmphex(value, ==, 0xCC);

    /* R/W Local High Set-point LSB */
    i2c_set8(i2cdev, R_LHSL, 0xCD);
    value = i2c_get8(i2cdev, R_LHSL);
    g_assert_cmphex(value, ==, 0xCD);

    /* R/W TACH Count Threshold Register, MSB */
    i2c_set8(i2cdev, R_TCTH, 0xCE);
    value = i2c_get8(i2cdev, R_TCTH);
    g_assert_cmphex(value, ==, 0xCE);

    /* R/W TACH Count Threshold Register, LSB */
    i2c_set8(i2cdev, R_TCTL, 0xAF);
    value = i2c_get8(i2cdev, R_TCTL);
    g_assert_cmphex(value, ==, 0xAF);

    /* R/W Direct Duty-Cycle Control Register */
    i2c_set8(i2cdev, R_PWMR, 0xD0);
    value = i2c_get8(i2cdev, R_PWMR);
    g_assert_cmphex(value, ==, 0xD0);

}

static void test_read_only(void *obj, void* data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    uint8_t value;

    /* R Current PWM Duty-Cycle Register */
    i2c_set8(i2cdev, R_PWMV, 0xE1);
    value = i2c_get8(i2cdev, R_PWMV);
    g_assert_cmphex(value, !=, 0xE1);

    /* R TACH1 Count Register, MSB */
    i2c_set8(i2cdev, R_TC1H, 0xE2);
    value = i2c_get8(i2cdev, R_TC1H);
    g_assert_cmphex(value, !=, 0xE2);

    /* R TACH1 Count Register, LSB */
    i2c_set8(i2cdev, R_TC1L, 0xE3);
    value = i2c_get8(i2cdev, R_TC1L);
    g_assert_cmphex(value, !=, 0xE3);

    /* R TACH2 Count Register, MSB */
    i2c_set8(i2cdev, R_TC2H, 0xE4);
    value = i2c_get8(i2cdev, R_TC2H);
    g_assert_cmphex(value, !=, 0xE4);

    /* R TACH2 Count Register, LSB */
    i2c_set8(i2cdev, R_TC2L, 0xE5);
    value = i2c_get8(i2cdev, R_TC2L);
    g_assert_cmphex(value, !=, 0xE5);

    /* R Remote Temperature Reading Register, MSB */
    i2c_set8(i2cdev, R_RTH, 0xE6);
    value = i2c_get8(i2cdev, R_RTH);
    g_assert_cmphex(value, !=, 0xE6);

    /* R Remote Temperature Reading Register, LSB */
    i2c_set8(i2cdev, R_RTL, 0xE7);
    value = i2c_get8(i2cdev, R_RTL);
    g_assert_cmphex(value, !=, 0xE7);

    /* R Local Temperature Reading Register, MSB */
    i2c_set8(i2cdev, R_LTH, 0xE8);
    value = i2c_get8(i2cdev, R_LTH);
    g_assert_cmphex(value, !=, 0xE8);

    /* R Local Temperature Reading Register, LSB */
    i2c_set8(i2cdev, R_LTL, 0xE9);
    value = i2c_get8(i2cdev, R_LTL);
    g_assert_cmphex(value, !=, 0xE9);

    /* R Status Register */
    i2c_set8(i2cdev, R_SR, 0xEA);
    value = i2c_get8(i2cdev, R_SR);
    g_assert_cmphex(value, !=, 0xEA);

}

/* TODO(b/291326612):Test status behaviour*/
/* TODO(b/291326612) Test non-volatile reads and writes */
/* TODO(b/291326612) Test min and max alerts */

static void max31760_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {
        .extra_device_opts = "id=" TEST_ID ",address=0x58"
    };
    add_qi2c_address(&opts, &(QI2CAddress) { 0x58 });

    qos_node_create_driver("max31760", i2c_device_create);
    qos_node_consumes("max31760", "i2c-bus", &opts);

    qos_add_test("test_read_write", "max31760", test_read_write, NULL);
    qos_add_test("test_read_only", "max31760", test_read_only, NULL);
}
libqos_init(max31760_register_nodes);

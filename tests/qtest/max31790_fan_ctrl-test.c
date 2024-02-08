/*
 * QTests for MAX31790 Fan controller
 *
 * Independently control 6 fans, up to 12 tachometer inputs,
 * controlled through i2c
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include <math.h>
#include "hw/sensor/max31790_fan_ctrl.h"
#include "libqtest-single.h"
#include "libqos/qgraph.h"
#include "libqos/i2c.h"
#include "qapi/qmp/qdict.h"
#include "qapi/qmp/qnum.h"
#include "qemu/bitops.h"

#define TEST_ID         "max31790-test"
#define TEST_ADDR       (0x37)
#define TEST_MAX_RPM    0x4000

static uint16_t qmp_max31790_get(const char *id, const char *property)
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

static void qmp_max31790_set(const char *id,
                            const char *property,
                            uint16_t value)
{
    QDict *response;

    response = qmp("{ 'execute': 'qom-set', 'arguments': { 'path': %s, "
                   "'property': %s, 'value': %u } }", id, property, value);
    g_assert(qdict_haskey(response, "return"));
    qobject_unref(response);
}

static uint32_t max31790_tach_count2rpm(uint16_t tach, uint8_t sr)
{
    if (tach) {
        return (sr * MAX31790_CLK_FREQ * 60) / (MAX31790_PULSES_PER_REV * tach);
    } else {
        return 0;
    }
}

/* R/W Tach - 6 fans */
static void test_defaults(void *obj, void *data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    uint8_t i2c_value;

    i2c_value = i2c_get8(i2cdev, MAX31790_REG_GLOBAL_CONFIG);
    g_assert_cmphex(i2c_value, ==, MAX31790_GLOBAL_CONFIG_DEFAULT);

    i2c_value = i2c_get8(i2cdev, MAX31790_REG_PWM_FREQ);
    g_assert_cmphex(i2c_value, ==, MAX31790_PWM_FREQ_DEFAULT);

    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        i2c_value = i2c_get8(i2cdev, MAX31790_REG_FAN_DYNAMICS(i));
        g_assert_cmphex(i2c_value, ==, MAX31790_FAN_DYNAMICS_DEFAULT);
    }

    i2c_value = i2c_get8(i2cdev, MAX31790_REG_FAILED_FAN_OPTS_SEQ_STRT);
    g_assert_cmphex(i2c_value, ==, MAX31790_FAILED_FAN_OPTS_SEQ_STRT_DEFAULT);
}

static void test_pwm(void *obj, void *data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    char *path;
    int err;
    uint16_t i2c_value, value, rpm;


    /* init fans to different pwm duty cycles */
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        path = g_strdup_printf("max_rpm[%d]", i);
        qmp_max31790_set(TEST_ID, path, TEST_MAX_RPM); /* ~16k RPM */
        g_free(path);
        i2c_set8(i2cdev, MAX31790_REG_FAN_CONFIG(i), 0); /* enable PWM mode */
        path = g_strdup_printf("pwm[%d]", i);
        qmp_max31790_set(TEST_ID, path, i * 0x40);
        g_free(path);
    }

    /* read and compare qmp with i2c 9-bit pwm */
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        path = g_strdup_printf("pwm[%d]", i);
        value = qmp_max31790_get(TEST_ID, path);
        g_free(path);
        i2c_value = i2c_get8(i2cdev, MAX31790_REG_PWMOUT_MSB(i)) << 8;
        i2c_value |= i2c_get8(i2cdev, MAX31790_REG_PWMOUT_LSB(i));
        i2c_value >>= MAX31790_PWM_SHAMT;
        g_assert_cmphex(value, ==, i2c_value);
    }

    /* expect tach to match pwm scaled to max_rpm */
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        i2c_value = i2c_get8(i2cdev, MAX31790_REG_TACH_COUNT_MSB(i)) << 8;
        i2c_value |= i2c_get8(i2cdev, MAX31790_REG_TACH_COUNT_LSB(i));
        i2c_value >>= 5;
        value = max31790_tach_count2rpm(i2c_value, MAX31790_SR_DEFAULT);
        rpm = (TEST_MAX_RPM * i * 0x40) / 0x1FF; /* max_rpm x pwm_duty_cycle */
        err = value - rpm;
        g_assert_cmpuint(abs(err), <, 163); /* ~1% of max_rpm */
    }
}

static void test_rpm(void *obj, void *data, QGuestAllocator *alloc)
{
    QI2CDevice *i2cdev = (QI2CDevice *)obj;
    char *path;
    int err;
    uint16_t i2c_value, value, rpm;

    /* init fans to different speeds */
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        i2c_set8(i2cdev, MAX31790_REG_FAN_CONFIG(i),
                 MAX31790_FAN_CFG_RPM_MODE);
        path = g_strdup_printf("target_rpm[%d]", i);
        qmp_max31790_set(TEST_ID, path, i * 1000);
        g_free(path);
    }

    /* read and compare qmp with i2c 11-bit tach */
    for (int i = 0; i < MAX31790_NUM_FANS; i++) {
        path = g_strdup_printf("target_rpm[%d]", i);
        value = qmp_max31790_get(TEST_ID, path);
        g_free(path);

        i2c_value = i2c_get8(i2cdev, MAX31790_REG_TACH_COUNT_MSB(i)) << 8;
        i2c_value |= i2c_get8(i2cdev, MAX31790_REG_TACH_COUNT_LSB(i));
        i2c_value >>= MAX31790_TACH_SHAMT;

        rpm = max31790_tach_count2rpm(i2c_value, MAX31790_SR_DEFAULT);
        err = value - rpm;
        g_assert_cmpint(abs(err), <, 20); /* 20 RPM */
        err = (i * 1000) - rpm;
        g_assert_cmpint(abs(err), <, 20);
    }
}

static void max31790_register_nodes(void)
{
    QOSGraphEdgeOptions opts = {
        .extra_device_opts = "id=" TEST_ID ",address=0x37"
    };
    add_qi2c_address(&opts, &(QI2CAddress) { TEST_ADDR });

    qos_node_create_driver("max31790", i2c_device_create);
    qos_node_consumes("max31790", "i2c-bus", &opts);

    qos_add_test("test_defaults", "max31790", test_defaults, NULL);
    qos_add_test("test_pwm", "max31790", test_pwm, NULL);
    qos_add_test("test_rpm", "max31790", test_rpm, NULL);
}
libqos_init(max31790_register_nodes);

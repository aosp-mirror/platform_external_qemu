/*
 * Max 31790 Fan controller
 *
 * Independently control 6 fans, up to 12 tachometer inputs,
 * controlled through i2c
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MAX31790_FAN_CTRL_H
#define MAX31790_FAN_CTRL_H

#include "hw/i2c/i2c.h"
#include "qom/object.h"

#define MAX31790_NUM_FANS 6
#define MAX31790_NUM_TACHS 12

typedef struct MAX31790State {
    I2CSlave parent;

    /* Registers */
    uint8_t global_config;
    uint8_t pwm_freq;
    uint8_t fan_config[MAX31790_NUM_FANS];
    uint8_t fan_dynamics[MAX31790_NUM_FANS];
    uint8_t fan_fault_status_2;
    uint8_t fan_fault_status_1;
    uint8_t fan_fault_mask_2;
    uint8_t fan_fault_mask_1;
    uint8_t failed_fan_opts_seq_strt;
    uint16_t tach_count[MAX31790_NUM_TACHS];
    uint16_t pwm_duty_cycle[MAX31790_NUM_FANS];
    uint16_t pwmout[MAX31790_NUM_FANS];
    uint16_t target_count[MAX31790_NUM_FANS];
    uint8_t window[MAX31790_NUM_FANS];

    /* config */
    uint16_t max_rpm[MAX31790_NUM_FANS];

    /* i2c transaction state */
    uint8_t command;
    bool i2c_cmd_event;
    bool cmd_is_new;
} MAX31790State;

#define TYPE_MAX31790 "max31790"
#define MAX31790(obj) OBJECT_CHECK(MAX31790State, (obj), TYPE_MAX31790)

#define MAX31790_REG_GLOBAL_CONFIG             0x00                 /* R/W */
#define MAX31790_REG_PWM_FREQ                  0x01                 /* R/W */
#define MAX31790_REG_FAN_CONFIG(ch)           (0x02 + (ch))         /* R/W */
#define MAX31790_REG_FAN_DYNAMICS(ch)         (0x08 + (ch))         /* R/W */
#define MAX31790_REG_FAN_FAULT_STATUS_2        0x10                 /* R/W */
#define MAX31790_REG_FAN_FAULT_STATUS_1        0x11                 /* R/W */
#define MAX31790_REG_FAN_FAULT_MASK_2          0x12                 /* R/W */
#define MAX31790_REG_FAN_FAULT_MASK_1          0x13                 /* R/W */
#define MAX31790_REG_FAILED_FAN_OPTS_SEQ_STRT  0x14                 /* R/W */
#define MAX31790_REG_TACH_COUNT_MSB(ch)       (0x18 + (ch) * 2)     /* R */
#define MAX31790_REG_TACH_COUNT_LSB(ch)       (0x19 + (ch) * 2)     /* R */
#define MAX31790_REG_PWM_DUTY_CYCLE_MSB(ch)   (0x30 + (ch) * 2)     /* R */
#define MAX31790_REG_PWM_DUTY_CYCLE_LSB(ch)   (0x31 + (ch) * 2)     /* R */
#define MAX31790_REG_PWMOUT_MSB(ch)           (0x40 + (ch) * 2)     /* R/W */
#define MAX31790_REG_PWMOUT_LSB(ch)           (0x41 + (ch) * 2)     /* R/W */
#define MAX31790_REG_TARGET_COUNT_MSB(ch)     (0x50 + (ch) * 2)     /* R/W */
#define MAX31790_REG_TARGET_COUNT_LSB(ch)     (0x51 + (ch) * 2)     /* R/W */
#define MAX31790_REG_WINDOW(ch)               (0x60 + (ch))         /* R/W */

#define MAX31790_GLOBAL_CONFIG_DEFAULT                0x20
#define MAX31790_PWM_FREQ_DEFAULT                     0x44 /* 125Hz */
#define MAX31790_FAN_DYNAMICS_DEFAULT                 0x4C
#define MAX31790_FAILED_FAN_OPTS_SEQ_STRT_DEFAULT     0x45
#define MAX31790_PWMOUT_DEFAULT                       (128 << 7) /* 25% */
#define MAX31790_TARGET_COUNT_DEFAULT                 0x3D60

/* Fan Config register bits */
#define MAX31790_FAN_CFG_RPM_MODE             BIT(7)
#define MAX31790_FAN_CFG_MONITOR_ONLY         BIT(4)
#define MAX31790_FAN_CFG_TACH_INPUT_EN        BIT(3)
#define MAX31790_FAN_CFG_TACH_INPUT           BIT(0)

/* Tachometer calculation constants */
#define MAX31790_PULSES_PER_REV             2
#define MAX31790_SR_DEFAULT                 4
#define MAX31790_CLK_FREQ                   8192
#define MAX31790_MAX_RPM_DEFAULT            16500

/* reg alignment amounts */
#define MAX31790_PWM_SHAMT                  7
#define MAX31790_TACH_SHAMT                 5
#endif

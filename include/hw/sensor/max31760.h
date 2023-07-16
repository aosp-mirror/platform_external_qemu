/*
 * MAX31760 Fan Controller with Nonvolatile Lookup Table
 *
 * Features:
 *  - 2 tachometer inputs
 *  - Remote temperature sensing
 *
 * Datasheet:
 * https://www.analog.com/media/en/technical-documentation/data-sheets/MAX31760.pdf
 *
 * Copyright 2023 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef MAX31760_H
#define MAX31760_H

#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "qom/object.h"
#include "qemu/bitops.h"
#include "hw/register.h"

REG8(CR1, 0x00)         /* R/W Control Register 1 */
    FIELD(CR1, ALTMSK, 7, 1)    /* Alert mask */
    FIELD(CR1, POR, 6, 1)       /* Software POR */
    FIELD(CR1, HYST, 5, 1)      /* Lookup Table Hysteresis */
    FIELD(CR1, DRV, 3, 2)       /* PWM Frequency */
    FIELD(CR1, PPS, 2, 1)       /* PWM Polarity */
    FIELD(CR1, MTI, 1, 1)       /* Maximum Temperature as Index */
    FIELD(CR1, TIS, 0, 1)       /* Temperature Index Source */
REG8(CR2, 0x01)         /* R/W Control Register 2 */
    FIELD(CR2, STBY, 7, 1)      /* Standby Mode Enable */
    FIELD(CR2, ALERTS, 6, 1)    /* ALERT Functionality Selection */
    FIELD(CR2, SPEN, 5, 1)      /* Spin-Up Enable */
    FIELD(CR2, FF, 4, 1)        /* FF Functionality Selection */
    FIELD(CR2, FS, 3, 1)        /* FS Input Enable */
    /* Rotation-Detection (RD) Signal-Polarity Selection*/
    FIELD(CR2, RDPS, 2, 1)
    FIELD(CR2, FSST, 1, 1)      /* Fan-Sense Signal Type */
    FIELD(CR2, DFC, 0, 1)       /* Direct Fan Control (Manual Mode) */
REG8(CR3, 0x02)         /* R/W Control Register 3 */
    FIELD(CR3, CLR_FAIL, 7, 1) /* Clear Fan Fail Status Bits  */
    FIELD(CR3, FF_0, 6, 1)     /* Duty-Cycle Fan-Fail Detection */
    FIELD(CR3, RAMP, 4, 2)     /* PWM Duty-Cycle Ramp Rate */
    FIELD(CR3, TACHFULL, 3, 1) /* Detect fan failure only if duty cycle=100%. */
    FIELD(CR3, PSEN, 2, 1)     /* Pulse Stretch Enable */
    FIELD(CR3, TACH2E, 1, 1)   /* TACH2 Enable */
    FIELD(CR3, TACH1E, 0, 1)   /* TACH2 Enable */
REG8(FFDC, 0x03)        /* R/W Fan Fault Duty Cycle */
REG8(MASK, 0x04)        /* R/W Alert Mask Register */
    FIELD(MASK, LHAM, 5, 1)     /* Local Temperature High Alarm Mask */
    FIELD(MASK, LOTAM, 4, 1)    /* Local Overtemperature Alarm Mask */
    FIELD(MASK, RHAM, 3, 1)     /* Remote High-Temperature Alarm Mask */
    FIELD(MASK, ROTAM, 2, 1)    /* Remote Overtemperature Alarm Mask */
    FIELD(MASK, TACH2AM, 1, 1)  /* TACH2 Alarm Mask */
    FIELD(MASK, TACH1AM, 0, 1)  /* TACH2 Alarm Mask */
REG8(IFR, 0x05)         /* R/W Ideality Factor Register */
    FIELD(IFR, IF, 0, 6)
REG8(RHSH, 0x06)        /* R/W Remote High Set-point MSB */
REG8(RHSL, 0x07)        /* R/W Remote High Set-point LSB */
REG8(LOTSH, 0x08)       /* R/W Local Overtemperature Set-point MSB */
REG8(LOTSL, 0x09)       /* R/W Local Overtemperature Set-point LSB */
REG8(ROTSH, 0x0A)       /* R/W Remote Overtemperature Set-point MSB */
REG8(ROTSL, 0x0B)       /* R/W Remote Overtemperature Set-point LSB */
REG8(LHSH, 0x0C)        /* R/W Local High Set-point MSB */
REG8(LHSL, 0x0D)        /* R/W Local High Set-point LSB */
REG8(TCTH, 0x0E)        /* R/W TACH Count Threshold Register, MSB */
REG8(TCTL, 0x0F)        /* R/W TACH Count Threshold Register, LSB */
REG8(USER, 0x10)        /* Read/Write 8 Bytes of General-Purpose User Memory */
REG8(LUT, 0x20)         /* Read/Write 48-Byte Lookup Table (LUT) */
REG8(PWMR, 0x50)        /* R/W Direct Duty-Cycle Control Register */
REG8(PWMV, 0x51)        /* R Current PWM Duty-Cycle Register */
REG8(TC1H, 0x52)        /* R TACH1 Count Register, MSB */
REG8(TC1L, 0x53)        /* R TACH1 Count Register, LSB */
REG8(TC2H, 0x54)        /* R TACH2 Count Register, MSB */
REG8(TC2L, 0x55)        /* R TACH2 Count Register, LSB */
REG8(RTH, 0x56)         /* R Remote Temperature Reading Register, MSB */
REG8(RTL, 0x57)         /* R Remote Temperature Reading Register, LSB */
REG8(LTH, 0x58)         /* R Local Temperature Reading Register, MSB */
REG8(LTL, 0x59)         /* R Local Temperature Reading Register, LSB */
REG8(SR, 0x5A)          /* R Status Register */
    FIELD(SR, RHA, 3, 1)     /* Remote Temperature High Alarm */
    FIELD(SR, ROTA, 2, 1)    /* Remote Overtemperature Alarm */
    FIELD(SR, TACH2A, 1, 1)  /* TACH2 Alarm */
    FIELD(SR, TACH1A, 0, 1)  /* TACH1 Alarm */
REG8(EEX, 0x5B)         /* W Load EEPROM to RAM; Write RAM to EEPROM */

#define MAX31760_CR1_DEFAULT    0x01
#define MAX31760_CR2_DEFAULT    0x10
#define MAX31760_CR3_DEFAULT    0x03
#define MAX31760_FFDC_DEFAULT   0xFF
#define MAX31760_MASK_DEFAULT   0xC0
#define MAX31760_IFR_DEFAULT    0x18
#define MAX31760_RHSH_DEFAULT   0x55
#define MAX31760_RHSL_DEFAULT   0x00
#define MAX31760_LOTSH_DEFAULT  0x55
#define MAX31760_LOTSL_DEFAULT  0x00
#define MAX31760_ROTSH_DEFAULT  0x6E
#define MAX31760_ROTSL_DEFAULT  0x00
#define MAX31760_LHSH_DEFAULT   0x46
#define MAX31760_LHSL_DEFAULT   0x00
#define MAX31760_TCTH_DEFAULT   0xFF
#define MAX31760_TCTL_DEFAULT   0xFE
#define MAX31760_USER_SIZE      8
#define MAX31760_LUT_SIZE       48

#define MAX31760_NUM_REG (R_EEX + 1)

typedef struct MAX31760State {
    I2CSlave parent;

    /* Registers */
    uint8_t reg[MAX31760_NUM_REG];

    /* i2c transaction state */
    uint8_t command;
    bool i2c_cmd_event;

} MAX31760State;

#define TYPE_MAX31760 "max31760"
OBJECT_DECLARE_SIMPLE_TYPE(MAX31760State, MAX31760)

#endif

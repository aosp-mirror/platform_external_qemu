/*
 * MAX16600 VR13.HC Dual-Output Voltage Regulator Chipset
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

#include "hw/i2c/pmbus_device.h"

#define TYPE_MAX16600 "max16600"
#define MAX16600(obj) OBJECT_CHECK(MAX16600State, (obj), TYPE_MAX16600)

#define MAX16600_PHASE_ID       0xF3
/*
 * Packet error checking capability is disabled.
 * Pending QEMU support
 */
#define MAX16600_CAPABILITY_DEFAULT 0x30
#define MAX16600_OPERATION_DEFAULT 0x88
#define MAX16600_ON_OFF_CONFIG_DEFAULT 0x17
#define MAX16600_VOUT_MODE_DEFAULT 0x22
#define MAX16600_PHASE_ID_DEFAULT 0x80

#define MAX16600_READ_VIN_DEFAULT 5    /* Volts */
#define MAX16600_READ_IIN_DEFAULT 3    /* Amps */
#define MAX16600_READ_PIN_DEFAULT 100  /* Watts */
#define MAX16600_READ_VOUT_DEFAULT 5   /* Volts */
#define MAX16600_READ_IOUT_DEFAULT 3   /* Amps */
#define MAX16600_READ_POUT_DEFAULT 100 /* Watts */
#define MAX16600_READ_TEMP_DEFAULT 40  /* Celsius */

typedef struct MAX16600State {
    PMBusDevice parent;
    const char *ic_device_id;
    uint8_t phase_id;
} MAX16600State;

/*
 * determines the exponents used in linear conversion for CORE
 * (iin, pin) may be (-4, 0) or (-3, 1)
 * iout may be -2, -1, 0, 1
 */
static const struct {
    int vin, iin, pin, iout, pout, temp;
} max16600_exp = {-6, -4, 0, -2, -1, 0};

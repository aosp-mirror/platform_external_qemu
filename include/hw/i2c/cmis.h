/*
 * Common Management Interface Specification (CMIS) Module
 *
 * This module describes a CMIS device. The specs can be found at
 * http://www.qsfp-dd.com/wp-content/uploads/2019/05/QSFP-DD-CMIS-rev4p0.pdf
 * Copyright 2022 Google LLC
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
#ifndef CMIS_H
#define CMIS_H

#include "hw/i2c/i2c.h"
#include "qom/object.h"

#define CMIS_PAGE_SIZE 0x80

typedef enum CMISMode {
    CMIS_IDLE,
    CMIS_WRITE_ADDR,
    CMIS_WRITE_DATA,
    CMIS_READ_DATA,
    CMIS_DONE,
} CMISMode;

typedef struct CMISDevice {
    /* CMIS device is on top of I2C protocol. */
    I2CSlave parent;

    CMISMode mode;
    uint8_t current_addr;
    uint8_t bank_addr;
    uint8_t page_addr;
} CMISDevice;

typedef struct CMISClass {
    I2CSlaveClass parent_class;

    uint8_t (*read_lower_page)(CMISDevice *d, uint8_t addr);
    int (*write_lower_page)(CMISDevice *d, uint8_t addr, uint8_t data);
    uint8_t (*read_upper_page)(CMISDevice *d, uint8_t bank_addr,
                               uint8_t page_addr, uint8_t addr);
    int (*write_upper_page)(CMISDevice *d, uint8_t bank_addr, uint8_t page_addr,
                             uint8_t addr, uint8_t data);
} CMICClass;

#define TYPE_CMIS_DEVICE "cmis-device"
OBJECT_DECLARE_TYPE(CMISDevice, CMISClass, CMIS_DEVICE)

#endif /* CMIS_H */

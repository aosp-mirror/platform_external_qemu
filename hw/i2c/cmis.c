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

#include "qemu/osdep.h"

#include "hw/qdev-properties-system.h"
#include "hw/i2c/cmis.h"
#include "qemu/log.h"
#include "qom/object.h"
#include "trace.h"

#define CMIS_BANK_ADDR 0x7e
#define CMIS_PAGE_ADDR 0x7f
#define CMIS_IS_LOWER(d) ((d) < CMIS_PAGE_SIZE)

static uint8_t cmis_device_read_byte(CMISDevice *d)
{
    CMISClass *c = CMIS_DEVICE_GET_CLASS(d);
    uint8_t host_addr = d->current_addr++;

    if (CMIS_IS_LOWER(d->current_addr)) {
        switch (host_addr) {
        case CMIS_BANK_ADDR:
            return d->bank_addr;

        case CMIS_PAGE_ADDR:
            return d->page_addr;

        default:
            return c->read_lower_page(d, host_addr);
        }
    }

    host_addr &= ~BIT(7);
    g_assert(host_addr < CMIS_PAGE_SIZE);
    return c->read_upper_page(d, d->bank_addr, d->page_addr, host_addr);
}

/* return 0 if success, -1 otherwise. */
static int cmis_device_write_byte(CMISDevice *d, uint8_t data)
{
    CMISClass *c = CMIS_DEVICE_GET_CLASS(d);
    uint8_t host_addr = d->current_addr++;

    if (CMIS_IS_LOWER(d->current_addr)) {
        switch (host_addr) {
        case CMIS_BANK_ADDR:
            d->bank_addr = data;
            return 0;

        case CMIS_PAGE_ADDR:
            d->page_addr = data;
            return 0;

        default:
            return c->write_lower_page(d, host_addr, data);
        }
    }

    host_addr &= ~BIT(7);
    g_assert(host_addr < CMIS_PAGE_SIZE);
    return c->write_upper_page(d, d->bank_addr, d->page_addr, host_addr, data);
}

static int cmis_device_event(I2CSlave *s, enum i2c_event event)
{
    CMISDevice *d = CMIS_DEVICE(s);

    switch (event) {
    case I2C_START_SEND:
        d->mode = CMIS_WRITE_ADDR;
        break;

    case I2C_START_RECV:
        d->mode = CMIS_READ_DATA;
        break;

    case I2C_FINISH:
        d->mode = CMIS_IDLE;
        break;

    case I2C_NACK:
        d->mode = CMIS_DONE;
        break;

    case I2C_START_SEND_ASYNC:
        return -1;
    }
    return 0;
}

static uint8_t cmis_device_recv(I2CSlave *s)
{
    CMISDevice *d = CMIS_DEVICE(s);
    uint8_t val;

    val = cmis_device_read_byte(d);
    trace_cmis_device_recv(object_get_canonical_path(OBJECT(s)), val);
    return val;
}

static int cmis_device_send(I2CSlave *s, uint8_t data)
{
    CMISDevice *d = CMIS_DEVICE(s);
    int ret = 0;

    trace_cmis_device_send(object_get_canonical_path(OBJECT(s)), data);
    switch (d->mode) {
    case CMIS_WRITE_ADDR:
        d->current_addr = data;
        d->mode = CMIS_WRITE_DATA;
        break;

    case CMIS_WRITE_DATA:
        ret = cmis_device_write_byte(d, data);
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unexpected data 0x%" PRIx8
            " during mode: %d", object_get_canonical_path(OBJECT(s)),
            data, d->mode);
    }

    return ret;
}

static void cmis_device_class_init(ObjectClass *klass, void *data)
{
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);

    k->event = cmis_device_event;
    k->recv = cmis_device_recv;
    k->send = cmis_device_send;
}

static const TypeInfo cmis_types[] = {
    {
        .name = TYPE_CMIS_DEVICE,
        .parent = TYPE_I2C_SLAVE,
        .instance_size = sizeof(CMISDevice),
        .class_size = sizeof(CMISClass),
        .class_init = cmis_device_class_init,
    },
};
DEFINE_TYPES(cmis_types);

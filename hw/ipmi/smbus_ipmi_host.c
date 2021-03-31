/*
 * IPMI SMBus (SSIF) Host Model
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

#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "hw/ipmi/ipmi.h"
#include "hw/ipmi/smbus_ipmi_host.h"
#include "hw/ipmi/smbus_ipmi.h"
#include "migration/vmstate.h"
#include "qapi/error.h"
#include "qemu/bitops.h"
#include "qemu/log.h"
#include "qemu/timer.h"
#include "qemu/units.h"
#include "trace.h"

#define SSIF_IPMI_TRANSACTION_TIMEOUT_MS    5000
#define SSIF_TIMER_DELAY_MS                 1

/*
 * We can't start a new bus transaction right after we finish because the
 * controller still has a lock on it's state.  To work around, we delay that
 * action.  This is also used if the controller is not ready for a new bus
 * action.
 */
static void smbus_ipmi_host_delay_action(SMBusIPMIHostDevice *s)
{
    timer_mod(&s->delay_timer, qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) +
                               SSIF_TIMER_DELAY_MS);
}

static void smbus_ipmi_host_start_i2c_transfer(SMBusIPMIHostDevice *s)
{
    if (i2c_start_device_transfer(&s->parent, s->sendbuf_len) == 0) {
        smbus_ipmi_host_delay_action(s);
    }
}

/* Receive a request from the host and send it to BMC core. */
static void smbus_ipmi_host_handle_req(IPMIInterface *ii, uint8_t msg_id,
                                       unsigned char *req, unsigned req_len)
{
    IPMIInterfaceClass *iic = IPMI_INTERFACE_GET_CLASS(ii);
    SMBusIPMIHostDevice *s = iic->get_backend_data(ii);

    /* Drop the request if the last request is still not handled. */
    if (s->state != SMBUS_IPMI_HOST_STATUS_IDLE) {
        return;
    }

    if (req_len == 0) {
        return;
    }
    if (req_len > MAX_SSIF_IPMI_MSG_SIZE) {
        /* Truncate the extra bytes that don't fit. */
        req_len = MAX_SSIF_IPMI_MSG_SIZE;
    }

    s->recvbuf_valid = false;
    s->recvbuf_pos = 0;
    s->response_len = 0;
    s->timeout_start = qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL);

    s->netfn = req[0];
    s->cmd = req[1];
    s->last_msg_id = msg_id;

    s->sendbuf_pos = 0;
    if (req_len <= MAX_SSIF_IPMI_MSG_CHUNK) {
        s->state = SMBUS_IPMI_HOST_STATUS_SINGLE_WRITE_CMD;
        s->sendbuf_len = req_len + 2;
        s->sendbuf[0] = SSIF_IPMI_REQUEST;
        s->sendbuf[1] = req_len;
    } else {
        s->state = SMBUS_IPMI_HOST_STATUS_MULTIPART_WRITE_CMD;
        memcpy(s->req, req, req_len);
        s->req_pos = 0;
        s->req_len = req_len;
        s->sendbuf_len = MAX_SSIF_IPMI_MSG_CHUNK + 2;
        s->sendbuf[0] = SSIF_IPMI_MULTI_PART_REQUEST_START;
        s->sendbuf[1] = MAX_SSIF_IPMI_MSG_CHUNK;
    }
    memcpy(s->sendbuf + 2, req, s->sendbuf_len - 2);

    smbus_ipmi_host_start_i2c_transfer(s);
}

static void smbus_ipmi_host_set_ipmi_handler(IPMIInterface *ii, IPMICore *h)
{
    SMBusIPMIHostDevice *s = SMBUS_IPMI_HOST(ii);

    s->host = h;
}

static void *smbus_ipmi_host_backend_data(IPMIInterface *ii)
{
    SMBusIPMIHostDevice *s = SMBUS_IPMI_HOST(ii);

    return s;
}

static void smbus_ipmi_host_set_atn(struct IPMIInterface *s, int val, int irq)
{
}

static void smbus_ipmi_host_enter_reset(Object *obj, ResetType type)
{
    SMBusIPMIHostDevice *s = SMBUS_IPMI_HOST(obj);

    timer_del(&s->delay_timer);
    s->state = SMBUS_IPMI_HOST_STATUS_IDLE;
}

static const VMStateDescription vmstate_SMBUS_IPMI_HOST = {
    .name = TYPE_SMBUS_IPMI_HOST,
    .version_id = 0,
    .minimum_version_id = 0,
    .fields = (VMStateField[]) {
        VMSTATE_TIMER(delay_timer, SMBusIPMIHostDevice),
        VMSTATE_END_OF_LIST(),
    },
};

static int smbus_ipmi_host_recvbuf_len(SMBusIPMIHostDevice *s)
{
    return s->recvbuf[0];
}

static void smbus_ipmi_host_send_response(SMBusIPMIHostDevice *s)
{
    IPMICoreClass *hk;
    hk = IPMI_CORE_GET_CLASS(s->host);
    hk->handle_command(s->host, s->response, s->response_len,
                       MAX_SSIF_IPMI_MSG_SIZE, s->last_msg_id);
    s->state = SMBUS_IPMI_HOST_STATUS_IDLE;
}

static void smbus_ipmi_host_send_response_error(SMBusIPMIHostDevice *s,
                                                uint8_t response_code)
{
    s->response_len = 3;
    s->response[0] = s->netfn | BIT(2);
    s->response[1] = s->cmd;
    s->response[2] = response_code;
    smbus_ipmi_host_send_response(s);
}

static void smbus_ipmi_host_delay_timer(void *opaque)
{
    SMBusIPMIHostDevice *s = (SMBusIPMIHostDevice *)opaque;

    if (qemu_clock_get_ms(QEMU_CLOCK_VIRTUAL) - s->timeout_start >
       SSIF_IPMI_TRANSACTION_TIMEOUT_MS) {
        smbus_ipmi_host_send_response_error(s, IPMI_CC_TIMEOUT);
        return;
    }

    switch (s->state) {
    case SMBUS_IPMI_HOST_STATUS_SINGLE_WRITE_CMD:
    case SMBUS_IPMI_HOST_STATUS_MULTIPART_WRITE_CMD:
    case SMBUS_IPMI_HOST_STATUS_READ_CMD:
    case SMBUS_IPMI_HOST_STATUS_RECEIVING:
        smbus_ipmi_host_start_i2c_transfer(s);
        break;
    case SMBUS_IPMI_HOST_STATUS_IDLE:
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "Unexpected state in smbus_ipmi_host_delay_timer");
        break;
    }
}

static void smbus_ipmi_host_realize(DeviceState *dev, Error **errp)
{
    SMBusIPMIHostDevice *s = SMBUS_IPMI_HOST(dev);

    timer_init_ms(&s->delay_timer, QEMU_CLOCK_VIRTUAL,
        smbus_ipmi_host_delay_timer, s);
}

static bool smbus_ipmi_host_read_start(SMBusIPMIHostDevice *s)
{
    if (s->sendbuf[0] == SSIF_IPMI_RESPONSE && s->recvbuf[1] == 0 &&
        s->recvbuf[2] == 1) {
        return true;
    }

    return false;
}

static void smbus_ipmi_host_start_ssif_read(SMBusIPMIHostDevice *s,
                                            uint8_t ssif_cmd)
{
    s->state = SMBUS_IPMI_HOST_STATUS_READ_CMD;
    s->sendbuf[0] = ssif_cmd;
    s->sendbuf_pos = 0;
    s->sendbuf_len = 1;
    smbus_ipmi_host_delay_action(s);
}

static void smbus_ipmi_host_process_response(SMBusIPMIHostDevice *s)
{
    if (s->response_len + smbus_ipmi_host_recvbuf_len(s) >
        MAX_SSIF_IPMI_MSG_SIZE) {
        qemu_log_mask(LOG_GUEST_ERROR, "SSIF receive message overflow.");
        smbus_ipmi_host_send_response_error(s, IPMI_CC_UNSPECIFIED);
        return;
    }

    if (smbus_ipmi_host_read_start(s)) {
        /* Multipart-part read start */
        s->response_len = smbus_ipmi_host_recvbuf_len(s) - 2;
        memcpy(s->response, &s->recvbuf[3], s->response_len);
        smbus_ipmi_host_start_ssif_read(s,
                                        SSIF_IPMI_MULTI_PART_RESPONSE_MIDDLE);
    } else if (s->sendbuf[0] == SSIF_IPMI_MULTI_PART_RESPONSE_MIDDLE &&
               s->recvbuf[1] != 0xff) {
        /* Multipart-part read middle */
        memcpy(&s->response[s->response_len], &s->recvbuf[2],
               smbus_ipmi_host_recvbuf_len(s) - 1);
        s->response_len += smbus_ipmi_host_recvbuf_len(s) - 1;
        smbus_ipmi_host_start_ssif_read(s,
                                        SSIF_IPMI_MULTI_PART_RESPONSE_MIDDLE);
    } else if (s->sendbuf[0] == SSIF_IPMI_MULTI_PART_RESPONSE_MIDDLE &&
               s->recvbuf[1] == 0xff) {
        /* Multipart-part read end */
        memcpy(&s->response[s->response_len], &s->recvbuf[2],
               smbus_ipmi_host_recvbuf_len(s) - 1);
        s->response_len += smbus_ipmi_host_recvbuf_len(s) - 1;
        smbus_ipmi_host_send_response(s);
    } else if (s->sendbuf[0] == SSIF_IPMI_RESPONSE &&
               !smbus_ipmi_host_read_start(s)) {
        /* Single-part read */
        memcpy(&s->response[0], &s->recvbuf[1], smbus_ipmi_host_recvbuf_len(s));
        s->response_len = smbus_ipmi_host_recvbuf_len(s);
        smbus_ipmi_host_send_response(s);
    } else {
        qemu_log_mask(LOG_GUEST_ERROR, "Unexpected SIIF read.");
        smbus_ipmi_host_send_response_error(s, IPMI_CC_UNSPECIFIED);
    }
}

static int smbus_ipmi_host_finish_event(SMBusIPMIHostDevice *s)
{
    switch (s->state) {
    case SMBUS_IPMI_HOST_STATUS_MULTIPART_WRITE_CMD:
        s->req_pos += MAX_SSIF_IPMI_MSG_CHUNK;
        s->sendbuf_pos = 0;
        if (s->req_len - s->req_pos <= MAX_SSIF_IPMI_MSG_CHUNK) {
            s->state = SMBUS_IPMI_HOST_STATUS_SINGLE_WRITE_CMD;
            s->sendbuf_len = s->req_len - s->req_pos + 2;
            s->sendbuf[0] = SSIF_IPMI_MULTI_PART_REQUEST_END;
            s->sendbuf[1] = s->req_len - s->req_pos;
        } else {
            s->state = SMBUS_IPMI_HOST_STATUS_MULTIPART_WRITE_CMD;
            s->sendbuf_len = MAX_SSIF_IPMI_MSG_CHUNK + 2;
            s->sendbuf[0] = SSIF_IPMI_MULTI_PART_REQUEST_MIDDLE;
            s->sendbuf[1] = MAX_SSIF_IPMI_MSG_CHUNK;
        }

        memcpy(s->sendbuf + 2, s->req + s->req_pos, s->sendbuf_len - 2);
        smbus_ipmi_host_delay_action(s);
        break;
    case SMBUS_IPMI_HOST_STATUS_SINGLE_WRITE_CMD:
        s->state = SMBUS_IPMI_HOST_STATUS_READ_CMD;
        smbus_ipmi_host_start_ssif_read(s, SSIF_IPMI_RESPONSE);
        break;
    case SMBUS_IPMI_HOST_STATUS_READ_CMD:
        s->state = SMBUS_IPMI_HOST_STATUS_RECEIVING;
        s->recvbuf_pos = 0;
        s->sendbuf_len = 0;
        smbus_ipmi_host_delay_action(s);
        break;
    case SMBUS_IPMI_HOST_STATUS_RECEIVING:
        if (s->recvbuf_valid) {
            smbus_ipmi_host_process_response(s);
        } else {
            smbus_ipmi_host_send_response_error(s, IPMI_CC_UNSPECIFIED);
        }
        break;
    case SMBUS_IPMI_HOST_STATUS_IDLE:
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "Unexpected state in smbus_ipmi_host_finish_event");
        break;
    }

    return 0;
}

static int smbus_ipmi_host_event(I2CSlave *is, enum i2c_event event)
{
    SMBusIPMIHostDevice *s = container_of(is, SMBusIPMIHostDevice, parent);

    switch (event) {
    case I2C_START_SEND:
        break;
    case I2C_START_RECV:
        break;
    case I2C_FINISH:
        return smbus_ipmi_host_finish_event(s);
    case I2C_NACK:
        qemu_log_mask(LOG_GUEST_ERROR, "Unexpected I2C_NACK");
        s->state = SMBUS_IPMI_HOST_STATUS_IDLE;
        break;
    case I2C_START_SEND_ASYNC:
        qemu_log_mask(LOG_GUEST_ERROR, "Unexpected I2C_START_SEND_ASYNC");
        s->state = SMBUS_IPMI_HOST_STATUS_IDLE;
        break;
    }

    return 0;
}

static uint8_t smbus_ipmi_host_recv(I2CSlave *is)
{
    SMBusIPMIHostDevice *s = container_of(is, SMBusIPMIHostDevice, parent);

    if (s->sendbuf_len > 0 && s->sendbuf_pos < s->sendbuf_len) {
        uint8_t result = s->sendbuf[s->sendbuf_pos];
        s->sendbuf_pos++;

        return result;
    }

    return 0;
}

/* Returns non-zero to indicate that the transmission is done. */
static int smbus_ipmi_host_send(I2CSlave *is, uint8_t data)
{
    SMBusIPMIHostDevice *s = container_of(is, SMBusIPMIHostDevice, parent);

    if (s->recvbuf_pos >= MAX_SSIF_IPMI_MSG_CHUNK + 1) {
        s->recvbuf_valid = false;
        return 1;
    }

    s->recvbuf[s->recvbuf_pos] = data;
    s->recvbuf_pos++;

    if (s->recvbuf_pos >= 1) {
        if (smbus_ipmi_host_recvbuf_len(s) == 1) {
            s->recvbuf_valid = false;
            return 1;
        }
        if (smbus_ipmi_host_recvbuf_len(s) > MAX_SSIF_IPMI_MSG_CHUNK) {
            s->recvbuf_valid = false;
            return 1;
        }
        if (smbus_ipmi_host_recvbuf_len(s) + 1 <= s->recvbuf_pos) {
            s->recvbuf_valid = true;
            return 1;
        }
    }

    return 0;
}

static void smbus_ipmi_host_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(klass);
    IPMIInterfaceClass *iic = IPMI_INTERFACE_CLASS(klass);

    k->event = &smbus_ipmi_host_event;
    k->recv = &smbus_ipmi_host_recv;
    k->send = &smbus_ipmi_host_send;

    dc->desc = "SSIF Host Controller";
    dc->vmsd = &vmstate_SMBUS_IPMI_HOST;
    dc->realize = smbus_ipmi_host_realize;
    rc->phases.enter = smbus_ipmi_host_enter_reset;

    iic->set_atn = smbus_ipmi_host_set_atn;
    iic->set_ipmi_handler = smbus_ipmi_host_set_ipmi_handler;
    iic->get_backend_data = smbus_ipmi_host_backend_data;
    iic->handle_msg = smbus_ipmi_host_handle_req;
}

static const TypeInfo smbus_ipmi_host_info = {
    .name               = TYPE_SMBUS_IPMI_HOST,
    .parent             = TYPE_I2C_SLAVE,
    .instance_size      = sizeof(SMBusIPMIHostDevice),
    .class_init         = smbus_ipmi_host_class_init,
    .interfaces         = (InterfaceInfo[]) {
        { TYPE_IPMI_INTERFACE },
        { },
    },
};

static void smbus_ipmi_host_register_type(void)
{
    type_register_static(&smbus_ipmi_host_info);
}
type_init(smbus_ipmi_host_register_type);

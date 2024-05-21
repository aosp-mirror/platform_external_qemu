/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 * SPDX-FileCopyrightText: Copyright (c) 2022 Samsung Electronics Co., Ltd.
 * SPDX-FileContributor: Klaus Jensen <k.jensen@samsung.com>
 */

#include "qemu/osdep.h"
#include "qemu/main-loop.h"

#include "hw/qdev-properties.h"
#include "hw/i2c/i2c.h"
#include "hw/i2c/mctp.h"

#include "trace.h"

static uint8_t crc8(uint16_t data)
{
#define POLY (0x1070U << 3)
    int i;

    for (i = 0; i < 8; i++) {
        if (data & 0x8000) {
            data = data ^ POLY;
        }

        data = data << 1;
    }

    return (uint8_t)(data >> 8);
#undef POLY
}

static uint8_t i2c_smbus_pec(uint8_t crc, uint8_t *buf, size_t len)
{
    int i;

    for (i = 0; i < len; i++) {
        crc = crc8((crc ^ buf[i]) << 8);
    }

    return crc;
}

void i2c_mctp_schedule_send(MCTPI2CEndpoint *mctp)
{
    I2CBus *i2c = I2C_BUS(qdev_get_parent_bus(DEVICE(mctp)));

    mctp->tx.state = I2C_MCTP_STATE_TX_START_SEND;

    i2c_bus_master(i2c, mctp->tx.bh);
}

static void i2c_mctp_tx(void *opaque)
{
    DeviceState *dev = DEVICE(opaque);
    I2CBus *i2c = I2C_BUS(qdev_get_parent_bus(dev));
    I2CSlave *slave = I2C_SLAVE(dev);
    MCTPI2CEndpoint *mctp = MCTP_I2C_ENDPOINT(dev);
    MCTPI2CEndpointClass *mc = MCTP_I2C_ENDPOINT_GET_CLASS(mctp);
    MCTPI2CPacket *pkt = (MCTPI2CPacket *)mctp->buffer;
    uint8_t flags = 0;

    switch (mctp->tx.state) {
    case I2C_MCTP_STATE_TX_SEND_BYTE:
        if (mctp->pos < mctp->len) {
            uint8_t byte = mctp->buffer[mctp->pos];

            trace_i2c_mctp_tx_send_byte(mctp->pos, byte);

            /* send next byte */
            i2c_send_async(i2c, byte);

            mctp->pos++;

            break;
        }

        /* packet sent */
        i2c_end_transfer(i2c);

        /* end of any control data */
        mctp->len = 0;

        /* fall through */

    case I2C_MCTP_STATE_TX_START_SEND:
        if (mctp->tx.is_control) {
            /* packet payload is already in buffer */
            flags |= MCTP_H_FLAGS_SOM | MCTP_H_FLAGS_EOM;
        } else {
            /* get message bytes from derived device */
            mctp->len = mc->get_message_bytes(mctp, pkt->mctp.payload,
                                              I2C_MCTP_MAXMTU, &flags);
        }

        if (!mctp->len) {
            trace_i2c_mctp_tx_done();

            /* no more packets needed; release the bus */
            i2c_bus_release(i2c);

            mctp->state = I2C_MCTP_STATE_IDLE;
            mctp->tx.is_control = false;

            break;
        }

        mctp->state = I2C_MCTP_STATE_TX;

        pkt->i2c = (MCTPI2CPacketHeader) {
            .dest = mctp->tx.addr & ~0x1,
            .prot = 0xf,
            .byte_count = 5 + mctp->len,
            .source = slave->address << 1 | 0x1,
        };

        pkt->mctp.hdr = (MCTPPacketHeader) {
            .version = 0x1,
            .eid.dest = mctp->tx.eid,
            .eid.source = mctp->my_eid,
            .flags = flags | (mctp->tx.pktseq++ & 0x3) << 4 | mctp->tx.flags,
        };

        mctp->len += sizeof(MCTPI2CPacket);
        mctp->buffer[mctp->len] = i2c_smbus_pec(0, mctp->buffer, mctp->len);
        mctp->len++;

        trace_i2c_mctp_tx_start_send(mctp->len);

        i2c_start_send_async(i2c, pkt->i2c.dest >> 1);

        /* already "sent" the destination slave address */
        mctp->pos = 1;

        mctp->tx.state = I2C_MCTP_STATE_TX_SEND_BYTE;

        break;
    }
}

#define i2c_mctp_control_data(buf) \
    (i2c_mctp_payload(buf) + offsetof(MCTPControlMessage, data))

static void i2c_mctp_handle_control_set_eid(MCTPI2CEndpoint *mctp, uint8_t eid)
{
    mctp->my_eid = eid;

    uint8_t buf[] = {
        0x0, 0x0, eid, 0x0,
    };

    memcpy(i2c_mctp_control_data(mctp->buffer), buf, sizeof(buf));
    mctp->len += sizeof(buf);
}

static void i2c_mctp_handle_control_get_eid(MCTPI2CEndpoint *mctp)
{
    uint8_t buf[] = {
        0x0, mctp->my_eid, 0x0, 0x0,
    };

    memcpy(i2c_mctp_control_data(mctp->buffer), buf, sizeof(buf));
    mctp->len += sizeof(buf);
}

static void i2c_mctp_handle_control_get_version(MCTPI2CEndpoint *mctp)
{
    uint8_t buf[] = {
        0x0, 0x1, 0x0, 0x1, 0x3, 0x1,
    };

    memcpy(i2c_mctp_control_data(mctp->buffer), buf, sizeof(buf));
    mctp->len += sizeof(buf);
}

enum {
    MCTP_CONTROL_SET_EID                    = 0x01,
    MCTP_CONTROL_GET_EID                    = 0x02,
    MCTP_CONTROL_GET_VERSION                = 0x04,
    MCTP_CONTROL_GET_MESSAGE_TYPE_SUPPORT   = 0x05,
};

static void i2c_mctp_handle_control(MCTPI2CEndpoint *mctp)
{
    MCTPI2CEndpointClass *mc = MCTP_I2C_ENDPOINT_GET_CLASS(mctp);
    MCTPControlMessage *msg = (MCTPControlMessage *)i2c_mctp_payload(mctp->buffer);

    /* clear Rq/D */
    msg->flags &= 0x1f;

    mctp->len = offsetof(MCTPControlMessage, data);

    trace_i2c_mctp_handle_control(msg->command);

    switch (msg->command) {
    case MCTP_CONTROL_SET_EID:
        i2c_mctp_handle_control_set_eid(mctp, msg->data[1]);
        break;

    case MCTP_CONTROL_GET_EID:
        i2c_mctp_handle_control_get_eid(mctp);
        break;

    case MCTP_CONTROL_GET_VERSION:
        i2c_mctp_handle_control_get_version(mctp);
        break;

    case MCTP_CONTROL_GET_MESSAGE_TYPE_SUPPORT:
        mctp->len += mc->get_message_types(mctp, i2c_mctp_control_data(mctp->buffer));
        break;

    default:
        trace_i2c_mctp_unhandled_control(msg->command);

        msg->data[0] = MCTP_CONTROL_ERROR_UNSUPPORTED_CMD;
        mctp->len++;

        break;
    }

    i2c_mctp_schedule_send(mctp);
}

static int i2c_mctp_event_cb(I2CSlave *i2c, enum i2c_event event)
{
    MCTPI2CEndpoint *mctp = MCTP_I2C_ENDPOINT(i2c);
    MCTPI2CEndpointClass *mc = MCTP_I2C_ENDPOINT_GET_CLASS(mctp);
    MCTPI2CPacket *pkt = (MCTPI2CPacket *)mctp->buffer;
    size_t payload_len;
    uint8_t pec;

    switch (event) {
    case I2C_START_SEND:
        if (mctp->state == I2C_MCTP_STATE_IDLE) {
            mctp->state = I2C_MCTP_STATE_RX_STARTED;
        } else if (mctp->state != I2C_MCTP_STATE_RX) {
            return -1;
        }

        /* the i2c core eats the slave address, so put it back in */
        pkt->i2c.dest = i2c->address << 1;
        mctp->len = 1;

        return 0;

    case I2C_FINISH:
        payload_len = mctp->len - (1 + offsetof(MCTPI2CPacket, mctp.payload));

        if (pkt->i2c.byte_count + 3 != mctp->len - 1) {
            trace_i2c_mctp_drop_invalid_length(pkt->i2c.byte_count + 3,
                                               mctp->len - 1);
            goto drop;
        }

        pec = i2c_smbus_pec(0, mctp->buffer, mctp->len - 1);
        if (mctp->buffer[mctp->len - 1] != pec) {
            trace_i2c_mctp_drop_invalid_pec(mctp->buffer[mctp->len - 1], pec);
            goto drop;
        }

        if (!(pkt->mctp.hdr.eid.dest == mctp->my_eid
            || pkt->mctp.hdr.eid.dest == 0)) {
            trace_i2c_mctp_drop_invalid_eid(pkt->mctp.hdr.eid.dest,
                                            mctp->my_eid);
            goto drop;
        }

        if (pkt->mctp.hdr.flags & MCTP_H_FLAGS_SOM) {
            mctp->tx.is_control = false;

            if (mctp->state == I2C_MCTP_STATE_RX) {
                mc->reset_message(mctp);
            }

            mctp->state = I2C_MCTP_STATE_RX;

            mctp->tx.addr = pkt->i2c.source;
            mctp->tx.eid = pkt->mctp.hdr.eid.source;
            mctp->tx.flags = pkt->mctp.hdr.flags & 0x7;
            mctp->tx.pktseq = (pkt->mctp.hdr.flags >> 4) & 0x3;

            if ((pkt->mctp.payload[0] & 0x7f) == MCTP_MESSAGE_TYPE_CONTROL) {
                mctp->tx.is_control = true;

                i2c_mctp_handle_control(mctp);

                return 0;
            }
        } else if (mctp->state == I2C_MCTP_STATE_RX_STARTED) {
            trace_i2c_mctp_drop("expected SOM");
            goto drop;
        } else if (((pkt->mctp.hdr.flags >> 4) & 0x3) != (++mctp->tx.pktseq & 0x3)) {
            trace_i2c_mctp_drop_invalid_pktseq((pkt->mctp.hdr.flags >> 4) & 0x3,
                                               mctp->tx.pktseq & 0x3);
            goto drop;
        }

        mc->put_message_bytes(mctp, i2c_mctp_payload(mctp->buffer), payload_len);

        if (pkt->mctp.hdr.flags & MCTP_H_FLAGS_EOM) {
            mc->handle_message(mctp);
            mctp->state = I2C_MCTP_STATE_WAIT_TX;
        }

        return 0;

    default:
        return -1;
    }

drop:
    mc->reset_message(mctp);

    mctp->state = I2C_MCTP_STATE_IDLE;

    return 0;
}

static int i2c_mctp_send_cb(I2CSlave *i2c, uint8_t data)
{
    MCTPI2CEndpoint *mctp = MCTP_I2C_ENDPOINT(i2c);

    if (mctp->len < I2C_MCTP_MAX_LENGTH) {
        mctp->buffer[mctp->len++] = data;
        return 0;
    }

    return -1;
}

static void i2c_mctp_instance_init(Object *obj)
{
    MCTPI2CEndpoint *mctp = MCTP_I2C_ENDPOINT(obj);

    mctp->tx.bh = qemu_bh_new(i2c_mctp_tx, mctp);
}

static Property mctp_i2c_props[] = {
    DEFINE_PROP_UINT8("eid", MCTPI2CEndpoint, my_eid, 0x9),
    DEFINE_PROP_END_OF_LIST(),
};

static void i2c_mctp_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    I2CSlaveClass *k = I2C_SLAVE_CLASS(oc);

    k->event = i2c_mctp_event_cb;
    k->send = i2c_mctp_send_cb;

    device_class_set_props(dc, mctp_i2c_props);
}

static const TypeInfo i2c_mctp_info = {
    .name = TYPE_MCTP_I2C_ENDPOINT,
    .parent = TYPE_I2C_SLAVE,
    .abstract = true,
    .instance_init = i2c_mctp_instance_init,
    .instance_size = sizeof(MCTPI2CEndpoint),
    .class_init = i2c_mctp_class_init,
    .class_size = sizeof(MCTPI2CEndpointClass),
};

static void register_types(void)
{
    type_register_static(&i2c_mctp_info);
}

type_init(register_types)

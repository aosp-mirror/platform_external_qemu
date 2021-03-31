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

/*
 * This models the host implementation of an SSIF IPMI system.  This connects
 * to the device side which is running on the QEMU host via I2C bus and
 * bridges to IPMI by implementing IPMIInterfaceClass.
 */

#ifndef SMBUS_IPMI_HOST_H
#define SMBUS_IPMI_HOST_H

#include "exec/memory.h"
#include "hw/ipmi/ipmi.h"
#include "hw/ipmi/smbus_ipmi.h"
#include "qemu/timer.h"

typedef struct SMBusIPMIHostDevice SMBusIPMIHostDevice;

typedef enum SMBusIPMIHostStatus {
    SMBUS_IPMI_HOST_STATUS_IDLE,
    SMBUS_IPMI_HOST_STATUS_MULTIPART_WRITE_CMD,
    SMBUS_IPMI_HOST_STATUS_SINGLE_WRITE_CMD,
    SMBUS_IPMI_HOST_STATUS_READ_CMD,
    SMBUS_IPMI_HOST_STATUS_RECEIVING,
} SMBusIPMIHostStatus;


/**
 * struct SMBusIPMIHostDevice - IPMI SMBus (SSIF) Host device state.
 * @parent: i2c bus device.
 * @host: IPMI host interface.
 * @state: Current state of the IPMI request.
 * @delay_timer: Timer to delay and retry i2c bus actions.
 * @timeout_start: The time in ms that the request was received.
 * @req: IPMI request buffer
 * @req_pos: Offset of the IPMI request buffer that is being sent.
 * @req_len: Length of the IPMI request.
 * @cmd: The command of the IPMI request.
 * @netfn: The netfn of the IPMI request.
 * @last_msg_id: The message id of last incoming request from host.
 * @sendbuf: I2C output buffer to the guest.
 * @sendbuf_pos: Current byte position of the I2C output buffer.
 * @sendbuf_len: Length of the I2C output buffer.
 * @recvbuf: I2C input buffer from the guest.
 * @recvbuf_pos: Current byte position of the I2C input buffer.
 * @recvbuf_valid: Whether or not the I2C input buffer is valid.
 */
struct SMBusIPMIHostDevice {
    I2CSlave parent;

    IPMICore                *host;

    /* General state of the ipmi host */
    SMBusIPMIHostStatus     state;
    QEMUTimer               delay_timer;
    int64_t                 timeout_start;

    /* Fields from the request */
    uint8_t                 req[MAX_SSIF_IPMI_MSG_SIZE];
    uint32_t                req_pos;
    uint32_t                req_len;
    uint8_t                 cmd;
    uint8_t                 netfn;
    uint8_t                 last_msg_id;

    /* Output buffer for sending data to the guest over i2c */
    /* +2 for command and length bytes */
    uint8_t                 sendbuf[MAX_SSIF_IPMI_MSG_CHUNK + 2];
    uint32_t                sendbuf_pos;
    uint32_t                sendbuf_len;

    /* Input buffer for receiving data from the guest over i2c */
    /* +1 for length byte */
    uint8_t                 recvbuf[MAX_SSIF_IPMI_MSG_CHUNK + 1];
    uint32_t                recvbuf_pos;
    bool                    recvbuf_valid;

    /* Buffer for storing entire ipmi response */
    uint8_t                 response[MAX_SSIF_IPMI_MSG_SIZE];
    uint32_t                response_len;
};

#define TYPE_SMBUS_IPMI_HOST "smbus-ipmi-host"
#define SMBUS_IPMI_HOST(obj)                                              \
    OBJECT_CHECK(SMBusIPMIHostDevice, (obj), TYPE_SMBUS_IPMI_HOST)

#endif /* SMBUS_IPMI_HOST_H */

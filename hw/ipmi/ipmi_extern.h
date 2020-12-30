/*
 * IPMI external connection
 *
 * Copyright (c) 2015 Corey Minyard, MontaVista Software, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef HW_IPMI_EXTERN_H
#define HW_IPMI_EXTERN_H

#include "qemu/osdep.h"
#include "hw/ipmi/ipmi.h"

#define VM_MSG_CHAR        0xA0 /* Marks end of message */
#define VM_CMD_CHAR        0xA1 /* Marks end of a command */
#define VM_ESCAPE_CHAR     0xAA /* Set bit 4 from the next byte to 0 */

#define VM_PROTOCOL_VERSION        1
#define VM_CMD_VERSION             0xff /* A version number byte follows */
#define VM_CMD_NOATTN              0x00
#define VM_CMD_ATTN                0x01
#define VM_CMD_ATTN_IRQ            0x02
#define VM_CMD_POWEROFF            0x03
#define VM_CMD_RESET               0x04
#define VM_CMD_ENABLE_IRQ          0x05 /* Enable/disable the messaging irq */
#define VM_CMD_DISABLE_IRQ         0x06
#define VM_CMD_SEND_NMI            0x07
#define VM_CMD_CAPABILITIES        0x08
#define   VM_CAPABILITIES_POWER    0x01
#define   VM_CAPABILITIES_RESET    0x02
#define   VM_CAPABILITIES_IRQ      0x04
#define   VM_CAPABILITIES_NMI      0x08
#define   VM_CAPABILITIES_ATTN     0x10
#define   VM_CAPABILITIES_GRACEFUL_SHUTDOWN 0x20
#define VM_CMD_GRACEFUL_SHUTDOWN   0x09

typedef struct IPMIExtern {
    DeviceState parent;

    CharBackend chr;

    IPMICore *core;

    bool bmc_side;
    bool connected;

    unsigned char inbuf[MAX_IPMI_MSG_SIZE + 2];
    unsigned int inpos;
    bool in_escape;
    bool in_too_many;
    bool waiting_rsp;
    bool sending_cmd;

    unsigned char outbuf[(MAX_IPMI_MSG_SIZE + 2) * 2 + 1];
    unsigned int outpos;
    unsigned int outlen;

    struct QEMUTimer *extern_timer;
} IPMIExtern;

#define TYPE_IPMI_EXTERN "ipmi-extern"
#define IPMI_EXTERN(obj) \
    OBJECT_CHECK(IPMIExtern, (obj), TYPE_IPMI_EXTERN)

void ipmi_extern_handle_command(IPMIExtern *ibe,
                                uint8_t *cmd, unsigned int cmd_len,
                                unsigned int max_cmd_len,
                                uint8_t msg_id);

bool continue_send(IPMIExtern *ibe);
int ipmi_extern_post_migrate(IPMIExtern *ibe, int version_id);

#endif /* HW_IPMI_EXTERN_H */

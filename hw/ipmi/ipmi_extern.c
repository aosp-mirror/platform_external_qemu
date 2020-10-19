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

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "qapi/error.h"
#include "qemu/timer.h"
#include "chardev/char-fe.h"
#include "hw/ipmi/ipmi.h"
#include "hw/ipmi/ipmi_extern.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qom/object.h"

static unsigned char
ipmb_checksum(const unsigned char *data, int size, unsigned char start)
{
        unsigned char csum = start;

        for (; size > 0; size--, data++) {
                csum += *data;
        }
        return csum;
}

/* Returns whether check_reset is required for IPMI_BMC_EXTERN. */
bool continue_send(IPMIExtern *ibe)
{
    int ret;
    if (ibe->outlen == 0) {
        return true;
    }
    ret = qemu_chr_fe_write(&ibe->chr, ibe->outbuf + ibe->outpos,
                            ibe->outlen - ibe->outpos);
    if (ret > 0) {
        ibe->outpos += ret;
    }
    if (ibe->outpos < ibe->outlen) {
        /* Not fully transmitted, try again in a 10ms */
        timer_mod_ns(ibe->extern_timer,
                     qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 10000000);
        return false;
    } else {
        /* Sent */
        ibe->outlen = 0;
        ibe->outpos = 0;
        if (!ibe->bmc_side && !ibe->sending_cmd) {
            ibe->waiting_rsp = true;
        } else {
            ibe->sending_cmd = false;
        }

        if (ibe->waiting_rsp) {
            /* Make sure we get a response within 4 seconds. */
            timer_mod_ns(ibe->extern_timer,
                         qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) + 4000000000ULL);
        }
        return true;
    }
}

static void extern_timeout(void *opaque)
{
    IPMIExtern *ibe = opaque;
    IPMIInterface *s = ibe->core->intf;

    if (ibe->connected) {
        /*TODO: only core-side */
        if (ibe->waiting_rsp && (ibe->outlen == 0)) {
            IPMIInterfaceClass *k = IPMI_INTERFACE_GET_CLASS(s);
            /* The message response timed out, return an error. */
            ibe->waiting_rsp = false;
            ibe->inbuf[1] = ibe->outbuf[1] | 0x04;
            ibe->inbuf[2] = ibe->outbuf[2];
            ibe->inbuf[3] = IPMI_CC_TIMEOUT;
            k->handle_msg(s, ibe->outbuf[0], ibe->inbuf + 1, 3);
        } else {
            continue_send(ibe);
        }
    }
}

static void addchar(IPMIExtern *ibe, unsigned char ch)
{
    switch (ch) {
    case VM_MSG_CHAR:
    case VM_CMD_CHAR:
    case VM_ESCAPE_CHAR:
        ibe->outbuf[ibe->outlen] = VM_ESCAPE_CHAR;
        ibe->outlen++;
        ch |= 0x10;
        /* fall through */
    default:
        ibe->outbuf[ibe->outlen] = ch;
        ibe->outlen++;
    }
}

void ipmi_extern_handle_command(IPMIExtern *ibe,
                                uint8_t *cmd, unsigned int cmd_len,
                                unsigned int max_cmd_len,
                                uint8_t msg_id)
{
    IPMIInterface *s = ibe->core->intf;
    uint8_t err = 0, csum;
    unsigned int i;

    if (ibe->outlen) {
        /* We already have a command queued.  Shouldn't ever happen. */
        error_report("IPMI KCS: Got command when not finished with the"
                     " previous command");
        abort();
    }

    /* If it's too short or it was truncated, return an error. */
    if (cmd_len < 2) {
        err = IPMI_CC_REQUEST_DATA_LENGTH_INVALID;
    } else if ((cmd_len > max_cmd_len) || (cmd_len > MAX_IPMI_MSG_SIZE)) {
        err = IPMI_CC_REQUEST_DATA_TRUNCATED;
    } else if (!ibe->connected) {
        err = IPMI_CC_BMC_INIT_IN_PROGRESS;
    }
    if (err) {
        IPMIInterfaceClass *k = IPMI_INTERFACE_GET_CLASS(s);
        unsigned char rsp[3];

        rsp[0] = cmd[0] | 0x04;
        rsp[1] = cmd[1];
        rsp[2] = err;

        if (ibe->bmc_side) {
            /* For BMC Side, send out an error message. */
            addchar(ibe, msg_id);
            for (i = 0; i < 3; ++i) {
                addchar(ibe, rsp[i]);
            }
            csum = ipmb_checksum(&msg_id, 1, 0);
            addchar(ibe, -ipmb_checksum(rsp, 3, csum));
            continue_send(ibe);
        } else {
            /* For Core side, handle an error message. */
            ibe->waiting_rsp = false;
            k->handle_msg(s, msg_id, rsp, 3);
        }
        goto out;
    }

    addchar(ibe, msg_id);
    for (i = 0; i < cmd_len; i++) {
        addchar(ibe, cmd[i]);
    }
    csum = ipmb_checksum(&msg_id, 1, 0);
    addchar(ibe, -ipmb_checksum(cmd, cmd_len, csum));

    ibe->outbuf[ibe->outlen] = VM_MSG_CHAR;
    ibe->outlen++;

    /* Start the transmit */
    continue_send(ibe);

 out:
    return;
}

static void handle_msg(IPMIExtern *ibe)
{
    IPMIInterfaceClass *k = IPMI_INTERFACE_GET_CLASS(ibe->core->intf);

    if (ibe->in_escape) {
        ipmi_debug("msg escape not ended\n");
        return;
    }
    if (ibe->inpos < (ibe->bmc_side ? 4 : 5)) {
        ipmi_debug("msg too short\n");
        return;
    }
    if (ibe->in_too_many) {
        ibe->inbuf[3] = IPMI_CC_REQUEST_DATA_TRUNCATED;
        ibe->inpos = 4;
    } else if (ipmb_checksum(ibe->inbuf, ibe->inpos, 0) != 0) {
        ipmi_debug("msg checksum failure\n");
        return;
    } else {
        ibe->inpos--; /* Remove checkum */
    }

    timer_del(ibe->extern_timer);
    ibe->waiting_rsp = false;
    k->handle_msg(ibe->core->intf, ibe->inbuf[0],
                  ibe->inbuf + 1, ibe->inpos - 1);
}

static int can_receive(void *opaque)
{
    return 1;
}

static void receive(void *opaque, const uint8_t *buf, int size)
{
    IPMIExtern *ibe = opaque;
    IPMICoreClass *ck = IPMI_CORE_GET_CLASS(ibe->core);
    int i;
    unsigned char hw_op;
    unsigned char hw_operand = 0;

    for (i = 0; i < size; i++) {
        unsigned char ch = buf[i];

        switch (ch) {
        case VM_MSG_CHAR:
            handle_msg(ibe);
            ibe->in_too_many = false;
            ibe->inpos = 0;
            break;

        case VM_CMD_CHAR:
            if (ibe->in_too_many) {
                ipmi_debug("cmd in too many\n");
                ibe->in_too_many = false;
                ibe->inpos = 0;
                break;
            }
            if (ibe->in_escape) {
                ipmi_debug("cmd in escape\n");
                ibe->in_too_many = false;
                ibe->inpos = 0;
                ibe->in_escape = false;
                break;
            }
            ibe->in_too_many = false;
            if (ibe->inpos < 1) {
                break;
            }
            hw_op = ibe->inbuf[0];
            if (ibe->inpos > 1) {
                hw_operand = ibe->inbuf[1];
            }
            ibe->inpos = 0;
            goto out_hw_op;
            break;

        case VM_ESCAPE_CHAR:
            ibe->in_escape = true;
            break;

        default:
            if (ibe->in_escape) {
                ch &= ~0x10;
                ibe->in_escape = false;
            }
            if (ibe->in_too_many) {
                break;
            }
            if (ibe->inpos >= sizeof(ibe->inbuf)) {
                ibe->in_too_many = true;
                break;
            }
            ibe->inbuf[ibe->inpos] = ch;
            ibe->inpos++;
            break;
        }
    }
    return;

 out_hw_op:
    ck->handle_hw_op(ibe->core, hw_op, hw_operand);
}

static void chr_event(void *opaque, QEMUChrEvent event)
{
    IPMIExtern *ibe = opaque;
    IPMIInterface *s = ibe->core->intf;
    IPMIInterfaceClass *k = IPMI_INTERFACE_GET_CLASS(s);
    unsigned char v;

    switch (event) {
    case CHR_EVENT_OPENED:
        ibe->connected = true;
        ibe->outpos = 0;
        ibe->outlen = 0;
        addchar(ibe, VM_CMD_VERSION);
        addchar(ibe, VM_PROTOCOL_VERSION);
        ibe->outbuf[ibe->outlen] = VM_CMD_CHAR;
        ibe->outlen++;
        /* Only send capability for core side. */
        if (!ibe->bmc_side) {
            addchar(ibe, VM_CMD_CAPABILITIES);
            v = VM_CAPABILITIES_IRQ | VM_CAPABILITIES_ATTN;
            if (k->do_hw_op(ibe->core->intf, IPMI_POWEROFF_CHASSIS, 1) == 0) {
                v |= VM_CAPABILITIES_POWER;
            }
            if (k->do_hw_op(ibe->core->intf, IPMI_SHUTDOWN_VIA_ACPI_OVERTEMP, 1)
                == 0) {
                v |= VM_CAPABILITIES_GRACEFUL_SHUTDOWN;
            }
            if (k->do_hw_op(ibe->core->intf, IPMI_RESET_CHASSIS, 1) == 0) {
                v |= VM_CAPABILITIES_RESET;
            }
            if (k->do_hw_op(ibe->core->intf, IPMI_SEND_NMI, 1) == 0) {
                v |= VM_CAPABILITIES_NMI;
            }
            addchar(ibe, v);
            ibe->outbuf[ibe->outlen] = VM_CMD_CHAR;
            ibe->outlen++;
        }
        ibe->sending_cmd = false;
        continue_send(ibe);
        break;

    case CHR_EVENT_CLOSED:
        if (!ibe->connected) {
            return;
        }
        ibe->connected = false;
        /*
         * Don't hang the OS trying to handle the ATN bit, other end will
         * resend on a reconnect.
         */
        k->set_atn(s, 0, 0);
        if (ibe->waiting_rsp) {
            ibe->waiting_rsp = false;
            ibe->inbuf[1] = ibe->outbuf[1] | 0x04;
            ibe->inbuf[2] = ibe->outbuf[2];
            ibe->inbuf[3] = IPMI_CC_BMC_INIT_IN_PROGRESS;
            k->handle_msg(s, ibe->outbuf[0], ibe->inbuf + 1, 3);
        }
        break;

    case CHR_EVENT_BREAK:
    case CHR_EVENT_MUX_IN:
    case CHR_EVENT_MUX_OUT:
        /* Ignore */
        break;
    }
}

static void ipmi_extern_realize(DeviceState *dev, Error **errp)
{
    IPMIExtern *ibe = IPMI_EXTERN(dev);

    if (!qemu_chr_fe_backend_connected(&ibe->chr)) {
        error_setg(errp, "IPMI external bmc requires chardev attribute");
        return;
    }

    qemu_chr_fe_set_handlers(&ibe->chr, can_receive, receive,
                             chr_event, NULL, ibe, NULL, true);
}

int ipmi_extern_post_migrate(IPMIExtern *ibe, int version_id)
{
    /*
     * We don't directly restore waiting_rsp, Instead, we return an
     * error on the interface if a response was being waited for.
     */
    if (ibe->waiting_rsp) {
        IPMIInterface *ii = ibe->core->intf;
        IPMIInterfaceClass *iic = IPMI_INTERFACE_GET_CLASS(ii);

        ibe->waiting_rsp = false;
        ibe->inbuf[1] = ibe->outbuf[1] | 0x04;
        ibe->inbuf[2] = ibe->outbuf[2];
        ibe->inbuf[3] = IPMI_CC_BMC_INIT_IN_PROGRESS;
        iic->handle_msg(ii, ibe->outbuf[0], ibe->inbuf + 1, 3);
    }
    return 0;
}

static void ipmi_extern_init(Object *obj)
{
    IPMIExtern *ibe = IPMI_EXTERN(obj);

    ibe->extern_timer = timer_new_ns(QEMU_CLOCK_VIRTUAL, extern_timeout, ibe);
}

static void ipmi_extern_finalize(Object *obj)
{
    IPMIExtern *ibe = IPMI_EXTERN(obj);

    timer_del(ibe->extern_timer);
    timer_free(ibe->extern_timer);
}


static void ipmi_extern_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->hotpluggable = false;
    dc->realize = ipmi_extern_realize;
}

static const TypeInfo ipmi_extern_type = {
    .name          = TYPE_IPMI_EXTERN,
    .parent        = TYPE_DEVICE,
    .instance_size = sizeof(IPMIExtern),
    .instance_init = ipmi_extern_init,
    .instance_finalize = ipmi_extern_finalize,
    .class_init    = ipmi_extern_class_init,
 };

static void ipmi_extern_register_types(void)
{
    type_register_static(&ipmi_extern_type);
}

type_init(ipmi_extern_register_types)

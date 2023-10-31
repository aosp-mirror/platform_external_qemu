/*
 * Silvaco I3C Controller
 *
 * Copyright (C) 2023 Google, LLC
 *
 * This code is licensed under the GPL version 2 or later.  See
 * the COPYING file in the top-level directory.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "hw/registerfields.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"
#include "trace.h"
#include "hw/i3c/i3c.h"
#include "hw/i3c/svc-i3c.h"
#include "hw/irq.h"

REG32(MCONFIG, 0x00)
    FIELD(MCONFIG, I2CBAUD, 28, 3)
    FIELD(MCONFIG, ODHPP,   24, 1)
    FIELD(MCONFIG, ODBAUD,  16, 8)
    FIELD(MCONFIG, PPLOW,   12, 4)
    FIELD(MCONFIG, PPBAUD,  8, 4)
    FIELD(MCONFIG, ODSTOP,  6, 1)
    FIELD(MCONFIG, DISTO,   3, 1)
    FIELD(MCONFIG, MSTENA,  0, 2)
REG32(MCTRL, 0x84)
    FIELD(MCTRL, RDTERM,  16, 8)
    FIELD(MCTRL, ADDR,    9, 7)
    FIELD(MCTRL, DIR,     8, 1)
    FIELD(MCTRL, IBIRESP, 6, 2)
    FIELD(MCTRL, TYPE,    4, 2)
    FIELD(MCTRL, REQUEST, 0, 3)
REG32(MSTATUS, 0x88)
    FIELD(MSTATUS, IBIADDR,   24, 7)
    FIELD(MSTATUS, NOWMASTER, 19, 1)
    FIELD(MSTATUS, ERRWARN,   15, 1)
    FIELD(MSTATUS, IBIWON,    13, 1)
    FIELD(MSTATUS, TXNOTFULL, 12, 1)
    FIELD(MSTATUS, RXPEND,    11, 1)
    FIELD(MSTATUS, COMPLETE,  10, 1)
    FIELD(MSTATUS, MCTRLDONE, 9, 1)
    FIELD(MSTATUS, SLVSTART,  8, 1)
    FIELD(MSTATUS, IBITYPE,   6, 2)
    FIELD(MSTATUS, NACKED,    5, 1)
    FIELD(MSTATUS, BETWEEN,   4, 1)
    FIELD(MSTATUS, STATE,     0, 3)
REG32(IBIRULES, 0x8c)
    FIELD(IBIRULES, NOBYTE, 31, 1)
    FIELD(IBIRULES, MSB0,   30, 1)
    FIELD(IBIRULES, ADDR4,  24, 6)
    FIELD(IBIRULES, ADDR3,  18, 6)
    FIELD(IBIRULES, ADDR2,  12, 6)
    FIELD(IBIRULES, ADDR1,  6, 6)
    FIELD(IBIRULES, ADDR0,  0, 6)
REG32(MINTSET, 0x90)
    FIELD(MINTSET, NOWMASTER, 19, 1)
    FIELD(MINTSET, ERRWARN,   15, 1)
    FIELD(MINTSET, IBIWON,    13, 1)
    FIELD(MINTSET, TXNOTFULL, 12, 1)
    FIELD(MINTSET, RXPEND,    11, 1)
    FIELD(MINTSET, COMPLETE,  10, 1)
    FIELD(MINTSET, MCTRLDONE, 9, 1)
    FIELD(MINTSET, SLVSTART,  8, 1)
REG32(MINTCLR, 0x94)
    FIELD(MINTCLR, NOWMASTER, 19, 1)
    FIELD(MINTCLR, ERRWARN,   15, 1)
    FIELD(MINTCLR, IBIWON,    13, 1)
    FIELD(MINTCLR, TXNOTFULL, 12, 1)
    FIELD(MINTCLR, RXPEND,    11, 1)
    FIELD(MINTCLR, COMPLETE,  10, 1)
    FIELD(MINTCLR, MCTRLDONE, 9, 1)
    FIELD(MINTCLR, SLVSTART,  8, 1)
REG32(MINTMASKED, 0x98)
    FIELD(MINTMASKED, NOWMASTER, 19, 1)
    FIELD(MINTMASKED, ERRWARN,   15, 1)
    FIELD(MINTMASKED, IBIWON,    13, 1)
    FIELD(MINTMASKED, TXNOTFULL, 12, 1)
    FIELD(MINTMASKED, RXPEND,    11, 1)
    FIELD(MINTMASKED, COMPLETE,  10, 1)
    FIELD(MINTMASKED, MCTRLDONE, 9, 1)
    FIELD(MINTMASKED, SLVSTART,  8, 1)
REG32(MERRWARN, 0x9c)
    FIELD(MERRWARN, TIMEOUT, 20, 1)
    FIELD(MERRWARN, INVREQ,  19, 1)
    FIELD(MERRWARN, MSGERR,  18, 1)
    FIELD(MERRWARN, OWRITE,  17, 1)
    FIELD(MERRWARN, OREAD,   16, 1)
    FIELD(MERRWARN, TERM,    4, 1)
    FIELD(MERRWARN, WRABT,   3, 1)
    FIELD(MERRWARN, NACK,    2, 1)
REG32(MDMACTRL, 0xa0)
    FIELD(MDMACTRL, DMAWIDTH, 4, 2)
    FIELD(MDMACTRL, DMATB,    2, 2)
    FIELD(MDMACTRL, DMAFB,    0, 2)
REG32(MDATACTRL, 0xac)
    FIELD(MDATACTRL, RXEMPTY, 31, 1)
    FIELD(MDATACTRL, TXFULL,  30, 1)
    FIELD(MDATACTRL, RXCOUNT, 24, 5)
    FIELD(MDATACTRL, TXCOUNT, 16, 5)
    FIELD(MDATACTRL, RXTRIG,  6, 2)
    FIELD(MDATACTRL, TXTRIG,  4, 2)
    FIELD(MDATACTRL, UNLOCK,  3, 1)
    FIELD(MDATACTRL, FLUSHFB, 1, 1)
    FIELD(MDATACTRL, FLUSHTB, 0, 1)
REG32(MWDATAB, 0xb0)
    FIELD(MWDATAB, END_A, 16, 1)
    FIELD(MWDATAB, END_B, 8, 1)
    FIELD(MWDATAB, DATA,  0, 8)
REG32(MWDATABE, 0xb4)
    FIELD(MWDATABE, DATA, 0, 8)
REG32(MWDATAH, 0xb8)
    FIELD(MWDATAH, END,   16, 1)
    FIELD(MWDATAH, DATA1, 8, 8)
    FIELD(MWDATAH, DATA0, 0, 8)
REG32(MWDATAHE, 0xbc)
    FIELD(MWDATAHE, DATA1, 8, 8)
    FIELD(MWDATAHE, DATA0, 0, 8)
REG32(MRDATAB, 0xc0)
    FIELD(MRDATAB, DATA, 0, 8)
REG32(MRDATAH, 0xc8)
    FIELD(MRDATAH, DATA1, 8, 8)
    FIELD(MRDATAH, DATA0, 0, 8)
REG32(MWMSG_SDR, 0xd0)
    FIELD(MWMSG_SDR, LEN,  11, 5)
    FIELD(MWMSG_SDR, I2C,  10, 1)
    FIELD(MWMSG_SDR, END,  8, 1)
    FIELD(MWMSG_SDR, ADDR, 1, 7)
    FIELD(MWMSG_SDR, DIR,  0, 1)
    FIELD(MWMSG_SDR, DATA, 0, 16)
REG32(MRMSG_SDR, 0xd4)
    FIELD(MRMSG_SDR, DATA, 0, 16)

static const uint32_t svc_i3c_resets[SVC_I3C_NR_REGS] = {
    [R_MCONFIG] =   0x00000030,
    [R_MSTATUS] =   0x00001000,
    [R_MDMACTRL] =  0x00000010,
    [R_MDATACTRL] = 0x80000030,
};

static const uint32_t svc_i3c_ro[SVC_I3C_NR_REGS] = {
    [R_MSTATUS]    = 0x0e0000b4,
    [R_MCTRL]      = 0xff000080,
    [R_MSTATUS]    = 0xfff7d8ff,
    [R_MINTSET]    = 0xfff740ff,
    [R_MINTCLR]    = 0xfff740ff,
    [R_MINTMASKED] = 0xffffffff,
    [R_MERRWARN]   = 0xff70f9e3,
    [R_MDMACTRL]   = 0xffffffc0,
    [R_MDATACTRL]  = 0xffffff00,
    [R_MWDATAB]    = 0xfffefe00,
    [R_MWDATABE]   = 0xffffff00,
    [R_MWDATAH]    = 0xfffe0000,
    [R_MWDATAHE]   = 0xffff0000,
    [R_MRDATAB]    = 0xffffff00,
    [R_MRDATAH]    = 0xffff0000,
    [R_MWMSG_SDR]  = 0xffff0000,
    [R_MRMSG_SDR]  = 0xffff0000,
};

static void svc_i3c_update_irq(SVCI3C *s)
{
    s->regs[R_MINTMASKED] = s->regs[R_MSTATUS] & s->regs[R_MINTSET];
    bool level = !!(s->regs[R_MINTMASKED]);

    qemu_set_irq(s->irq, level);
}

static uint64_t svc_i3c_read(void *opaque, hwaddr offset, unsigned size)
{
    SVCI3C *s = SVC_I3C(opaque);
    uint32_t addr = offset >> 2;

    return s->regs[addr];
}

static int svc_i3c_ibi_handle(I3CBus *bus, uint8_t addr, bool is_recv)
{
    return 0;
}

static int svc_i3c_ibi_recv(I3CBus *bus, uint8_t data)
{
    return 0;
}

static int svc_i3c_ibi_finish(I3CBus *bus)
{
    return 0;
}

static void svc_i3c_enter_reset(Object *obj, ResetType type)
{
    SVCI3C *s = SVC_I3C(obj);

    for (size_t i = 0; i < ARRAY_SIZE(s->regs); i++) {
        s->regs[i] = svc_i3c_resets[i];
    }

    trace_svc_i3c_reset(object_get_canonical_path(obj));
}

static void svc_i3c_mintclr_w(SVCI3C *s, uint32_t val)
{
    /* Clear the corresponding bits in MINTSET/ */
    s->regs[R_MINTSET] &= ~val;
    svc_i3c_update_irq(s);
}

static void svc_i3c_mintset_w(SVCI3C *s, uint32_t val)
{
    s->regs[R_MINTSET] |= val;
    svc_i3c_update_irq(s);
}

static void svc_i3c_mstatus_w(SVCI3C *s, uint32_t val)
{
    /* MSTATUS is W1C. */
    s->regs[R_MSTATUS] &= ~val;
    svc_i3c_update_irq(s);
}

static void svc_i3c_write(void *opaque, hwaddr offset, uint64_t value,
                         unsigned size)
{
    SVCI3C *s = SVC_I3C(opaque);
    uint32_t addr = offset >> 2;
    uint32_t val32 = (uint32_t)value;

    val32 &= ~svc_i3c_ro[addr];
    trace_svc_i3c_write(object_get_canonical_path(OBJECT(s)), offset, val32);
    switch (addr) {
    case R_MSTATUS:
        svc_i3c_mstatus_w(s, val32);
        break;
    case R_MINTSET:
        svc_i3c_mintset_w(s, val32);
        break;
    case R_MINTCLR:
        svc_i3c_mintclr_w(s, val32);
        break;
    default:
        s->regs[addr] = val32;
        break;
    }
}

static const MemoryRegionOps svc_i3c_ops = {
    .read = svc_i3c_read,
    .write = svc_i3c_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
};

static void svc_i3c_realize(DeviceState *dev, Error **errp)
{
    SVCI3C *s = SVC_I3C(dev);
    g_autofree char *name = g_strdup_printf(TYPE_SVC_I3C ".%d",
                                            s->cfg.id);

    sysbus_init_irq(SYS_BUS_DEVICE(dev), &s->irq);

    memory_region_init_io(&s->mr, OBJECT(s), &svc_i3c_ops,
                          s, name, SVC_I3C_NR_REGS << 2);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mr);

    s->bus = i3c_init_bus(DEVICE(s), name);
    I3CBusClass *bc = I3C_BUS_GET_CLASS(s->bus);
    bc->ibi_handle = svc_i3c_ibi_handle;
    bc->ibi_recv = svc_i3c_ibi_recv;
    bc->ibi_finish = svc_i3c_ibi_finish;
}

static Property svc_i3c_properties[] = {
    DEFINE_PROP_UINT8("device-id", SVCI3C, cfg.id, 0),
    DEFINE_PROP_END_OF_LIST(),
};

static void svc_i3c_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    dc->desc = "Silvaco I3C Controller";
    dc->realize = svc_i3c_realize;
    device_class_set_props(dc, svc_i3c_properties);

    rc->phases.enter = svc_i3c_enter_reset;
}

static const TypeInfo svc_i3c_info = {
    .name = TYPE_SVC_I3C,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(SVCI3C),
    .class_init = svc_i3c_class_init,
};

static void svc_i3c_register_types(void)
{
    type_register_static(&svc_i3c_info);
}

type_init(svc_i3c_register_types);

/*
 * Nuvoton NPCM7xx PECI Module
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/peci/npcm7xx_peci.h"
#include "qemu/bitops.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "trace.h"

#define PECI_CTL_STS            0
#define     PECI_CTL_STS_DONE_EN      BIT(6)
#define     PECI_CTL_STS_ABRT_ERR     BIT(4)
#define     PECI_CTL_STS_CRC_ERR      BIT(3)
#define     PECI_CTL_STS_DONE         BIT(1)
#define     PECI_CTL_STS_START_BUSY   BIT(0)
#define PECI_RD_LENGTH          0x4
#define PECI_ADDR               0x8
#define PECI_CMD                0xC
#define PECI_CTL2               0x10
#define PECI_WR_LENGTH          0x1C
#define PECI_PDDR               0x2C
#define PECI_DAT_INOUT(reg)    (0x100 + (reg) * 4)

static uint64_t npcm7xx_peci_read(void *opaque, hwaddr offset, unsigned size)
{
    NPCM7xxPECIState *ps = NPCM7XX_PECI(opaque);
    uint8_t ret = 0;

    if (!ps->bus->num_clients) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: no peci clients added to board\n",
                      __func__);
        return 0;
    }

    qemu_irq_lower(ps->irq);

    switch (offset) {
    case PECI_CTL_STS:
        ret = ps->status;
        ps->status &= ~PECI_CTL_STS_CRC_ERR;
        break;

    case PECI_RD_LENGTH:
        ret = ps->pcmd.rd_length;
        break;

    case PECI_ADDR:
        ret = ps->pcmd.addr;
        break;

    case PECI_CMD:
        ret = ps->pcmd.cmd;
        break;

    case PECI_CTL2:
        ret = ps->ctl2;
        break;

    case PECI_WR_LENGTH:
        ret = ps->pcmd.wr_length;
        break;

    case PECI_PDDR:
        qemu_log_mask(LOG_UNIMP, "%s: PECI PDDR is unimplemented.\n", __func__);
        ret = ps->pddr;  /* undoc register */
        break;

    case PECI_DAT_INOUT(0) ... PECI_DAT_INOUT(63):
        ret = ps->pcmd.tx[(offset - PECI_DAT_INOUT(0)) / 4];
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown register 0x%lx\n",
                      __func__, offset);
        ret = 0xff;
        break;
    }
    trace_npcm7xx_peci_read(offset, ret);
    return ret;
}

static void npcm7xx_peci_write(void *opaque, hwaddr offset, uint64_t input,
                               unsigned size)
{
    NPCM7xxPECIState *ps = NPCM7XX_PECI(opaque);
    uint8_t data = input & 0xff;

    trace_npcm7xx_peci_write(offset, input);

    /* ignore writes if the bus has not been populated */
    if (!ps->bus->num_clients) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: no peci clients added to board\n",
                      __func__);
        return;
    }

    switch (offset) {
    case PECI_CTL_STS:
        ps->status = data;
        /* STS_START busy is set by the bmc when the request is written */
        if (data & PECI_CTL_STS_START_BUSY) {
            if (peci_handle_cmd(ps->bus, &(ps->pcmd))) {
                ps->status |= PECI_CTL_STS_CRC_ERR;
            }
            ps->status |= PECI_CTL_STS_DONE;
            ps->status &= ~PECI_CTL_STS_START_BUSY;
            qemu_irq_raise(ps->irq);
        }
        break;

    case PECI_RD_LENGTH:
        ps->pcmd.rd_length = data;
        break;

    case PECI_ADDR:
        ps->pcmd.addr = data;
        break;

    case PECI_CMD:
        ps->pcmd.cmd = data;
        break;

    case PECI_CTL2:
        ps->ctl2 = data;
        break;

    case PECI_WR_LENGTH:
        ps->pcmd.wr_length = data;
        break;

    case PECI_PDDR:
        ps->pddr = data;
        break;

    case PECI_DAT_INOUT(0) ... PECI_DAT_INOUT(63):
        ps->pcmd.rx[(offset - PECI_DAT_INOUT(0)) / 4] = data;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: to unknown register 0x%lx : 0x%lx\n",
                      __func__, offset, input);
        return;
    }

}

static void npcm7xx_peci_reset(Object *obj, ResetType type)
{
    NPCM7xxPECIState *ps = NPCM7XX_PECI(obj);

    ps->status = PECI_CTL_STS_DONE_EN;
}

static const MemoryRegionOps npcm7xx_peci_ops = {
    .read = npcm7xx_peci_read,
    .write = npcm7xx_peci_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .min_access_size = 1,
        .unaligned = false,
    },
};

static void npcm7xx_peci_realize(DeviceState *dev, Error **errp)
{
    NPCM7xxPECIState *ps = NPCM7XX_PECI(dev);
    SysBusDevice *sbd = &ps->parent;

    memory_region_init_io(&ps->iomem, OBJECT(ps), &npcm7xx_peci_ops, ps,
                          TYPE_NPCM7XX_PECI, 4 * KiB);

    sysbus_init_mmio(sbd, &ps->iomem);
    sysbus_init_irq(sbd, &ps->irq);

    ps->bus = peci_bus_create(DEVICE(ps));
}

static void npcm7xx_peci_class_init(ObjectClass *klass, void *data)
{
    ResettableClass *rc = RESETTABLE_CLASS(klass);
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->desc = "NPCM7xx PECI Module";
    dc->realize = npcm7xx_peci_realize;
    rc->phases.enter = npcm7xx_peci_reset;
}

static const TypeInfo npcm7xx_peci_info = {
    .name = TYPE_NPCM7XX_PECI,
    .parent = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(NPCM7xxPECIState),
    .class_init = npcm7xx_peci_class_init,
};

static void npcm7xx_peci_register_types(void)
{
    type_register_static(&npcm7xx_peci_info);
}

type_init(npcm7xx_peci_register_types)

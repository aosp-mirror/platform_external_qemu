/*
 * Nuvoton PCIe Root complex
 *
 * Copyright 2022 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "hw/pci-host/npcm_pcierc.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "qom/object.h"
#include "trace.h"

static uint64_t npcm_pcierc_cfg_read(void *opaque, hwaddr addr, unsigned size)
{
    NPCMPCIERCState *s = NPCM_PCIERC(opaque);
    uint32_t ret = -1;

    switch (addr) {
    case NPCM_PCIERC_RCCFGNUM:
        ret = s->rccfgnum;
        break;

    case NPCM_PCIERC_INTEN:
        ret = s->rcinten;
        break;

    case NPCM_PCIERC_INTST:
        ret = s->rcintstat;
        break;

    case NPCM_PCIERC_IMSI_ADDR:
        ret = s->rcimsiaddr;
        break;

    case NPCM_PCIERC_MSISTAT:
        ret = s->rcmsisstat;
        break;

    case NPCM_PCIERC_AXI_ERROR_REPORT:
        ret = s->axierr;
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: read from unimplemented register 0x%04lx\n",
                      __func__, addr);
        ret = -1;
        break;
    }
    trace_npcm_pcierc_read(addr, size, ret);
    return ret;
}

static void npcm_pcierc_cfg_write(void *opaque, hwaddr addr, uint64_t data,
                                  unsigned size)
{
    NPCMPCIERCState *s = NPCM_PCIERC(opaque);

    trace_npcm_pcierc_write(addr, size, data);
    switch (addr) {
    case NPCM_PCIERC_RCCFGNUM:
        s->rccfgnum = data;
        break;

    case NPCM_PCIERC_INTEN:
        s->rcinten = data;
        break;

    case NPCM_PCIERC_INTST:
        s->rcintstat &= ~data;
        break;

    case NPCM_PCIERC_IMSI_ADDR:
        s->rcimsiaddr = data;
        break;

    case NPCM_PCIERC_MSISTAT:
        s->rcmsisstat &= ~data;
        break;

    case NPCM_PCIERC_AXI_ERROR_REPORT:
        s->axierr = data;
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: write to unimplemented reg 0x%04lx data: 0x%lx\n",
                      __func__, addr, data);
        break;
    }
}

static void npcm_pcierc_reset(Object *obj, ResetType type)
{
    NPCMPCIERCState *s = NPCM_PCIERC(obj);

    s->rccfgnum = 0;
    s->rcinten = 0;
    s->rcintstat = 0;
    s->rcimsiaddr = 0;
    s->rcmsisstat = 0;
    s->axierr = 0;
}

static const char *npcm_pcierc_root_bus_path(PCIHostState *host_bridge,
                                             PCIBus *rootbus)
{
    return "0000:00";
}

static const MemoryRegionOps npcm_pcierc_cfg_ops = {
    .read       = npcm_pcierc_cfg_read,
    .write      = npcm_pcierc_cfg_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
        .unaligned = false,
    },
};

static void npcm_pcierc_realize(DeviceState *dev, Error **errp)
{
    NPCMPCIERCState *s = NPCM_PCIERC(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);

    memory_region_init_io(&s->mmio, OBJECT(s), &npcm_pcierc_cfg_ops,
                          s, TYPE_NPCM_PCIERC, 4 * KiB);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);
}

static void npcm_pcierc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIHostBridgeClass *hbc = PCI_HOST_BRIDGE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    hbc->root_bus_path = npcm_pcierc_root_bus_path;
    dc->realize = npcm_pcierc_realize;
    rc->phases.enter = npcm_pcierc_reset;
    set_bit(DEVICE_CATEGORY_BRIDGE, dc->categories);
    dc->fw_name = "pci";
}

static const TypeInfo npcm_pcierc_type_info = {
    .name = TYPE_NPCM_PCIERC,
    .parent = TYPE_PCIE_HOST_BRIDGE,
    .instance_size = sizeof(NPCMPCIERCState),
    .class_init = npcm_pcierc_class_init,
};

static void npcm_pcierc_register_types(void)
{
    type_register_static(&npcm_pcierc_type_info);
}

type_init(npcm_pcierc_register_types)

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

/* Map enabled windows to a memory subregion */
static void npcm_pcierc_map_enabled(NPCMPCIERCState *s, NPCMPCIEWindow *w)
{
    MemoryRegion *system = get_system_memory();
    uint32_t size = NPCM_PCIERC_SAL_SIZE(w->sal);
    hwaddr bar = ((uint64_t)w->sah) << 32 | (w->sal & 0xFFFFF000);
    char name[26];

    /* check if window is enabled */
    if (!(w->sal & NPCM_PCIERC_SAL_EN)) {
        return;
    }

    if (size > 2 * GiB || size < 4 * KiB) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Invalid PCI window size %d bytes\n",
                      __func__, size);
        return;
    }

    if (w->type == AXI2PCIE) {
        snprintf(name, sizeof(name), "npcm-axi2pcie-window-%d", w->id);
    } else if (w->type == PCIE2AXI) {
        snprintf(name, sizeof(name), "npcm-pcie2axi-window-%d", w->id);
    } else {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: unable to map uninitialized PCIe window",
                      __func__);
        return;
    }

    /* TODO: set subregion to target translation address */
    /* add subregion starting at the window source address */
    if (!memory_region_is_mapped(&w->mem)) {
        memory_region_init(&w->mem, OBJECT(s), name, size);
        memory_region_add_subregion(system, bar, &w->mem);
    }
}

/* unmap windows marked as disabled */
static void npcm_pcierc_unmap_disabled(NPCMPCIEWindow *w)
{
    MemoryRegion *system = get_system_memory();
    /* Bit 0 in the Source address enables the window */
    if (memory_region_is_mapped(&w->mem) && !(w->sal & NPCM_PCIERC_SAL_EN)) {
        memory_region_del_subregion(system, &w->mem);
    }
}

static void npcm_pcie_update_window_maps(NPCMPCIERCState *s)
{
    for (int i = 0; i < NPCM_PCIERC_NUM_PA_WINDOWS; i++) {
        npcm_pcierc_unmap_disabled(&s->pcie2axi[i]);
    }

    for (int i = 0; i < NPCM_PCIERC_NUM_AP_WINDOWS; i++) {
        npcm_pcierc_unmap_disabled(&s->axi2pcie[i]);
    }

    for (int i = 0; i < NPCM_PCIERC_NUM_AP_WINDOWS; i++) {
        npcm_pcierc_map_enabled(s, &s->axi2pcie[i]);
    }

    for (int i = 0; i < NPCM_PCIERC_NUM_PA_WINDOWS; i++) {
        npcm_pcierc_map_enabled(s, &s->pcie2axi[i]);
    }
}

static NPCMPCIEWindow *npcm_pcierc_get_window(NPCMPCIERCState *s, hwaddr addr)
{
    NPCMPCIEWindow *window;

    switch (addr) {
    case NPCM_PCIERC_PAnSAL(0) ... NPCM_PCIERC_PAnTP(1):
        window = &s->pcie2axi[NPCM_PCIERC_PA_WINDOW(addr)];
        break;

    case NPCM_PCIERC_APnSAL(0) ... NPCM_PCIERC_APnTP(4):
        window = &s->axi2pcie[NPCM_PCIERC_AP_WINDOW(addr)];
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid window address 0x%lx\n",
                      __func__, addr);
        return 0;
    }
    return window;
}

static int npcm_pcierc_get_window_offset(NPCMPCIEWindow *w, hwaddr addr)
{
    if (w->type == AXI2PCIE) {
        return addr & NPCM_PCIERC_AP_OFFSET_MASK;
    } else if (w->type == PCIE2AXI) {
        return addr & NPCM_PCIERC_PA_OFFSET_MASK;
    } else {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: unable to access uninitialized PCIe window",
                      __func__);
        return -1;
    }
}

static uint32_t npcm_pcierc_read_window(NPCMPCIERCState *s, hwaddr addr)
{
    NPCMPCIEWindow *window = npcm_pcierc_get_window(s, addr);
    int offset;

    if (!window) {
        return 0;
    }

    offset = npcm_pcierc_get_window_offset(window, addr);
    if (offset < 0) {
        return 0;
    }

    switch (offset) {
    case NPCM_PCIERC_SAL_OFFSET:
        return window->sal;

    case NPCM_PCIERC_SAH_OFFSET:
        return window->sah;

    case NPCM_PCIERC_TAL_OFFSET:
        return window->tal;

    case NPCM_PCIERC_TAH_OFFSET:
        return window->tah;

    case NPCM_PCIERC_PARAM_OFFSET:
        return window->params;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid window offset 0x%x\n",
                      __func__, offset);
        return 0;
    }
}

static void npcm_pcierc_write_window(NPCMPCIERCState *s, hwaddr addr,
                                     uint64_t data)
{
    NPCMPCIEWindow *window = npcm_pcierc_get_window(s, addr);
    int offset;

    if (!window) {
        return;
    }

    offset = npcm_pcierc_get_window_offset(window, addr);
    if (offset < 0) {
        return;
    }

    switch (offset) {
    case NPCM_PCIERC_SAL_OFFSET:
        window->sal = data;
        break;

    case NPCM_PCIERC_SAH_OFFSET:
        window->sah = data;
        break;

    case NPCM_PCIERC_TAL_OFFSET:
        window->tal = data;
        break;

    case NPCM_PCIERC_TAH_OFFSET:
        window->tah = data;
        break;

    case NPCM_PCIERC_PARAM_OFFSET:
        window->params = data;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: invalid window offset 0x%x\n",
                      __func__, offset);
    }

    npcm_pcie_update_window_maps(s);
}

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

    case NPCM_PCIERC_PAnSAL(0) ... NPCM_PCIERC_APnTP(4):
        ret = npcm_pcierc_read_window(s, addr);
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

    case NPCM_PCIERC_PAnSAL(0) ... NPCM_PCIERC_APnTP(4):
        npcm_pcierc_write_window(s, addr, data);
        break;

    default:
        qemu_log_mask(LOG_UNIMP,
                      "%s: write to unimplemented reg 0x%04lx data: 0x%lx\n",
                      __func__, addr, data);
        break;
    }
}

static void npcm_pcierc_reset_pcie_windows(NPCMPCIERCState *s)
{
    memset(s->axi2pcie, 0, sizeof(s->axi2pcie));
    memset(s->pcie2axi, 0, sizeof(s->pcie2axi));

    for (int i = 0; i < NPCM_PCIERC_NUM_PA_WINDOWS; i++) {
        s->pcie2axi[i].id = i;
        s->pcie2axi[i].type = PCIE2AXI;
    }

    for (int i = 0; i < NPCM_PCIERC_NUM_AP_WINDOWS; i++) {
        s->axi2pcie[i].id = i;
        s->axi2pcie[i].type = AXI2PCIE;
    }
}

static void npcm_pcierc_reset(Object *obj)
{
    NPCMPCIERCState *s = NPCM_PCIERC(obj);

    s->rccfgnum = 0;
    s->rcinten = 0;
    s->rcintstat = 0;
    s->rcimsiaddr = 0;
    s->rcmsisstat = 0;
    s->axierr = 0;

    npcm_pcierc_reset_pcie_windows(s);
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

static void npcm_pcierc_instance_init(Object *obj)
{
    NPCMPCIERCState *s = NPCM_PCIERC(obj);

    npcm_pcierc_reset_pcie_windows(s);
}

static void npcm_pcierc_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIHostBridgeClass *hbc = PCI_HOST_BRIDGE_CLASS(klass);
    ResettableClass *rc = RESETTABLE_CLASS(klass);

    hbc->root_bus_path = npcm_pcierc_root_bus_path;
    dc->realize = npcm_pcierc_realize;
    rc->phases.exit = npcm_pcierc_reset;
    set_bit(DEVICE_CATEGORY_BRIDGE, dc->categories);
    dc->fw_name = "pci";
}

static const TypeInfo npcm_pcierc_type_info = {
    .name = TYPE_NPCM_PCIERC,
    .parent = TYPE_PCIE_HOST_BRIDGE,
    .instance_size = sizeof(NPCMPCIERCState),
    .instance_init = npcm_pcierc_instance_init,
    .class_init = npcm_pcierc_class_init,
};

static void npcm_pcierc_register_types(void)
{
    type_register_static(&npcm_pcierc_type_info);
}

type_init(npcm_pcierc_register_types)

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

/* read root complex configuration registers */
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

/* write root complex configuration registers */
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

/* read PCIe configuration space */
static uint64_t npcm_pcie_host_config_read(void *opaque, hwaddr addr,
                                           unsigned size)
{
    NPCMPCIERCState *s = NPCM_PCIERC(opaque);
    PCIHostState *pcih = PCI_HOST_BRIDGE(opaque);
    int bus = NPCM_PCIE_RCCFGNUM_BUS(s->rccfgnum);
    uint8_t devfn = NPCM_PCIE_RCCFGNUM_DEVFN(s->rccfgnum);
    PCIDevice *pcid = pci_find_device(pcih->bus, bus, devfn);

    if (pcid) {
        return pci_host_config_read_common(pcid, addr,
                                           pci_config_size(pcid),
                                           size);
    }
    return 0;
}

/* write PCIe configuration space */
static void npcm_pcie_host_config_write(void *opaque, hwaddr addr,
                                        uint64_t data, unsigned size)
{
    NPCMPCIERCState *s = NPCM_PCIERC(opaque);
    PCIHostState *pcih = PCI_HOST_BRIDGE(opaque);
    int bus = NPCM_PCIE_RCCFGNUM_BUS(s->rccfgnum);
    uint8_t devfn = NPCM_PCIE_RCCFGNUM_DEVFN(s->rccfgnum);
    PCIDevice *pcid = pci_find_device(pcih->bus, bus, devfn);

    if (pcid) {
        pci_host_config_write_common(pcid, addr,
                                     pci_config_size(pcid),
                                     data,
                                     size);
    }
}

static AddressSpace *npcm_pcierc_set_iommu(PCIBus *bus, void *opaque, int devfn)
{
    NPCMPCIERCState *s = NPCM_PCIERC(opaque);

    return &s->pcie_space;
}

static const PCIIOMMUOps npcm_pcierc_iommu_ops = {
    .get_address_space = npcm_pcierc_set_iommu,
};

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

static const MemoryRegionOps npcm_pcie_cfg_space_ops = {
    .read       = npcm_pcie_host_config_read,
    .write      = npcm_pcie_host_config_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
        .unaligned = false,
    },
    .valid = {
        .min_access_size = 1,
        .max_access_size = 8,
    }
};

static void npcm_pcie_set_irq(void *opaque, int irq_num, int level)
{
    NPCMPCIERCState *s = NPCM_PCIERC(opaque);

    qemu_set_irq(s->irq, level);
}

static void npcm_pcierc_realize(DeviceState *dev, Error **errp)
{
    NPCMPCIERCState *s = NPCM_PCIERC(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    PCIHostState *pci = PCI_HOST_BRIDGE(dev);
    PCIDevice *root = pci_new(PCI_DEVFN(0, 0), TYPE_NPCM_PCIE_ROOT_PORT);

    memory_region_init_io(&s->mmio, OBJECT(s), &npcm_pcierc_cfg_ops,
                          s, TYPE_NPCM_PCIERC, 4 * KiB);
    sysbus_init_mmio(sbd, &s->mmio);
    sysbus_init_irq(sbd, &s->irq);

    /* IO memory region is needed to create a PCI bus, but is unused on ARM */
    memory_region_init(&s->pcie_io, OBJECT(s), "npcm-pcie-io", 16);

    /*
     * pcie_root is a 128 MiB memory region in the BMC physical address space
     * in which all PCIe windows must have their programmable source or
     * destination address
     */
    memory_region_init_io(&s->pcie_root, OBJECT(s), &npcm_pcie_cfg_space_ops,
                          s, "npcm-pcie-config", 128 * MiB);
    sysbus_init_mmio(sbd, &s->pcie_root);

    pci->bus = pci_register_root_bus(dev, "pcie",
                                     npcm_pcie_set_irq,
                                     pci_swizzle_map_irq_fn,
                                     s, &s->pcie_root, &s->pcie_io,
                                     0, 4, TYPE_PCIE_BUS);

    address_space_init(&s->pcie_space, &s->pcie_root, "pcie-address-space");
    pci_realize_and_unref(root, pci->bus, &error_fatal);
    pci_setup_iommu(pci->bus, &npcm_pcierc_iommu_ops, s);
}

static void npcm_pcie_root_port_realize(DeviceState *dev, Error **errp)
{
    PCIERootPortClass *rpc = PCIE_ROOT_PORT_GET_CLASS(dev);
    Error *local_err = NULL;

    rpc->parent_realize(dev, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }
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

static void npcm_pcie_rp_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pk = PCI_DEVICE_CLASS(klass);
    PCIERootPortClass *rpc = PCIE_ROOT_PORT_CLASS(klass);

    dc->desc = "Nuvoton PCIe Root Port";
    dc->user_creatable = false;

    device_class_set_parent_realize(dc,
                                    npcm_pcie_root_port_realize,
                                    &rpc->parent_realize);

    /* TODO(b/229132071) replace with real values */
    pk->vendor_id = PCI_VENDOR_ID_QEMU;
    pk->device_id = 0;
    pk->class_id = PCI_CLASS_BRIDGE_PCI;

    rpc->exp_offset = NPCM_PCIE_HEADER_OFFSET; /* Express capabilities offset */
    rpc->aer_offset = NPCM_PCIE_AER_OFFSET;
}

static const TypeInfo npcm_pcierc_type_info = {
    .name = TYPE_NPCM_PCIERC,
    .parent = TYPE_PCIE_HOST_BRIDGE,
    .instance_size = sizeof(NPCMPCIERCState),
    .instance_init = npcm_pcierc_instance_init,
    .class_init = npcm_pcierc_class_init,
};

static const TypeInfo npcm_pcie_port_type_info = {
    .name = TYPE_NPCM_PCIE_ROOT_PORT,
    .parent = TYPE_PCIE_ROOT_PORT,
    .instance_size = sizeof(NPCMPCIERootPort),
    .class_init = npcm_pcie_rp_class_init,
};

static void npcm_pcierc_register_types(void)
{
    type_register_static(&npcm_pcierc_type_info);
    type_register_static(&npcm_pcie_port_type_info);
}

type_init(npcm_pcierc_register_types)

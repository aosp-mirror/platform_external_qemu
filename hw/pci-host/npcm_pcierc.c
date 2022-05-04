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
#include "hw/pci/msi.h"
#include "qapi/error.h"
#include "qemu/log.h"
#include "qemu/units.h"
#include "qom/object.h"
#include "trace.h"


#define NPCM_SAL            BIT(0)
#define NPCM_SAH            BIT(1)
#define NPCM_TAL            BIT(2)
#define NPCM_TAH            BIT(3)
#define NPCM_PARAMS         BIT(4)
#define NPCM_BITFIELDS_ALL  0x1f


static bool npcm_pcierc_valid_window_addr(hwaddr addr, uint32_t size)
{
    if ((addr + size) > NPCM_PCIE_HOLE_END) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: window mapping @0x%lx, size: %d is invalid.\n",
                      __func__, addr, size);
        return false;
    } else if (addr < NPCM_PCIE_HOLE) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: window mapping @0x%lx, is invalid.\n",
                      __func__, addr);
        return false;
    } else {
        return true;
    }
};

static bool npcm_pcierc_valid_window_size(hwaddr src, hwaddr dst, uint32_t size)
{
    if (size > 2 * GiB || size < 4 * KiB) {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: Invalid PCI window size %d bytes\n",
                      __func__, size);
        return false;
    }

    return true;
}

/* Map enabled windows to a memory subregion */
static void npcm_pcierc_map_enabled(NPCMPCIERCState *s, NPCMPCIEWindow *w)
{
    MemoryRegion *system = get_system_memory();
    uint32_t size = NPCM_PCIERC_SAL_SIZE(w->sal);
    hwaddr src_ba = ((uint64_t)w->sah) << 32 | (w->sal & 0xFFFFF000);
    hwaddr dest_ba = ((uint64_t)w->tah) << 32 | w->tal;
    char name[26];

    if (!(w->sal & NPCM_PCIERC_SAL_EN) || /* ignore disabled windows */
        !npcm_pcierc_valid_window_size(src_ba, dest_ba, size) ||
        memory_region_is_mapped(&w->mem) /* ignore existing windows */) {
        return;
    }

    /* bitfield for all 5 registers required to create a PCIe window */
    if (w->set_fields != NPCM_BITFIELDS_ALL) {
        return;
    }
    w->set_fields = 0;

    /*
     * This implementation of the Nuvoton root complex uses memory region
     * aliasing to emulate the behaviour of the windowing system on hardware.
     * AXI to PCIe windows in QEMU are system_memory subregions aliased to PCI
     * memory at the respective source and translation addresses
     * PCIe to AXI windows are done as PCI memory subregions aliased to system
     * memory. PCIe to AXI windows have no address restrictions.
     */
    if (w->type == AXI2PCIE) {
        if (!npcm_pcierc_valid_window_addr(src_ba, size)) {
            return;
        };
        snprintf(name, sizeof(name), "npcm-axi2pcie-window-%d", w->id);
        if (w->params &
            (NPCM_PCIERC_TRSF_PARAM_CONFIG | NPCM_PCIERC_TRSL_ID_CONFIG)) {
            memory_region_init_alias(&w->mem, OBJECT(s), name,
                                     &s->rp_config, 0, size);
        } else {
            memory_region_init_alias(&w->mem, OBJECT(s), name,
                                     &s->pcie_memory, dest_ba, size);
        }
        memory_region_add_subregion(system, src_ba, &w->mem);
    } else if (w->type == PCIE2AXI) {
        snprintf(name, sizeof(name), "npcm-pcie2axi-window-%d", w->id);
        memory_region_init_alias(&w->mem, OBJECT(s), name,
                                 system, src_ba, size);
        memory_region_add_subregion(&s->pcie_memory, dest_ba, &w->mem);
    } else {
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: unable to map uninitialized PCIe window",
                      __func__);
        return;
    }
}

/* unmap windows marked as disabled */
static void npcm_pcierc_unmap_disabled(NPCMPCIERCState *s, NPCMPCIEWindow *w)
{
    MemoryRegion *system = get_system_memory();

    /* Bit 0 in the Source address enables the window */
    if (memory_region_is_mapped(&w->mem) && !(w->sal & NPCM_PCIERC_SAL_EN)) {
        if (w->type == AXI2PCIE) {
            memory_region_del_subregion(system, &w->mem);
        } else {
            memory_region_del_subregion(&s->pcie_memory, &w->mem);
        }
    }
}

static void npcm_pcie_update_window_maps(NPCMPCIERCState *s)
{
    for (int i = 0; i < NPCM_PCIERC_NUM_PA_WINDOWS; i++) {
        npcm_pcierc_unmap_disabled(s, &s->pcie2axi[i]);
    }

    for (int i = 0; i < NPCM_PCIERC_NUM_AP_WINDOWS; i++) {
        npcm_pcierc_unmap_disabled(s, &s->axi2pcie[i]);
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
        window->set_fields |= NPCM_SAL;
        break;

    case NPCM_PCIERC_SAH_OFFSET:
        window->sah = data;
        window->set_fields |= NPCM_SAH;
        break;

    case NPCM_PCIERC_TAL_OFFSET:
        window->tal = data;
        window->set_fields |= NPCM_TAL;
        break;

    case NPCM_PCIERC_TAH_OFFSET:
        window->tah = data;
        window->set_fields |= NPCM_TAH;
        break;

    case NPCM_PCIERC_PARAM_OFFSET:
        window->params = data;
        window->set_fields |= NPCM_PARAMS;
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
        return pci_host_config_read_common(pcid, (addr & 0x7FF),
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
        pci_host_config_write_common(pcid, (addr & 0x7FF),
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
    PCIHostState *phs = PCI_HOST_BRIDGE(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    PCIDevice *root = pci_new(PCI_DEVFN(0, 0), TYPE_NPCM_PCIE_ROOT_PORT);

    /* init the underlying memory region for all PCI address space */
    memory_region_init(&s->pcie_memory, OBJECT(s), "npcm-pcie-mem", UINT64_MAX);

    /* I/O memory region is needed to create a PCI bus, but is unused on ARM */
    memory_region_init(&s->pcie_io, OBJECT(s), "npcm-pcie-io", 16);

    phs->bus = pci_register_root_bus(dev, "pcie",
                                     npcm_pcie_set_irq,
                                     pci_swizzle_map_irq_fn,
                                     s, &s->pcie_memory, &s->pcie_io,
                                     0, 4, TYPE_PCIE_BUS);

    address_space_init(&s->pcie_space, &s->pcie_memory, "pcie-address-space");
    pci_setup_iommu(phs->bus, &npcm_pcierc_iommu_ops, s);
    /* init region for root complex registers (not config space) */
    memory_region_init_io(&s->rc_regs, OBJECT(s), &npcm_pcierc_cfg_ops,
                          s, TYPE_NPCM_PCIERC, 4 * KiB);
    sysbus_init_mmio(sbd, &s->rc_regs);
    sysbus_init_irq(sbd, &s->irq);

    /* create and add region for the root port in config space */
    memory_region_init_io(&s->rp_config, OBJECT(s),
                          &npcm_pcie_cfg_space_ops, s, "npcm-pcie-config",
                          4 * KiB);

    /* realize the root port */
    pci_realize_and_unref(root, phs->bus, &error_fatal);
    /* enable MSI (non-X) in root port config space */
    msi_nonbroken = true;
    msi_init(root, NPCM_PCIERC_MSI_OFFSET, NPCM_PCIERC_MSI_NR,
             true, true, errp);

    npcm_pcierc_reset_pcie_windows(s);
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

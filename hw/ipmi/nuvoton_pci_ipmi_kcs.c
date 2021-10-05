/*
 * Nuvoton Nuvoton PCI IPMI KCS implementation
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/bitops.h"
#include "qapi/error.h"
#include "qom/object.h"
#include "migration/vmstate.h"
#include "chardev/char-fe.h"
#include "hw/pci/pci.h"
#include "hw/pci/pci_device.h"
#include "hw/isa/isa.h"
#include "hw/sysbus.h"
#include "hw/ipmi/ipmi_kcs.h"
#include "hw/irq.h"
#include "hw/qdev-properties-system.h"
#include "hw/registerfields.h"
#include "hw/pci/pci_regs.h"

/* By default the VID for the device belongs to Winbond, not Nuvoton */
#define PCI_VENDOR_ID_WINBOND 0x1050
#define PCI_DEVICE_ID_WINBOND_BPCI 0x750

#define TYPE_NUVOTON_PCI_IPMI "nuvoton-pci-ipmi"
OBJECT_DECLARE_SIMPLE_TYPE(NuvotonPCIIPMIState, NUVOTON_PCI_IPMI)

#define TYPE_NUVOTON_IPMI_KCS "nuvoton-ipmi-kcs"
OBJECT_DECLARE_SIMPLE_TYPE(NuvotonIPMIKCSState, NUVOTON_IPMI_KCS)

#define NUVOTON_PCI_MBX_BAR_SIZE  0x4000
#define NUVOTON_PCI_MBX_REG_START 0x4000
#define NUVOTON_PCI_MBX_MMIO_SIZE 0x8000

/* Additional PCI configuration registers not part of PCI spec */
REG32(NUVOTON_BPCI_VID_DID_WRITE, 0x44);
    FIELD(NUVOTON_BPCI_VID_DID_WRITE, WR_DEV_ID, 16, 16)
    FIELD(NUVOTON_BPCI_VID_DID_WRITE, WR_VENDOR_ID, 0, 16)
REG32(NUVOTON_BPCI_CLASS_CODE_WRITE, 0x48);
    FIELD(NUVOTON_BPCI_CLASS_CODE_WRITE, WR_CC_31_8, 8, 24)
    FIELD(NUVOTON_BPCI_CLASS_CODE_WRITE, WR_CC_7_0, 0, 8)
REG32(NUVOTON_BPCI_SUBSYSTEM_ID_WRITE, 0x4c);
    FIELD(NUVOTON_BPCI_SUBSYSTEM_ID_WRITE, SID, 16, 16)
    FIELD(NUVOTON_BPCI_SUBSYSTEM_ID_WRITE, SVID, 0, 16)
REG32(NUVOTON_BPCI_PID_REVISION, 0x60);
REG32(NUVOTON_BPCI_PMID, 0x78);
    FIELD(NUVOTON_BPCI_PMID, D21, 25, 2)
    FIELD(NUVOTON_BPCI_PMID, DSI, 21, 1)
    FIELD(NUVOTON_BPCI_PMID, VERSION, 16, 3)
    FIELD(NUVOTON_BPCI_PMID, PM_NEXT_PTR, 8, 8)
    FIELD(NUVOTON_BPCI_PMID, PM_CAP_ID, 0, 8)
REG32(NUVOTON_BPCI_PMCR, 0x7c);
    FIELD(NUVOTON_BPCI_PMCR, POWERSTATE, 0, 2)

/* Host mailbox registers */
REG32(NUVOTON_HMBXSTAT, 0x4000);
    FIELD(NUVOTON_HMBXSTAT, CORI, 31, 1)
    FIELD(NUVOTON_HMBXSTAT, HIF, 0, 8)
REG32(NUVOTON_HMBXCTL, 0x4004);
    FIELD(NUVOTON_HMBXCTL, HIDECFG, 31, 1)
    FIELD(NUVOTON_HMBXCTL, CFGLCK, 30, 1)
    FIELD(NUVOTON_HMBXCTL, HIE, 0, 8)
REG32(NUVOTON_HMBXCMD, 0x4008);
    FIELD(NUVOTON_HMBXCMD, CIF, 0, 8)

typedef enum {
    NUVOTON_VID_RST           = 0x1050,
    NUVOTON_DID_RST           = 0x750,
    NUVOTON_CMD_RST           = 0x400,
    NUVOTON_STATUS_RST        = 0x490,
    NUVOTON_CLASS_REV_RST     = 0xff000000,
    NUVOTON_CACHE_LINE_SZ_RST = 0x00,
    NUVOTON_LATENCY_TIMER_RST = 0x00,
    NUVOTON_HDR_TYPE_RST      = 0x00,
    NUVOTON_BAR0_RST          = 0x00,
    NUVOTON_SUBSYSTEM_VID_RST = 0x7501050,
    NUVOTON_CAPABILITIES_RST  = 0x78,
    NUVOTON_IRQ_LINE_RST      = 0x00,
    NUVOTON_IRQ_PIN_RST       = 0x01,
    NUVOTON_MIN_GRANT_RST     = 0x10,
    NUVOTON_MAX_LATENCY_RST   = 0x20,
    NUVOTON_PID_REVISION_RST  = 0xa92750,
    NUVOTON_PMID_RST          = 0x210001,
    NUVOTON_PMCR_RST          = 0x00,
} NuvotonPCIIPMICfgResets;

typedef enum {
    NUVOTON_HMBXSTAT_RST = 0x00,
    NUVOTON_HMBXCTL_RST  = 0X00,
    NUVOTON_HMBXCMD_RST  = 0x00,
} NuvotonPCIIPMIMbxResets;

typedef enum {
    NUVOTON_PCI_IPMI_STATE_IDLE,
    NUVOTON_PCI_IPMI_STATE_OFFSET,
    NUVOTON_PCI_IPMI_STATE_SIZE,
    NUVOTON_PCI_IPMI_STATE_DATA,
} NuvotonPCIIPMIHostState;

typedef enum {
    NUVOTON_PCI_IPMI_CODE_OK           = 0x00,
    NUVOTON_PCI_IPMI_CODE_OP_INVALID   = 0xa0,
    NUVOTON_PCI_IPMI_CODE_SIZE_INVALID = 0xa1,
} NuvotonPCIIPMICode;

typedef enum {
    NUVOTON_PCI_IPMI_OP_READ    = 0x01,
    NUVOTON_PCI_IPMI_OP_WRITE   = 0x02,
} NuvotonPCIIPMIOp;

typedef struct NuvotonIPMIKCSState NuvotonIPMIKCSState;
typedef struct NuvotonPCIIPMIState NuvotonPCIIPMIState;

struct NuvotonPCIIPMIState {
    PCIDevice pcidev;
    NuvotonIPMIKCSState *ipmi_kcs_state;

    struct {
        uint8_t  ram[NUVOTON_PCI_MBX_BAR_SIZE];
        uint32_t hmbxstat;
        uint32_t hmbxctl;
        uint32_t hmbxcmd;

        qemu_irq irq_host[8];
        qemu_irq wire_bmc_cif[8];
        MemoryRegion iomem;

        /* Variables for TX/RXing BMC data */
        CharBackend chr;
        NuvotonPCIIPMIHostState state;
        uint8_t op;
        hwaddr offset;
        size_t size;
        uint64_t data;
        size_t rx_cnt;

        hwaddr tx_addr;
        size_t tx_size;
        uint64_t tx_data;
    } pci_mbx;
};

struct NuvotonIPMIKCSState {
    ISADevice isadev;
    IPMIKCS kcs;
    qemu_irq irq;
    uint32_t uuid;

    struct {
        int32_t isairq;
        uint32_t io_base;
    } cfg;
};

static void nuvoton_ipmi_kcs_get_fwinfo(IPMIInterface *ii, IPMIFwInfo *info)
{
    NuvotonIPMIKCSState *s = NUVOTON_IPMI_KCS(ii);

    ipmi_kcs_get_fwinfo(&s->kcs, info);
    info->interrupt_number = s->cfg.isairq;
    info->uuid = s->uuid;
}

static void nuvoton_ipmi_kcs_raise_irq(IPMIKCS *ik)
{
    NuvotonIPMIKCSState *s = ik->opaque;

    qemu_irq_raise(s->irq);
}

static void nuvoton_ipmi_kcs_lower_irq(IPMIKCS *ik)
{
    NuvotonIPMIKCSState *s = ik->opaque;

    qemu_irq_lower(s->irq);
}

static void *nuvoton_ipmi_kcs_get_backend_data(IPMIInterface *ii)
{
    NuvotonIPMIKCSState *s = NUVOTON_IPMI_KCS(ii);;

    return &s->kcs;
}

static uint32_t nuvoton_pci_ipmi_cfg_read(PCIDevice *dev, uint32_t addr,
                                          int size)
{
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(dev);
    bool hidecfg = FIELD_EX32(s->pci_mbx.hmbxctl, NUVOTON_HMBXCTL, HIDECFG);

    if (hidecfg) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Attempted to read from PCI cfg "
                      "regs when configuration space is hidden\n",
                      object_get_canonical_path(OBJECT(s)));
        return 0;
    }

    return pci_default_read_config(dev, addr, size);
}

static void nuvoton_pci_ipmi_cfg_write(PCIDevice *dev, uint32_t addr,
                                       uint32_t val, int size)
{
    uint8_t *p;
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(dev);
    bool cfglck = FIELD_EX32(s->pci_mbx.hmbxctl, NUVOTON_HMBXCTL, CFGLCK);
    bool hidecfg = FIELD_EX32(s->pci_mbx.hmbxctl, NUVOTON_HMBXCTL, HIDECFG);

    if (hidecfg) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Attempted to write to PCI cfg regs "
                      "when configuration space is hidden\n",
                      object_get_canonical_path(OBJECT(s)));
        return;
    }

    if (cfglck && (addr >= A_NUVOTON_BPCI_VID_DID_WRITE &&
                   addr <= A_NUVOTON_BPCI_SUBSYSTEM_ID_WRITE)) {
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Attempted to write to PCI cfg regs "
                      "when configuration is locked\n",
                      object_get_canonical_path(OBJECT(s)));
        return;
    }

    switch (addr) {
    case A_NUVOTON_BPCI_VID_DID_WRITE:
        p = &dev->config[PCI_VENDOR_ID];
        break;
    case A_NUVOTON_BPCI_CLASS_CODE_WRITE:
        p = &dev->config[PCI_CLASS_REVISION];
        break;
    case A_NUVOTON_BPCI_SUBSYSTEM_ID_WRITE:
        p = &dev->config[PCI_SUBSYSTEM_VENDOR_ID];
        break;
    case A_NUVOTON_BPCI_PMCR:
        /* Clear RO bits */
        val &= ~0xfffffffc;
        p = &dev->config[A_NUVOTON_BPCI_PMCR];
        break;
    default:
        pci_default_write_config(dev, addr, val, size);
        /* Return, don't break. We're done writing */
        return;
    }

    switch (size) {
    case 1:
        return pci_set_byte(p, val);
    case 2:
        return pci_set_word(p, val);
    case 4:
        return pci_set_long(p, val);
    default:
        g_assert_not_reached();
    }
}

static void nuvoton_pcimbx_host_update_irq(NuvotonPCIIPMIState *s)
{
    size_t i;
    uint8_t hif = FIELD_EX32(s->pci_mbx.hmbxstat, NUVOTON_HMBXSTAT, HIF);
    uint8_t hie = FIELD_EX32(s->pci_mbx.hmbxstat, NUVOTON_HMBXCTL, HIE);

    for (i = 0; i < ARRAY_SIZE(s->pci_mbx.irq_host); i++) {
        bool hif_set = extract8(hif, 1 << i, 1);
        bool hie_set = extract8(hie, 1 << i, 1);

        qemu_set_irq(s->pci_mbx.irq_host[i], hif_set && hie_set);
    }
}

static void nuvoton_pcimbx_bmc_update_irq(NuvotonPCIIPMIState *s)
{
    size_t i;
    uint8_t cif = FIELD_EX32(s->pci_mbx.hmbxstat, NUVOTON_HMBXCMD, CIF);

    for (i = 0; i < ARRAY_SIZE(s->pci_mbx.wire_bmc_cif); i++) {
        bool cif_set = extract8(cif, 1 << i, 1);

        qemu_set_irq(s->pci_mbx.wire_bmc_cif[i], cif_set);
    }
}

static uint64_t nuvoton_pci_ipmi_mbx_ram_read(NuvotonPCIIPMIState *s,
                                              hwaddr addr,
                                              unsigned size)

{
    switch(size) {
    case 1:
        return *(uint8_t *)&s->pci_mbx.ram[addr];
    case 2:
        return *(uint16_t *)&s->pci_mbx.ram[addr];
    case 4:
        return *(uint32_t *)&s->pci_mbx.ram[addr];
    case 8:
        return *(uint64_t *)&s->pci_mbx.ram[addr];
    default:
        g_assert_not_reached();
    }
}

static uint64_t nuvoton_pci_ipmi_mbx_reg_read(NuvotonPCIIPMIState *s,
                                              hwaddr addr)
{
    switch (addr) {
    case A_NUVOTON_HMBXSTAT:
        return s->pci_mbx.hmbxstat;
    case A_NUVOTON_HMBXCTL:
        return s->pci_mbx.hmbxctl;
    case A_NUVOTON_HMBXCMD:
        return s->pci_mbx.hmbxcmd;
    /* Anything else in the MMIO region is RES1 */
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Access to unknown register at "
                      "0x%lx\n", object_get_canonical_path(OBJECT(s)), addr);
        return 0xffffffff;
    }
}

static uint64_t nuvoton_pci_ipmi_mbx_read(void *opaque, hwaddr addr,
                                          unsigned size)
{
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(opaque);
    uint64_t data;

    if (addr < NUVOTON_PCI_MBX_REG_START) {
        data = nuvoton_pci_ipmi_mbx_ram_read(s, addr, size);
    } else {
        data = nuvoton_pci_ipmi_mbx_reg_read(s, addr);
    }

    return data;
}

static void nuvoton_pci_ipmi_hmbxstat_w(NuvotonPCIIPMIState *s, uint64_t val)
{
    uint32_t v = (uint32_t) val;

    /* Clear RO and RES bits */
    v &= ~0x7fffff00;
    /* Bits are W1C */
    s->pci_mbx.hmbxstat &= ~v;

    nuvoton_pcimbx_host_update_irq(s);
}

static void nuvoton_pci_ipmi_hmbxctl_w(NuvotonPCIIPMIState *s, uint64_t val)
{
    uint32_t v = (uint32_t) val;
    bool cfglck = FIELD_EX32(s->pci_mbx.hmbxctl, NUVOTON_HMBXCTL, CFGLCK);

    /* Clear RO and RES bits */
    v &= ~0x3fffff00;
    s->pci_mbx.hmbxctl = v;
    /* CFGLCK can only be cleared on reset */
    s->pci_mbx.hmbxctl = FIELD_DP32(s->pci_mbx.hmbxctl, NUVOTON_HMBXCTL, CFGLCK,
                                    cfglck);

    nuvoton_pcimbx_host_update_irq(s);
}

static void nuvoton_pci_ipmi_hmbxcmd_w(NuvotonPCIIPMIState *s, uint64_t val)
{
    uint32_t v = (uint32_t) val;

    /* Clear RO and RES bits */
    v &= ~0xffffff00;
    /* Bits are W1S */
    s->pci_mbx.hmbxcmd |= v;

    nuvoton_pcimbx_bmc_update_irq(s);
}

static void nuvoton_pci_ipmi_mbx_tx_hdr(NuvotonPCIIPMIState *s)
{
    struct packet {
        union {
            struct {
                uint8_t op;
                uint64_t offset;
                uint8_t size;
            } __attribute__((packed));
            uint8_t bytes[10];
        };
    } pkt = {
        .op = NUVOTON_PCI_IPMI_OP_WRITE,
        .offset = s->pci_mbx.tx_addr,
        .size = s->pci_mbx.tx_size,
    };

    /*
     * The BMC chardev can only handle 64-bits at a time, so we need at least
     * 2 transactions to send the header.
     */
    qemu_chr_fe_write_all(&s->pci_mbx.chr, pkt.bytes, 5);
    qemu_chr_fe_write_all(&s->pci_mbx.chr, &pkt.bytes[5], 5);
}

static void nuvoton_pci_ipmi_mbx_tx(NuvotonPCIIPMIState *s)
{
    /*
     * BMC expects no more than 64-bits of data at a time, and our MMIO region
     * also only accepts at most 64-bits at a time. Validate that sanity here.
     */
    assert(s->pci_mbx.size <= 8);

    nuvoton_pci_ipmi_mbx_tx_hdr(s);
    qemu_chr_fe_write(&s->pci_mbx.chr, (uint8_t *)&s->pci_mbx.tx_data,
                              s->pci_mbx.tx_size);
}

static void nuvoton_pci_ipmi_mbx_ram_write(NuvotonPCIIPMIState *s,
                                           hwaddr addr, uint64_t data,
                                           unsigned size)

{
    switch(size) {
    case 1:
        s->pci_mbx.ram[addr] = (uint8_t)data;
        break;
    case 2:
        *(uint16_t *)&s->pci_mbx.ram[addr] = (uint16_t)data;
        break;
    case 4:
        *(uint32_t *)&s->pci_mbx.ram[addr] = (uint32_t)data;
        break;
    case 8:
        *(uint64_t *)&s->pci_mbx.ram[addr] = data;
        break;
    default:
        g_assert_not_reached();
    }
}

static void nuvoton_pci_ipmi_mbx_write_and_tx(NuvotonPCIIPMIState *s,
                                                     hwaddr addr, uint64_t data,
                                                     unsigned size)
{
    s->pci_mbx.tx_addr = addr;
    s->pci_mbx.tx_size = size;
    s->pci_mbx.tx_data = data;

    nuvoton_pci_ipmi_mbx_ram_write(s, addr, data, size);
    nuvoton_pci_ipmi_mbx_tx(s);
}

static void nuvoton_pci_ipmi_mbx_reg_write(NuvotonPCIIPMIState *s, hwaddr addr,
                                           uint64_t val64)
{
    switch (addr) {
    case A_NUVOTON_HMBXSTAT:
        nuvoton_pci_ipmi_hmbxstat_w(s, val64);
        break;
    case A_NUVOTON_HMBXCTL:
        nuvoton_pci_ipmi_hmbxctl_w(s, val64);
        break;
    case A_NUVOTON_HMBXCMD:
        nuvoton_pci_ipmi_hmbxcmd_w(s, val64);
        break;
    default:
        /* Anything else is RES1, ignore writes to it */
        qemu_log_mask(LOG_GUEST_ERROR, "%s: Access to unknown register at "
                      "0x%lx\n", object_get_canonical_path(OBJECT(s)), addr);
        break;
    }
}

static void nuvoton_pci_ipmi_mbx_write(void *opaque, hwaddr addr,
                                       uint64_t val64, unsigned size)
{
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(opaque);

    if (addr < NUVOTON_PCI_MBX_REG_START) {
        nuvoton_pci_ipmi_mbx_write_and_tx(s, addr, val64, size);
    } else {
        nuvoton_pci_ipmi_mbx_reg_write(s, addr, val64);
    }
}

static bool nuvoton_pci_ipmi_mbx_check_mem_op(void *opaque, hwaddr addr,
                                              unsigned size, bool is_write,
                                              MemTxAttrs attrs)
{
    /* If the access is not a register, any access width is fine */
    if (addr < A_NUVOTON_HMBXSTAT || addr > A_NUVOTON_HMBXCMD) {
        return true;
    } else {
        /* It's a register access, ensure it's 32-bits */
        return size == 4;
    }
}

static const struct MemoryRegionOps nuvoton_pci_ipmi_mbx_ops = {
    .read = nuvoton_pci_ipmi_mbx_read,
    .write = nuvoton_pci_ipmi_mbx_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 8,
        .accepts = nuvoton_pci_ipmi_mbx_check_mem_op,
    },
};

static void nuvoton_pci_ipmi_mbx_send_response(NuvotonPCIIPMIState *s,
                                               uint8_t code)
{
    qemu_chr_fe_write(&s->pci_mbx.chr, &code, 1);
    if (code == NUVOTON_PCI_IPMI_CODE_OK &&
        s->pci_mbx.op == NUVOTON_PCI_IPMI_OP_READ) {
        qemu_chr_fe_write(&s->pci_mbx.chr, (uint8_t *)&s->pci_mbx.data,
                          s->pci_mbx.size);
    }
}

static void nuvoton_pci_ipmi_mbx_handle_read(NuvotonPCIIPMIState *s)
{
    s->pci_mbx.data = nuvoton_pci_ipmi_mbx_ram_read(s, s->pci_mbx.offset,
                                                    s->pci_mbx.size);
    nuvoton_pci_ipmi_mbx_send_response(s, MEMTX_OK);
}

static void nuvoton_pci_ipmi_mbx_handle_write(NuvotonPCIIPMIState *s)
{
    nuvoton_pci_ipmi_mbx_ram_write(s, s->pci_mbx.offset, s->pci_mbx.data,
                                   s->pci_mbx.size);
    nuvoton_pci_ipmi_mbx_send_response(s, MEMTX_OK);
}

static void nuvoton_pci_ipmi_mbx_decode_op(NuvotonPCIIPMIState *s,
                                           NuvotonPCIIPMIOp op)
{
    switch (op) {
    case NUVOTON_PCI_IPMI_OP_READ:
    case NUVOTON_PCI_IPMI_OP_WRITE:
        s->pci_mbx.op = op;
        s->pci_mbx.state = NUVOTON_PCI_IPMI_STATE_OFFSET;
        s->pci_mbx.offset = 0;
        s->pci_mbx.rx_cnt = 0;
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "Received unknown op: 0x%x\n", op);
        nuvoton_pci_ipmi_mbx_send_response(s, NUVOTON_PCI_IPMI_CODE_OP_INVALID);
        break;
    }
}

static void nuvoton_pci_ipmi_mbx_handle_idle(NuvotonPCIIPMIState *s,
                                             uint8_t byte)
{
    /* We received an ACK from a write we did, do nothing */
    if (byte == 0x00) {
        return;
    }

    /* Otherwise it's an op */
    nuvoton_pci_ipmi_mbx_decode_op(s, byte);
}

static void nuvoton_pci_ipmi_mbx_handle_size(NuvotonPCIIPMIState *s,
                                             uint8_t size)
{
    s->pci_mbx.size = size;

    if (s->pci_mbx.size < 1 || s->pci_mbx.size > sizeof(s->pci_mbx.data)) {
        nuvoton_pci_ipmi_mbx_send_response(s,
                                           NUVOTON_PCI_IPMI_CODE_SIZE_INVALID);
        s->pci_mbx.state = NUVOTON_PCI_IPMI_STATE_IDLE;
    } else if (s->pci_mbx.op == NUVOTON_PCI_IPMI_OP_READ) {
        nuvoton_pci_ipmi_mbx_handle_read(s);
        s->pci_mbx.state = NUVOTON_PCI_IPMI_STATE_IDLE;
    } else {
        s->pci_mbx.rx_cnt = 0;
        s->pci_mbx.data = 0;
        s->pci_mbx.state = NUVOTON_PCI_IPMI_STATE_DATA;
    }
}

static void nuvoton_pci_ipmi_mbx_handle_data(NuvotonPCIIPMIState *s,
                                             uint8_t byte)
{
    s->pci_mbx.data += (uint64_t)byte <<
                       (s->pci_mbx.rx_cnt * (sizeof(uint8_t) * 8));

    s->pci_mbx.rx_cnt++;
    if (s->pci_mbx.rx_cnt >= s->pci_mbx.size) {
        nuvoton_pci_ipmi_mbx_handle_write(s);
        s->pci_mbx.state = NUVOTON_PCI_IPMI_STATE_IDLE;
    }
}

static void nuvoton_pci_ipmi_mbx_handle_offset(NuvotonPCIIPMIState *s,
                                               uint8_t offset)
{
    s->pci_mbx.offset += offset <<
                         (s->pci_mbx.rx_cnt * (sizeof(uint8_t) * 8));
    s->pci_mbx.rx_cnt++;
    if (s->pci_mbx.rx_cnt >= sizeof(s->pci_mbx.size)) {
        s->pci_mbx.state = NUVOTON_PCI_IPMI_STATE_SIZE;
    }
}

static void nuvoton_pci_ipmi_mbx_receive_byte(NuvotonPCIIPMIState *s,
                                              uint8_t byte)
{
    switch (s->pci_mbx.state) {
    case NUVOTON_PCI_IPMI_STATE_IDLE:
        nuvoton_pci_ipmi_mbx_handle_idle(s, byte);
        break;
    case NUVOTON_PCI_IPMI_STATE_OFFSET:
        nuvoton_pci_ipmi_mbx_handle_offset(s, byte);
        break;
    case NUVOTON_PCI_IPMI_STATE_SIZE:
        nuvoton_pci_ipmi_mbx_handle_size(s, byte);
        break;
    case NUVOTON_PCI_IPMI_STATE_DATA:
        nuvoton_pci_ipmi_mbx_handle_data(s, byte);
        break;
    default:
        g_assert_not_reached();
    }
}

static void nuvoton_pci_ipmi_mbx_event(void *opaque, QEMUChrEvent evt)
{
    switch (evt) {
    case CHR_EVENT_OPENED:
    case CHR_EVENT_CLOSED:
    case CHR_EVENT_BREAK:
    case CHR_EVENT_MUX_IN:
    case CHR_EVENT_MUX_OUT:
        /* Ignore events */
        break;
    default:
        g_assert_not_reached();
    }
}

static int nuvoton_pci_ipmi_mbx_can_receive(void *opaque)
{
    return 1;
}

static void nuvoton_pci_ipmi_mbx_receive(void *opaque, const uint8_t *buf,
                                         int size)
{
    size_t i;
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(opaque);

    for (i = 0; i < size; i++) {
        nuvoton_pci_ipmi_mbx_receive_byte(s, buf[i]);
    }
}

static void nuvoton_pci_ipmi_reset(NuvotonPCIIPMIState *s)
{
    uint8_t *pci_conf = s->pcidev.config;

    pci_set_word(&pci_conf[PCI_VENDOR_ID], NUVOTON_VID_RST);
    pci_set_word(&pci_conf[PCI_DEVICE_ID], NUVOTON_DID_RST);
    pci_set_word(&pci_conf[PCI_COMMAND], NUVOTON_CMD_RST);
    pci_set_word(&pci_conf[PCI_STATUS], NUVOTON_STATUS_RST);
    pci_set_long(&pci_conf[PCI_CLASS_REVISION], NUVOTON_CLASS_REV_RST);
    pci_set_byte(&pci_conf[PCI_CACHE_LINE_SIZE], NUVOTON_CACHE_LINE_SZ_RST);
    pci_set_byte(&pci_conf[PCI_LATENCY_TIMER], NUVOTON_LATENCY_TIMER_RST);
    pci_set_byte(&pci_conf[PCI_HEADER_TYPE], NUVOTON_HDR_TYPE_RST);
    pci_set_long(&pci_conf[PCI_BASE_ADDRESS_0], NUVOTON_BAR0_RST);
    pci_set_long(&pci_conf[PCI_SUBSYSTEM_VENDOR_ID], NUVOTON_SUBSYSTEM_VID_RST);
    pci_set_long(&pci_conf[PCI_CAPABILITY_LIST], NUVOTON_CAPABILITIES_RST);
    pci_set_byte(&pci_conf[PCI_INTERRUPT_LINE], NUVOTON_IRQ_LINE_RST);
    pci_set_byte(&pci_conf[PCI_INTERRUPT_PIN], NUVOTON_IRQ_PIN_RST);
    pci_set_byte(&pci_conf[PCI_MIN_GNT], NUVOTON_MIN_GRANT_RST);
    pci_set_byte(&pci_conf[PCI_MAX_LAT], NUVOTON_MAX_LATENCY_RST);
    pci_set_long(&pci_conf[A_NUVOTON_BPCI_PID_REVISION],
                 NUVOTON_PID_REVISION_RST);
    pci_set_long(&pci_conf[A_NUVOTON_BPCI_PMID], NUVOTON_PMID_RST);
    pci_set_long(&pci_conf[A_NUVOTON_BPCI_PMCR], NUVOTON_PMCR_RST);

    s->pci_mbx.hmbxstat = NUVOTON_HMBXSTAT_RST;
    s->pci_mbx.hmbxctl  = NUVOTON_HMBXCTL_RST;
    s->pci_mbx.hmbxcmd  = NUVOTON_HMBXCMD_RST;
}

static void nuvoton_pci_ipmi_enter_reset(Object  *obj, ResetType type)
{
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(obj);

    nuvoton_pci_ipmi_reset(s);
}

static void nuvoton_pci_ipmi_realize(PCIDevice *pd, Error **errp)
{
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(pd);

    if (!s->ipmi_kcs_state) {
        error_setg(errp, "%s: IPMI KCS portion of device is not connected\n",
                   object_get_canonical_path(OBJECT(pd)));
    }

    pci_config_set_prog_interface(pd->config, 0x01);
    pci_config_set_interrupt_pin(pd->config, 0x01);
    pci_register_bar(pd, 0, PCI_BASE_ADDRESS_SPACE_MEMORY, &s->pci_mbx.iomem);

    qemu_chr_fe_set_handlers(&s->pci_mbx.chr, nuvoton_pci_ipmi_mbx_can_receive,
                             nuvoton_pci_ipmi_mbx_receive,
                             nuvoton_pci_ipmi_mbx_event,
                             NULL, OBJECT(s), NULL, true);
}

static void nuvoton_ipmi_kcs_realize(DeviceState *dev, Error **errp)
{
    Error *err = NULL;
    NuvotonIPMIKCSState *s = NUVOTON_IPMI_KCS(dev);
    ISADevice *isadev = ISA_DEVICE(dev);
    IPMIInterface *ii = IPMI_INTERFACE(dev);
    IPMIInterfaceClass *iic = IPMI_INTERFACE_GET_CLASS(ii);
    IPMICore *ic;

    if (!s->kcs.bmc) {
        error_setg(errp, "%s: IPMI device requires a bmc attribute to be set",
                   object_get_canonical_path(OBJECT(dev)));
        return;
    }

    iic->init(ii, 8, &err);
    if (err) {
        error_propagate(errp, err);
        return;
    }

    ic = IPMI_CORE(s->kcs.bmc);
    ic->intf = ii;

    s->uuid = ipmi_next_uuid();
    s->kcs.io_base = s->cfg.io_base;
    s->kcs.opaque = s;

    if (s->cfg.isairq > 0) {
        s->irq = isa_get_irq(isadev, s->cfg.isairq);
        s->kcs.use_irq = 1;
        s->kcs.raise_irq = nuvoton_ipmi_kcs_raise_irq;
        s->kcs.lower_irq = nuvoton_ipmi_kcs_lower_irq;
    }

    qdev_set_legacy_instance_id(dev, s->kcs.io_base, s->kcs.io_length);
    isa_register_ioport(isadev, &s->kcs.io, s->kcs.io_base);
}


static void nuvoton_ipmi_kcs_init(Object *obj)
{
    NuvotonIPMIKCSState *s = NUVOTON_IPMI_KCS(obj);

    ipmi_bmc_find_and_link(obj, (Object **) &s->kcs.bmc);
}

static const VMStateDescription vmstate_nuvoton_ipmi_kcs = {
    .name = TYPE_IPMI_INTERFACE_PREFIX "nuvoton-ipmi-kcs",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_STRUCT(kcs, NuvotonIPMIKCSState, 1, vmstate_IPMIKCS, IPMIKCS),
        VMSTATE_UINT32(uuid, NuvotonIPMIKCSState),
        VMSTATE_END_OF_LIST()
    }
};

static Property nuvoton_pci_ipmi_properties[] = {
    DEFINE_PROP_CHR("bmc-chardev", NuvotonPCIIPMIState, pci_mbx.chr),
    DEFINE_PROP_LINK("ipmi-kcs", NuvotonPCIIPMIState, ipmi_kcs_state,
                     TYPE_NUVOTON_IPMI_KCS, NuvotonIPMIKCSState *),
    DEFINE_PROP_END_OF_LIST(),
};

static void nuvoton_pci_ipmi_init(Object *obj)
{
    NuvotonPCIIPMIState *s = NUVOTON_PCI_IPMI(obj);

    memory_region_init_io(&s->pci_mbx.iomem, obj, &nuvoton_pci_ipmi_mbx_ops, s,
                          TYPE_NUVOTON_PCI_IPMI "-pci-mbx",
                          NUVOTON_PCI_MBX_MMIO_SIZE);
}

static void nuvoton_pci_ipmi_class_init(ObjectClass *oc, void *data)
{
    PCIDeviceClass *pdc = PCI_DEVICE_CLASS(oc);
    DeviceClass *dc = DEVICE_CLASS(oc);
    ResettableClass *rc = RESETTABLE_CLASS(oc);

    rc->phases.enter = nuvoton_pci_ipmi_enter_reset;

    dc->vmsd = &vmstate_nuvoton_ipmi_kcs;
    device_class_set_props(dc, nuvoton_pci_ipmi_properties);

    pdc->config_read = nuvoton_pci_ipmi_cfg_read;
    pdc->config_write = nuvoton_pci_ipmi_cfg_write;
    pdc->vendor_id = PCI_VENDOR_ID_WINBOND;
    pdc->device_id = PCI_DEVICE_ID_WINBOND_BPCI;
    pdc->revision = 1;
    pdc->class_id = PCI_CLASS_SERIAL_IPMI;
    pdc->realize = nuvoton_pci_ipmi_realize;
}

static const VMStateDescription vmstate_nuvoton_pci_ipmi = {
    .name = "nuvoton-pci-ipmi",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_PCI_DEVICE(pcidev, NuvotonPCIIPMIState),
        VMSTATE_END_OF_LIST()
    }
};

static Property nuvoton_ipmi_kcs_properties[] = {
    DEFINE_PROP_UINT32("portio", NuvotonIPMIKCSState, cfg.io_base, 0x4e),
    DEFINE_PROP_INT32("isa-irq", NuvotonIPMIKCSState, cfg.isairq, 10),
    DEFINE_PROP_END_OF_LIST(),
};

static void nuvoton_ipmi_kcs_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    IPMIInterfaceClass *iic = IPMI_INTERFACE_CLASS(oc);

    dc->vmsd = &vmstate_nuvoton_pci_ipmi;
    dc->realize = nuvoton_ipmi_kcs_realize;
    device_class_set_props(dc, nuvoton_ipmi_kcs_properties);

    iic->get_backend_data = nuvoton_ipmi_kcs_get_backend_data;
    iic->get_fwinfo = nuvoton_ipmi_kcs_get_fwinfo;
    ipmi_kcs_class_init(iic);
}

static const TypeInfo nuvoton_ipmi_kcs_info = {
    .name          = TYPE_NUVOTON_IPMI_KCS,
    .parent        = TYPE_ISA_DEVICE,
    .instance_size = sizeof(NuvotonIPMIKCSState),
    .instance_init = nuvoton_ipmi_kcs_init,
    .class_init    = nuvoton_ipmi_kcs_class_init,
    .interfaces    = (InterfaceInfo[]) {
        { TYPE_IPMI_INTERFACE },
        { },
    },
};

static const TypeInfo nuvoton_pci_ipmi_info = {
    .name          = TYPE_NUVOTON_PCI_IPMI,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(NuvotonPCIIPMIState),
    .instance_init = nuvoton_pci_ipmi_init,
    .class_init    = nuvoton_pci_ipmi_class_init,
    .interfaces    = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void nuvoton_pci_ipmi_kcs_register_types(void)
{
    type_register_static(&nuvoton_pci_ipmi_info);
    type_register_static(&nuvoton_ipmi_kcs_info);
}

type_init(nuvoton_pci_ipmi_kcs_register_types);

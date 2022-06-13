/*
 * PECI - Platform Environment Control Interface
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * This PECI implementation aims to simulate a host with peci as viewed by a
 * BMC. This is developed with OpenBMC firmware running in QEMU.
 *
 * TODO:
 *  - enable per core temperature readings through MSRs or PCI_CONFIG_LOCAL
 *  - enable DIMM error readings through MSRs and PCI_CONFIG_LOCAL
 *  - enable PECI for an x86 guest connected to a separate qemu BMC
 */

#include "qemu/osdep.h"
#include "hw/peci/peci.h"
#include "hw/qdev-properties.h"
#include "migration/vmstate.h"
#include "qemu/module.h"
#include "qemu/log.h"
#include "trace.h"

#define PECI_FCS_OK         0
#define PECI_FCS_ERR        1

static void peci_rd_pkg_cfg(PECIClientDevice *client, PECICmd *pcmd)
{
    PECIPkgCfg *resp = (PECIPkgCfg *)pcmd->tx;
    uint8_t index = pcmd->rx[1];
    uint16_t param = pcmd->rx[3] | pcmd->rx[2];

    switch (index) {
    case PECI_MBX_CPU_ID: /* CPU Family ID*/
        trace_peci_rd_pkg_cfg("CPU ID");
        resp->data = client->cpu_id;
        break;

    case PECI_MBX_DTS_MARGIN:
        trace_peci_rd_pkg_cfg("DTS MARGIN");
    /*
     * Processors return a value of DTS reading in 10.6 format
     * (10 bits signed decimal, 6 bits fractional).
     */
        resp->data = (-1 * client->core_temp_max) << 6;
        break;

    case PECI_MBX_DDR_DIMM_TEMP:
        trace_peci_rd_pkg_cfg("DDR DIMM TEMP");
        if (param > PECI_NUM_CHANNELS_MAX) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "%s: DDR_DIMM_TEMP unsupported channel index %d",
                          __func__, param);
            param = PECI_NUM_CHANNELS_MAX;
        }
        uint8_t channel_index = param * client->dimms_per_channel;
        memcpy(&resp->data,
               &client->dimm_temp[channel_index],
               sizeof(client->dimm_temp[0]) * client->dimms_per_channel);
        break;

    case PECI_MBX_TEMP_TARGET:
        trace_peci_rd_pkg_cfg("TEMP TARGET");
        PECITempTarget target = {
            .tcontrol = client->tcontrol,
            .tjmax = client->tjmax,
        };
        memcpy(&resp->data, &target, sizeof(target));
        break;

    default:
        qemu_log_mask(LOG_UNIMP, "%s: unimplemented PkgCfg Index: %u\n",
                      __func__, index);
    }

    resp->cc = PECI_DEV_CC_SUCCESS;
}

int peci_handle_cmd(PECIBus *bus, PECICmd *pcmd)
{
    PECIClientDevice *client = peci_get_client(bus, pcmd->addr);

    if (!client) { /* ignore commands if clients aren't found */
        qemu_log_mask(LOG_GUEST_ERROR,
                      "%s: no peci client at address: 0x%02x\n",
                      __func__, pcmd->addr);
        return PECI_FCS_ERR;
    }

    /* clear output buffer before handling command */
    for (size_t i = 0; i < pcmd->rd_length; i++) {
        pcmd->tx[i] = 0;
    }

    switch (pcmd->cmd) {
    case PECI_CMD_PING:
        trace_peci_handle_cmd("PING!");
        break;

    case PECI_CMD_GET_DIB: /* Device Info Byte */
        trace_peci_handle_cmd("GetDIB");
        PECIDIB dib = {
            .device_info = client->device_info,
            .revision = client->revision,
        };
        memcpy(pcmd->tx, &dib, sizeof(PECIDIB));
        break;

    case PECI_CMD_GET_TEMP: /* maximum die temp in socket */
        trace_peci_handle_cmd("GetTemp");
        /*
         * The data is returned as a negative value representing the number of
         * degrees centigrade below the maximum processor junction temperature
         */
        memcpy(pcmd->tx, &client->core_temp_max, sizeof(client->core_temp_max));
        break;

    case PECI_CMD_RD_PKG_CFG:
        trace_peci_handle_cmd("RdPkgConfig");
        peci_rd_pkg_cfg(client, pcmd);
        break;

    case PECI_CMD_RD_IA_MSR:
        trace_peci_handle_cmd("RdIAMSR");
        qemu_log_mask(LOG_UNIMP, "%s: unimplemented RD_IA_MSR\n", __func__);
        break;

    case PECI_CMD_RD_PCI_CFG:
        trace_peci_handle_cmd("RdPCIConfig");
        qemu_log_mask(LOG_UNIMP, "%s: unimplemented PCI_CFG\n", __func__);
        break;

    case PECI_CMD_RD_PCI_CFG_LOCAL:
        trace_peci_handle_cmd("RdPCIConfigLocal");
        qemu_log_mask(LOG_UNIMP, "%s: unimplemented PCI_CFG_LOCAL\n", __func__);
        break;

    case PECI_CMD_RD_END_PT_CFG:
        qemu_log_mask(LOG_UNIMP, "%s: unimplemented CMD_RD_END_PT_CFG\n",
                      __func__);
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: unknown cmd: %x\n",
                      __func__, pcmd->cmd);
        break;
    }

    return PECI_FCS_OK;
}

PECIBus *peci_bus_create(DeviceState *parent)
{
    return PECI_BUS(qbus_new(TYPE_PECI_BUS, parent, "peci-bus"));
};

static const TypeInfo peci_types[] = {
    {
        .name = TYPE_PECI_BUS,
        .parent = TYPE_BUS,
        .instance_size = sizeof(PECIBus),
    },
};

DEFINE_TYPES(peci_types);

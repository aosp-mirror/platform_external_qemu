/*
 * DesignWare I2C Module.
 *
 * Copyright 2021 Google LLC
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */
#ifndef DESIGNWARE_I2C_H
#define DESIGNWARE_I2C_H

#include "exec/memory.h"
#include "hw/i2c/i2c.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

/* Size of the FIFO buffers. */
#define DESIGNWARE_I2C_RX_FIFO_SIZE 16
#define DESIGNWARE_I2C_TX_FIFO_SIZE 16

typedef enum DesignWareI2CStatus {
    DW_I2C_STATUS_IDLE,
    DW_I2C_STATUS_SENDING_ADDRESS,
    DW_I2C_STATUS_SENDING,
    DW_I2C_STATUS_RECEIVING,
} DesignWareI2CStatus;

/*
 * struct DesignWareI2CState - DesignWare I2C device state.
 * @bus: The underlying I2C Bus
 * @irq: GIC interrupt line to fire on events
 * @ic_con: : I2C control register
 * @ic_tar: I2C target address register
 * @ic_sar: I2C slave address register
 * @ic_ss_scl_hcnt: Standard speed i2c clock scl high count register
 * @ic_ss_scl_lcnt: Standard speed i2c clock scl low count register
 * @ic_fs_scl_hcnt: Fast mode or fast mode plus i2c clock scl high count
 *                  register
 * @ic_fs_scl_lcnt:Fast mode or fast mode plus i2c clock scl low count
 *                  register
 * @ic_intr_mask: I2C Interrupt Mask Register
 * @ic_raw_intr_stat: I2C raw interrupt status register
 * @ic_rx_tl: I2C receive FIFO threshold register
 * @ic_tx_tl: I2C transmit FIFO threshold register
 * @ic_enable: I2C enable register
 * @ic_status: I2C status register
 * @ic_txflr: I2C transmit fifo level register
 * @ic_rxflr: I2C receive fifo level register
 * @ic_sda_hold: I2C SDA hold time length register
 * @ic_tx_abrt_source: The I2C transmit abort source register
 * @ic_sda_setup: I2C SDA setup register
 * @ic_enable_status: I2C enable status register
 * @ic_fs_spklen: I2C SS, FS or FM+ spike suppression limit
 * @ic_comp_param_1: Component parameter register
 * @ic_comp_version: I2C component version register
 * @ic_comp_type: I2C component type register
 * @rx_fifo: The FIFO buffer for receiving in FIFO mode.
 * @rx_cur: The current position of rx_fifo.
 * @status: The current status of the SMBus.
 */
typedef struct DesignWareI2CState {
    SysBusDevice parent;

    MemoryRegion iomem;

    I2CBus      *bus;
    qemu_irq     irq;

    uint32_t ic_con;
    uint32_t ic_tar;
    uint32_t ic_sar;
    uint32_t ic_ss_scl_hcnt;
    uint32_t ic_ss_scl_lcnt;
    uint32_t ic_fs_scl_hcnt;
    uint32_t ic_fs_scl_lcnt;
    uint32_t ic_intr_mask;
    uint32_t ic_raw_intr_stat;
    uint32_t ic_rx_tl;
    uint32_t ic_tx_tl;
    uint32_t ic_enable;
    uint32_t ic_status;
    uint32_t ic_txflr;
    uint32_t ic_rxflr;
    uint32_t ic_sda_hold;
    uint32_t ic_tx_abrt_source;
    uint32_t ic_sda_setup;
    uint32_t ic_enable_status;
    uint32_t ic_fs_spklen;
    uint32_t ic_comp_param_1;
    uint32_t ic_comp_version;
    uint32_t ic_comp_type;

    uint8_t      rx_fifo[DESIGNWARE_I2C_RX_FIFO_SIZE];
    uint8_t      rx_cur;

    DesignWareI2CStatus status;
} DesignWareI2CState;

#define TYPE_DESIGNWARE_I2C "designware-i2c"
#define DESIGNWARE_I2C(obj) OBJECT_CHECK(DesignWareI2CState, (obj), \
                                        TYPE_DESIGNWARE_I2C)

#endif /* DESIGNWARE_I2C_H */

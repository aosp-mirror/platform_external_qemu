/*
 * Nuvoton NPCM7xx/NPCM8xx SMBus Module.
 *
 * Copyright 2020 Google LLC
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
#ifndef NPCM7XX_SMBUS_H
#define NPCM7XX_SMBUS_H

#include "exec/memory.h"
#include "hw/i2c/i2c.h"
#include "hw/irq.h"
#include "hw/sysbus.h"

/*
 * Number of addresses this module contains. Do not change this without
 * incrementing the version_id in the vmstate.
 */
#define NPCM_SMBUS_NR_ADDRS 10

/* Size of the FIFO buffer. */
#define NPCM7XX_SMBUS_FIFO_SIZE 16
#define NPCM8XX_SMBUS_FIFO_SIZE 32

typedef enum NPCMSMBusStatus {
    NPCM_SMBUS_STATUS_IDLE,
    NPCM_SMBUS_STATUS_SENDING,
    NPCM_SMBUS_STATUS_RECEIVING,
    NPCM_SMBUS_STATUS_NEGACK,
    NPCM_SMBUS_STATUS_STOPPING_LAST_RECEIVE,
    NPCM_SMBUS_STATUS_STOPPING_NEGACK,
} NPCMSMBusStatus;

/*
 * struct NPCMSMBusState - System Management Bus device state.
 * @bus: The underlying I2C Bus.
 * @irq: GIC interrupt line to fire on events (if enabled).
 * @sda: The serial data register.
 * @st: The status register.
 * @cst: The control status register.
 * @cst2: The control status register 2.
 * @cst3: The control status register 3.
 * @ctl1: The control register 1.
 * @ctl2: The control register 2.
 * @ctl3: The control register 3.
 * @ctl4: The control register 4.
 * @ctl5: The control register 5.
 * @addr: The SMBus module's own addresses on the I2C bus.
 * @scllt: The SCL low time register.
 * @sclht: The SCL high time register.
 * @fif_ctl: The FIFO control register.
 * @fif_cts: The FIFO control status register.
 * @fair_per: The fair period register.
 * @txf_ctl: The transmit FIFO control register.
 * @t_out: The SMBus timeout register.
 * @txf_sts: The transmit FIFO status register.
 * @rxf_sts: The receive FIFO status register.
 * @rxf_ctl: The receive FIFO control register.
 * @rx_fifo: The FIFO buffer for receiving in FIFO mode.
 * @rx_cur: The current position of rx_fifo.
 * @status: The current status of the SMBus.
 * @target_device_mode_active: True if the bus is being controlled by the
 *                             device.
 * @target_device_send_bytes: Remaining bytes to send to the target device.
 * @lock: Used to sync between iomem actions and target device actions.
 */
typedef struct NPCMSMBusState {
    SysBusDevice parent;

    MemoryRegion iomem;

    I2CBus      *bus;
    qemu_irq     irq;

    uint8_t      sda;
    uint8_t      st;
    uint8_t      cst;
    uint8_t      cst2;
    uint8_t      cst3;
    uint8_t      ctl1;
    uint8_t      ctl2;
    uint8_t      ctl3;
    uint8_t      ctl4;
    uint8_t      ctl5;
    uint8_t      addr[NPCM_SMBUS_NR_ADDRS];

    uint8_t      scllt;
    uint8_t      sclht;

    uint8_t      fif_ctl;
    uint8_t      fif_cts;
    uint8_t      fair_per;
    uint8_t      txf_ctl;
    uint8_t      frto;
    uint8_t      t_out;
    uint8_t      txf_sts;
    uint8_t      rxf_sts;
    uint8_t      rxf_ctl;

    uint8_t      rx_fifo[NPCM8XX_SMBUS_FIFO_SIZE];
    uint8_t      rx_cur;

    bool         target_device_mode_initiated;
    uint8_t      target_device_address;
    bool         target_device_mode_active;
    int          target_device_send_bytes;

    QemuMutex lock;

    NPCMSMBusStatus status;
} NPCMSMBusState;

typedef struct NPCMSMBusClass {
    SysBusDeviceClass parent;

    bool has_lvl_we;
    uint8_t fifo_size;
    uint8_t rxf_ctl_last;
} NPCMSMBusClass;

#define TYPE_NPCM_SMBUS "npcm-smbus"
OBJECT_DECLARE_TYPE(NPCMSMBusState, NPCMSMBusClass, NPCM_SMBUS)
#define TYPE_NPCM7XX_SMBUS "npcm7xx-smbus"
#define TYPE_NPCM8XX_SMBUS "npcm8xx-smbus"

/*
 * NPCMI2CBus is needed to override device_initiated_transfer in the
 * I2CBusClass.
 */
typedef struct NPCMI2CBus {
    I2CBus parent;
} NPCMI2CBus;

#define TYPE_NPCM_I2C_BUS "npcm-i2c-bus"
OBJECT_DECLARE_SIMPLE_TYPE(NPCMI2CBus, NPCM_I2C_BUS)

#endif /* NPCM7XX_SMBUS_H */

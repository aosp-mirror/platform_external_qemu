/*
 * PECI - Platform Environment Control Interface
 *
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PECI_H
#define PECI_H

#include "hw/qdev-core.h"
#include "qom/object.h"

#define PECI_CMD_PING               0x0
#define PECI_CMD_GET_DIB            0xF7
#define PECI_CMD_GET_TEMP           0x01
#define PECI_CMD_RD_PKG_CFG         0xA1
#define PECI_CMD_WR_PKG_CFG         0xA5
#define   PECI_MBX_CPU_ID            0  /* Package Identifier Read */
#define     PECI_PKG_ID_CPU_ID              0x0000  /* CPUID Info */
#define     PECI_PKG_ID_PLATFORM_ID         0x0001  /* Platform ID */
#define     PECI_PKG_ID_UNCORE_ID           0x0002  /* Uncore Device ID */
#define     PECI_PKG_ID_MAX_THREAD_ID       0x0003  /* Max Thread ID */
#define     PECI_PKG_ID_MICROCODE_REV       0x0004  /* CPU Microcode Revision */
#define     PECI_PKG_ID_MACHINE_CHECK_STATUS 0x0005  /* Machine Check Status */
#define   PECI_MBX_VR_DEBUG          1  /* VR Debug */
#define   PECI_MBX_PKG_TEMP_READ     2  /* Package Temperature Read */
#define   PECI_MBX_ENERGY_COUNTER    3  /* Energy counter */
#define   PECI_MBX_ENERGY_STATUS     4  /* DDR Energy Status */
#define   PECI_MBX_WAKE_MODE_BIT     5  /* "Wake on PECI" Mode bit */
#define   PECI_MBX_EPI               6  /* Efficient Performance Indication */
#define   PECI_MBX_PKG_RAPL_PERF     8  /* Pkg RAPL Performance Status Read */
#define   PECI_MBX_PER_CORE_DTS_TEMP 9  /* Per Core DTS Temperature Read */
#define   PECI_MBX_DTS_MARGIN        10 /* DTS thermal margin */
#define   PECI_MBX_SKT_PWR_THRTL_DUR 11 /* Socket Power Throttled Duration */
#define   PECI_MBX_CFG_TDP_CONTROL   12 /* TDP Config Control */
#define   PECI_MBX_CFG_TDP_LEVELS    13 /* TDP Config Levels */
#define   PECI_MBX_DDR_DIMM_TEMP     14 /* DDR DIMM Temperature */
#define   PECI_MBX_CFG_ICCMAX        15 /* Configurable ICCMAX */
#define   PECI_MBX_TEMP_TARGET       16 /* Temperature Target Read */
#define   PECI_MBX_CURR_CFG_LIMIT    17 /* Current Config Limit */
#define   PECI_MBX_DIMM_TEMP_READ    20 /* Package Thermal Status Read */
#define   PECI_MBX_DRAM_IMC_TMP_READ 22 /* DRAM IMC Temperature Read */
#define   PECI_MBX_DDR_CH_THERM_STAT 23 /* DDR Channel Thermal Status */
#define   PECI_MBX_PKG_POWER_LIMIT1  26 /* Package Power Limit1 */
#define   PECI_MBX_PKG_POWER_LIMIT2  27 /* Package Power Limit2 */
#define   PECI_MBX_TDP               28 /* Thermal design power minimum */
#define   PECI_MBX_TDP_HIGH          29 /* Thermal design power maximum */
#define   PECI_MBX_TDP_UNITS         30 /* Units for power/energy registers */
#define   PECI_MBX_RUN_TIME          31 /* Accumulated Run Time */
#define   PECI_MBX_CONSTRAINED_TIME  32 /* Thermally Constrained Time Read */
#define   PECI_MBX_TURBO_RATIO       33 /* Turbo Activation Ratio */
#define   PECI_MBX_DDR_RAPL_PL1      34 /* DDR RAPL PL1 */
#define   PECI_MBX_DDR_PWR_INFO_HIGH 35 /* DRAM Power Info Read (high) */
#define   PECI_MBX_DDR_PWR_INFO_LOW  36 /* DRAM Power Info Read (low) */
#define   PECI_MBX_DDR_RAPL_PL2      37 /* DDR RAPL PL2 */
#define   PECI_MBX_DDR_RAPL_STATUS   38 /* DDR RAPL Performance Status */
#define   PECI_MBX_DDR_HOT_ABSOLUTE  43 /* DDR Hottest Dimm Absolute Temp */
#define   PECI_MBX_DDR_HOT_RELATIVE  44 /* DDR Hottest Dimm Relative Temp */
#define   PECI_MBX_DDR_THROTTLE_TIME 45 /* DDR Throttle Time */
#define   PECI_MBX_DDR_THERM_STATUS  46 /* DDR Thermal Status */
#define   PECI_MBX_TIME_AVG_TEMP     47 /* Package time-averaged temperature */
#define   PECI_MBX_TURBO_RATIO_LIMIT 49 /* Turbo Ratio Limit Read */
#define   PECI_MBX_HWP_AUTO_OOB      53 /* HWP Autonomous Out-of-band */
#define   PECI_MBX_DDR_WARM_BUDGET   55 /* DDR Warm Power Budget */
#define   PECI_MBX_DDR_HOT_BUDGET    56 /* DDR Hot Power Budget */
#define   PECI_MBX_PKG_PSYS_PWR_LIM3 57 /* Package/Psys Power Limit3 */
#define   PECI_MBX_PKG_PSYS_PWR_LIM1 58 /* Package/Psys Power Limit1 */
#define   PECI_MBX_PKG_PSYS_PWR_LIM2 59 /* Package/Psys Power Limit2 */
#define   PECI_MBX_PKG_PSYS_PWR_LIM4 60 /* Package/Psys Power Limit4 */
#define   PECI_MBX_PERF_LIMIT_REASON 65 /* Performance Limit Reasons */
#define PECI_CMD_RD_IA_MSR          0xB1
#define PECI_CMD_WR_IA_MSR          0xB5
#define PECI_CMD_RD_IA_MSREX        0xD1
#define PECI_CMD_RD_PCI_CFG         0x61
#define PECI_CMD_WR_PCI_CFG         0x65
#define PECI_CMD_RD_PCI_CFG_LOCAL   0xE1
#define PECI_CMD_WR_PCI_CFG_LOCAL   0xE5
#define PECI_CMD_RD_END_PT_CFG      0xC1
#define PECI_CMD_WR_END_PT_CFG      0xC5
#define PECI_CMD_CRASHDUMP_GET_FRAME            0x71

/* Device Specific Completion Code (CC) Definition */
#define PECI_DEV_CC_SUCCESS                            0x40
#define PECI_DEV_CC_NEED_RETRY                         0x80
#define PECI_DEV_CC_OUT_OF_RESOURCE                    0x81
#define PECI_DEV_CC_UNAVAIL_RESOURCE                   0x82
#define PECI_DEV_CC_INVALID_REQ                        0x90
#define PECI_DEV_CC_MCA_ERROR                          0x91
#define PECI_DEV_CC_CATASTROPHIC_MCA_ERROR             0x93
#define PECI_DEV_CC_FATAL_MCA_DETECTED                 0x94
#define PECI_DEV_CC_PARITY_ERROR_ON_GPSB_OR_PMSB       0x98
#define PECI_DEV_CC_PARITY_ERROR_ON_GPSB_OR_PMSB_IERR  0x9B
#define PECI_DEV_CC_PARITY_ERROR_ON_GPSB_OR_PMSB_MCA   0x9C


typedef struct PECIDIB {  /* DIB - Device Info Byte(s) */
    uint8_t device_info; /* bit 2 - num of domains */
    uint8_t revision;
} PECIDIB;

typedef struct  __attribute__ ((__packed__)) {
    uint8_t cc; /* completion code */
    uint32_t data;
} PECIPkgCfg;

typedef struct PECITempTarget {
    uint8_t reserved;
    int8_t tcontrol;
    uint8_t tjmax;
} PECITempTarget;

#define PECI_BASE_ADDR              0x30
#define PECI_BUFFER_SIZE            0x100
#define PECI_NUM_CPUS_MAX           56
#define PECI_DIMMS_PER_CHANNEL_MAX  3
#define PECI_NUM_CHANNELS_MAX       8
#define PECI_NUM_DIMMS_MAX  (PECI_NUM_CHANNELS_MAX * PECI_DIMMS_PER_CHANNEL_MAX)

typedef struct PECIClientDevice {
    DeviceState qdev;
    uint8_t address;
    uint8_t device_info;
    uint8_t revision;

    /* CPU */
    uint32_t cpu_id;
    uint8_t cpus;
    uint8_t tjmax;
    int8_t tcontrol;
    int8_t core_temp_max; /* absolute temp - tjmax */
    uint8_t core_temp[PECI_NUM_CPUS_MAX];

    /* DIMMS */
    uint8_t dimms;
    uint8_t dimms_per_channel;
    uint8_t dimm_temp_max;
    uint8_t dimm_temp[PECI_NUM_DIMMS_MAX];

} PECIClientDevice;

#define TYPE_PECI_CLIENT "peci-client"
OBJECT_DECLARE_SIMPLE_TYPE(PECIClientDevice, PECI_CLIENT)

typedef struct PECIBus {
    BusState qbus;
    uint8_t num_clients;
} PECIBus;

#define TYPE_PECI_BUS "peci-bus"
OBJECT_DECLARE_SIMPLE_TYPE(PECIBus, PECI_BUS)

/* Creates a basic qemu bus onto which PECI clients will be attached */
PECIBus *peci_bus_create(DeviceState *parent);

enum {
    FAM6_HASWELL_X = 0x306F0,
    FAM6_BROADWELL_X = 0x406F0,
    FAM6_SKYLAKE_X = 0x50650,
    FAM6_SKYLAKE_XD = 0x50660,
    FAM6_ICELAKE_X = 0x606A0,
    FAM6_SAPPHIRE_RAPIDS_X = 0x806F3,
};

typedef struct PECIClientProperties {
    uint32_t cpu_family;
    uint8_t cpus;
    uint8_t dimms;
    uint8_t dimms_per_channel;
} PECIClientProperties;

/* Creates a PECI client with the specified cpu and dimm count */
PECIClientDevice *peci_add_client(PECIBus *bus,
                                  uint8_t address,
                                  PECIClientProperties *props);
PECIClientDevice *peci_get_client(PECIBus *bus, uint8_t addr);

typedef struct PECICmd {
    uint8_t addr;
    uint8_t cmd;

    size_t wr_length;
    uint8_t rx[PECI_BUFFER_SIZE];

    size_t rd_length;
    uint8_t tx[PECI_BUFFER_SIZE];
} PECICmd;

int peci_handle_cmd(PECIBus *bus, PECICmd *pcmd);

#endif

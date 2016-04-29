/* Copyright (C) 2011 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/
#ifndef _HW_ANDROID_PIPE_H
#define _HW_ANDROID_PIPE_H

#include <stdbool.h>
#include <stdint.h>
#include "hw/hw.h"

extern bool qemu2_adb_server_init(int port);

#include "android/emulation/android_pipe.h"

/* The following definitions must match those in the kernel driver:
 *
 * For goldfish:
 *   android.googlesource.com/kernel/goldfish.git.
 *   $KERNEL/drivers/misc/qemupipe/qemu_pipe.c
 *
 * For ranchu:
 *   android.googlesource.com/kernel/common.git
 *   $KERNEL/drivers/staging/android/android_pipe.c
 */

/* pipe device registers */
#define PIPE_REG_COMMAND            0x00  /* write: value = command */
#define PIPE_REG_STATUS             0x04  /* read */
#define PIPE_REG_CHANNEL            0x08  /* read/write: channel id */
#define PIPE_REG_SIZE               0x0c  /* read/write: buffer size */
#define PIPE_REG_ADDRESS            0x10  /* write: physical address */
#define PIPE_REG_WAKES              0x14  /* read: wake flags */
/* read/write: parameter buffer address */
#define PIPE_REG_PARAMS_ADDR_LOW     0x18
#define PIPE_REG_PARAMS_ADDR_HIGH    0x1c
/* write: access with paremeter buffer */
#define PIPE_REG_ACCESS_PARAMS       0x20
#define PIPE_REG_VERSION             0x24 /* read: device version */
#define PIPE_REG_CHANNEL_HIGH        0x30 /* read/write: high 32 bit channel id */
#define PIPE_REG_ADDRESS_HIGH        0x34 /* write: high 32 bit physical address */

/* list of commands for PIPE_REG_COMMAND */
#define PIPE_CMD_OPEN               1  /* open new channel */
#define PIPE_CMD_CLOSE              2  /* close channel (from guest) */
#define PIPE_CMD_POLL               3  /* poll read/write status */

/* List of bitflags returned in status of CMD_POLL command */
#define PIPE_POLL_IN   (1 << 0)
#define PIPE_POLL_OUT  (1 << 1)
#define PIPE_POLL_HUP  (1 << 2)

/* The following commands are related to write operations */
#define PIPE_CMD_WRITE_BUFFER       4  /* send a user buffer to the emulator */
#define PIPE_CMD_WAKE_ON_WRITE      5  /* tell the emulator to wake us when writing is possible */

/* The following commands are related to read operations, they must be
 * listed in the same order than the corresponding write ones, since we
 * will use (CMD_READ_BUFFER - CMD_WRITE_BUFFER) as a special offset
 * in qemu_pipe_read_write() below.
 */
#define PIPE_CMD_READ_BUFFER        6  /* receive a page-contained buffer from the emulator */
#define PIPE_CMD_WAKE_ON_READ       7  /* tell the emulator to wake us when reading is possible */

/* Possible status values used to signal errors - see qemu_pipe_error_convert */
#define PIPE_ERROR_INVAL       -1
#define PIPE_ERROR_AGAIN       -2
#define PIPE_ERROR_NOMEM       -3
#define PIPE_ERROR_IO          -4

/* Bit-flags used to signal events from the emulator */
#define PIPE_WAKE_CLOSED       (1 << 0)  /* emulator closed pipe */
#define PIPE_WAKE_READ         (1 << 1)  /* pipe can now be read from */
#define PIPE_WAKE_WRITE        (1 << 2)  /* pipe can now be written to */

struct access_params_32 {
    uint32_t channel;   /* 0x00 */
    uint32_t size;      /* 0x04 */
    uint32_t address;   /* 0x08 */
    uint32_t cmd;       /* 0x0c */
    uint32_t result;    /* 0x10 */
    /* reserved for future extension */
    uint32_t flags;     /* 0x14 */
};

struct access_params_64 {
    uint64_t channel;   /* 0x00 */
    uint32_t size;      /* 0x08 */
    uint64_t address;   /* 0x0c */
    uint32_t cmd;       /* 0x14 */
    uint32_t result;    /* 0x18 */
    /* reserved for future extension */
    uint32_t flags;     /* 0x1c */
};

union access_params {
    struct access_params_32 aps32;
    struct access_params_64 aps64;
};


extern void android_zero_pipe_init(void);
extern void android_pingpong_init(void);
extern void android_throttle_init(void);
extern void android_adb_dbg_backend_init(void);
extern void android_adb_backend_init(void);
extern void android_sensors_init(void);
extern void android_net_pipes_init(void);

#endif /* _HW_ANDROID_PIPE_H */

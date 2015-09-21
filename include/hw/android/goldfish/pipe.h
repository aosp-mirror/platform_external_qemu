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
#ifndef _HW_GOLDFISH_PIPE_H
#define _HW_GOLDFISH_PIPE_H

#include "android/emulation/android_pipe.h"

/* The following definitions must match those under:
 *
 *    $KERNEL/drivers/misc/qemupipe/qemu_pipe.c
 *
 * Where $KERNEL points to the android-goldfish-2.6.xx branch on:
 *
 *     android.googlesource.com/kernel/goldfish.git.
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

void pipe_dev_init(bool newDeviceNaming);

struct access_params{
    uint32_t channel;
    uint32_t size;
    uint32_t address;
    uint32_t cmd;
    uint32_t result;
    /* reserved for future extension */
    uint32_t flags;
};

struct access_params_64 {
    uint64_t channel;
    uint32_t size;
    uint64_t address;
    uint32_t cmd;
    uint32_t result;
    /* reserved for future extension */
    uint32_t flags;
};

// Register 'zero' pipe service.
extern void android_pipe_add_type_zero(void);
extern void android_pipe_add_type_pingpong(void);
extern void android_pipe_add_type_throttle(void);

#endif /* _HW_GOLDFISH_PIPE_H */

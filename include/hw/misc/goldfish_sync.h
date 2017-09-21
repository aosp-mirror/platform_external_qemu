/* Copyright (C) 2016 The Android Open Source Project
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

#ifndef HW_MISC_GOLDFISH_SYNC_H
#define HW_MISC_GOLDFISH_SYNC_H

#include <stdint.h>

/* A list of commands to send to the guest through the goldfish_sync device
 * CMD_CREATE_TIMELINE: used to create a new timeline handle.
 * CMD_CREATE_FENCE: used to create a new sync fence handle.
 * CMD_INCREMENT_TIMELINE: used to increment a timeline's value.
 * CMD_DESTROY_TIMELINE: used to destroy a given timeline handle.
 * and sync thread handle. */
typedef enum {
    GOLDFISH_SYNC_CMD_CREATE_TIMELINE = 1,
    GOLDFISH_SYNC_CMD_CREATE_FENCE = 2,
    GOLDFISH_SYNC_CMD_INCREMENT_TIMELINE = 3,
    GOLDFISH_SYNC_CMD_DESTROY_TIMELINE = 4,
} GoldfishSyncCommand;

/* A set of callbacks that must be implemented by the host-side sync service
 * and will be called by the virtual device at runtime. */
typedef struct {
    /* Called when the guest sends the result of a previous command to the
     * host. */
    void (*receive_hostcmd_result)(uint32_t cmd,
                                   uint64_t handle,
                                   uint32_t time_arg,
                                   uint64_t hostcmd_handle);

    /* Called when the guest wants to trigger a host-side wait for a
     * specific glsync and thread pointer pair */
    void (*trigger_host_wait)(uint64_t glsync_ptr,
                              uint64_t thread_ptr,
                              uint64_t timeline);
    /* Save/Load hooks for pending sync operations. */
    void (*save)(QEMUFile* file);
    void (*load)(QEMUFile* file);
} GoldfishSyncServiceOps;

/* Register the host-side sync service callbacks with the device. This
 * must be called at emulation setup time before the device runs. */
void goldfish_sync_set_service_ops(const GoldfishSyncServiceOps *ops);

/* Send a command to the guest through the goldfish_sync device. */
void goldfish_sync_send_command(uint32_t cmd,
                                uint64_t handle,
                                uint32_t time_arg,
                                uint64_t hostcmd_handle);

#endif  /* HW_MISC_GOLDFISH_SYNC_H */

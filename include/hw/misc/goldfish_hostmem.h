/* Copyright (C) 2018 The Android Open Source Project
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

#ifndef HW_MISC_GOLDFISH_HOSTMEM_H
#define HW_MISC_GOLDFISH_HOSTMEM_H

#include <stdint.h>

typedef enum {
    GOLDFISH_HOSTMEM_CMD_SET_PTR = 1,
    GOLDFISH_HOSTMEM_CMD_UNSET_PTR = 2,
} GoldfishHostmemCommand;

typedef struct {
    /* Save/Load hooks for snapshots */
    void (*save)(QEMUFile* file);
    void (*load)(QEMUFile* file);
} GoldfishHostmemServiceOps;

/* Register the host-side hostmem service callbacks with the device. This
 * must be called at emulation setup time before the device runs. */
void goldfish_hostmem_set_service_ops(const GoldfishHostmemServiceOps *ops);

/* Send a command to the guest through the goldfish_hostmem device. */
void goldfish_hostmem_send_command(uint64_t gpa,
                                   uint64_t id,
                                   void* hostmem,
                                   uint64_t size,
                                   uint64_t status_code,
                                   uint64_t cmd);

#endif  /* HW_MISC_GOLDFISH_HOSTMEM_H */

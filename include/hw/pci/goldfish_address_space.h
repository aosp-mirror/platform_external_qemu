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
#ifndef _HW_GOLDFISH_ADDRESS_SPACE_H
#define _HW_GOLDFISH_ADDRESS_SPACE_H

#include "qemu/typedefs.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct GoldfishAddressSpaceOpsTag {
    int (*load)(QEMUFile *file);
    int (*save)(QEMUFile *file);
} GoldfishAddressSpaceOps;

extern void
goldfish_address_space_set_service_ops(const GoldfishAddressSpaceOps* ops);

#endif /* _HW_GOLDFISH_ADDRESS_SPACE_H */

/*
** Copyright (c) 2015, Intel Corporation
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

/****************************************************************
 * Android-specific virtual hardware (goldfish devices)
 ****************************************************************/

#include "hw/acpi/goldfish_defs.h"

Scope(\_SB) {

    /* Battery */
    Device(GFBY) {
        Name(_HID, "GFSH0001")
        Name(_STR, Unicode("goldfish battery"))
        Name(_CRS, ResourceTemplate() {
            Memory32Fixed(ReadWrite,
                GF_BATTERY_IOMEM_BASE,
                GF_BATTERY_IOMEM_SIZE
                )
            Interrupt(, Edge, ActiveHigh) {
                GF_BATTERY_IRQ
            }
        })
    }

    /* Events */
    Device(GFEV) {
        Name(_HID, "GFSH0002")
        Name(_STR, Unicode("goldfish events"))
        Name(_CRS, ResourceTemplate() {
            Memory32Fixed(ReadWrite,
                GF_EVENTS_IOMEM_BASE,
                GF_EVENTS_IOMEM_SIZE
                )
            Interrupt(, Edge, ActiveHigh) {
                GF_EVENTS_IRQ
            }
        })
    }

}

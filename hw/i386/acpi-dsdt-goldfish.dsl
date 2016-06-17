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

    /* Pipe */
    Device(GFPP) {
        Name(_HID, "GFSH0003")
        Name(_STR, Unicode("android pipe"))
        Name(_CRS, ResourceTemplate() {
            Memory32Fixed(ReadWrite,
                GF_PIPE_IOMEM_BASE,
                GF_PIPE_IOMEM_SIZE
                )
            Interrupt(, Edge, ActiveHigh) {
                GF_PIPE_IRQ
            }
        })
    }

    /* Framebuffer */
    Device(GFFB) {
        Name(_HID, "GFSH0004")
        Name(_STR, Unicode("goldfish framebuffer"))
        Name(_CRS, ResourceTemplate() {
            Memory32Fixed(ReadWrite,
                GF_FB_IOMEM_BASE,
                GF_FB_IOMEM_SIZE
                )
            Interrupt(, Edge, ActiveHigh) {
                GF_FB_IRQ
            }
        })
    }

    /* Audio */
    Device(GFAU) {
        Name(_HID, "GFSH0005")
        Name(_STR, Unicode("goldfish audio"))
        Name(_CRS, ResourceTemplate() {
            Memory32Fixed(ReadWrite,
                GF_AUDIO_IOMEM_BASE,
                GF_AUDIO_IOMEM_SIZE
                )
            Interrupt(, Edge, ActiveHigh) {
                GF_AUDIO_IRQ
            }
        })
    }

    /* Sync */
    Device(GFSK) {
        Name(_HID, "GFSH0006")
        Name(_STR, Unicode("goldfish sync"))
        Name(_CRS, ResourceTemplate() {
            Memory32Fixed(ReadWrite,
                GF_SYNC_IOMEM_BASE,
                GF_SYNC_IOMEM_SIZE
                )
            Interrupt(, Edge, ActiveHigh) {
                GF_SYNC_IRQ
                }
        })
    }

    /* RTC */
    Device(GFRT) {
        Name(_HID, "GFSH0007")
        Name(_STR, Unicode("goldfish rtc"))
        Name(_CRS, ResourceTemplate() {
            Memory32Fixed(ReadWrite,
                GF_RTC_IOMEM_BASE,
                GF_RTC_IOMEM_SIZE
                )
            Interrupt(, Edge, ActiveHigh) {
                GF_RTC_IRQ
            }
        })
    }
}

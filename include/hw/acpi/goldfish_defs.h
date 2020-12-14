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

/* Defines I/O resources (memory regions, interrupts, etc.) required by Android-
 * specific virtual hardware for x86/x86_64 emulation. Shared between ASL and C
 * code.
 *
 * For I/O memory, we use 0xff001000 and above.
 * For interrupts, we use lines 16 through 24.
 */

#ifndef ACPI_GOLDFISH_DEFS_H
#define ACPI_GOLDFISH_DEFS_H

/* goldfish battery */
#define GOLDFISH_BATTERY_IOMEM_BASE   0xff010000
#define GOLDFISH_BATTERY_IOMEM_SIZE   0x00001000
#define GOLDFISH_BATTERY_IRQ          16

/* goldfish events */
#define GOLDFISH_EVENTS_IOMEM_BASE    0xff011000
#define GOLDFISH_EVENTS_IOMEM_SIZE    0x00001000
#define GOLDFISH_EVENTS_IRQ           17

/* android pipe */
#define GOLDFISH_PIPE_IOMEM_BASE      0xff001000
#define GOLDFISH_PIPE_IOMEM_SIZE      0x00002000
#define GOLDFISH_PIPE_IRQ             18

/* goldfish framebuffer */
#define GOLDFISH_FB_IOMEM_BASE        0xff012000
#define GOLDFISH_FB_IOMEM_SIZE        0x00000100
#define GOLDFISH_FB_IRQ               19

/* goldfish audio */
#define GOLDFISH_AUDIO_IOMEM_BASE     0xff013000
#define GOLDFISH_AUDIO_IOMEM_SIZE     0x00000100
#define GOLDFISH_AUDIO_IRQ            20

/* goldfish sync */
#define GOLDFISH_SYNC_IOMEM_BASE      0xff014000
#define GOLDFISH_SYNC_IOMEM_SIZE      0x00002000
#define GOLDFISH_SYNC_IRQ             21

/* goldfish rtc */
#define GOLDFISH_RTC_IOMEM_BASE       0xff016000
#define GOLDFISH_RTC_IOMEM_SIZE       0x00001000
#define GOLDFISH_RTC_IRQ              22

/* goldfish rotary */
#define GOLDFISH_ROTARY_IOMEM_BASE    0xff017000
#define GOLDFISH_ROTARY_IOMEM_SIZE    0x00001000
#define GOLDFISH_ROTARY_IRQ           23

#define GOLDFISH_ADDRESS_SPACE_PCI_REVISION         1
#define GOLDFISH_ADDRESS_SPACE_PCI_SLOT             11
#define GOLDFISH_ADDRESS_SPACE_PCI_FUNCTION         0
#define GOLDFISH_ADDRESS_SPACE_NAME                 "goldfish_address_space"
#define GOLDFISH_ADDRESS_SPACE_PCI_VENDOR_ID        0x607D
#define GOLDFISH_ADDRESS_SPACE_PCI_DEVICE_ID        0xF153
#define GOLDFISH_ADDRESS_SPACE_PCI_INTERRUPT_PIN    2 /* pin B */
#define GOLDFISH_ADDRESS_SPACE_PCI_INTERRUPT_LINE   2
#define GOLDFISH_ADDRESS_SPACE_CONTROL_BAR          0
#define GOLDFISH_ADDRESS_SPACE_CONTROL_NAME         "goldfish_address_space_control"
#define GOLDFISH_ADDRESS_SPACE_CONTROL_SIZE         0x00001000
#define GOLDFISH_ADDRESS_SPACE_AREA_BAR             1
#define GOLDFISH_ADDRESS_SPACE_AREA_NAME            "goldfish_address_space_area"
#if defined(__APPLE__) && defined(__arm64__)
// Apple Silicon only has 36 bits of address space. Be more conservative here.
#define GOLDFISH_ADDRESS_SPACE_AREA_SIZE            (16ULL << 29ULL)
#else
#define GOLDFISH_ADDRESS_SPACE_AREA_SIZE            (16ULL << 30ULL)
#endif

#endif  /* !ACPI_GOLDFISH_DEFS_H */

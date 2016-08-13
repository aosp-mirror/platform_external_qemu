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
 * For interrupts, we use lines 16 through 23.
 */

#ifndef ACPI_GOLDFISH_DEFS_H
#define ACPI_GOLDFISH_DEFS_H

/* goldfish battery */
#define GF_BATTERY_IOMEM_BASE   0xff010000
#define GF_BATTERY_IOMEM_SIZE   0x00001000
#define GF_BATTERY_IRQ          16

/* goldfish events */
#define GF_EVENTS_IOMEM_BASE    0xff011000
#define GF_EVENTS_IOMEM_SIZE    0x00001000
#define GF_EVENTS_IRQ           17

/* android pipe */
#define GF_PIPE_IOMEM_BASE      0xff001000
#define GF_PIPE_IOMEM_SIZE      0x00002000
#define GF_PIPE_IRQ             18

/* goldfish framebuffer */
#define GF_FB_IOMEM_BASE        0xff012000
#define GF_FB_IOMEM_SIZE        0x00000100
#define GF_FB_IRQ               19

/* goldfish audio */
#define GF_AUDIO_IOMEM_BASE     0xff013000
#define GF_AUDIO_IOMEM_SIZE     0x00000100
#define GF_AUDIO_IRQ            20

/* goldfish sync */
#define GF_SYNC_IOMEM_BASE      0xff014000
#define GF_SYNC_IOMEM_SIZE      0x00002000
#define GF_SYNC_IRQ             21

/* goldfish rtc */
#define GF_RTC_IOMEM_BASE       0xff016000
#define GF_RTC_IOMEM_SIZE       0x00001000
#define GF_RTC_IRQ              22

#endif  /* !ACPI_GOLDFISH_DEFS_H */

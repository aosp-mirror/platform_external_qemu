/* Copyright (C) 2007-2008 The Android Open Source Project
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
#ifndef GOLDFISH_PIC_H
#define GOLDFISH_PIC_H

#ifdef TARGET_I386
/* Maximum IRQ number available for a device on x86. */
#define GFD_MAX_IRQ     16
#else
/* Maximum IRQ number available for a device on (ARM/MIPS) */
#define GFD_MAX_IRQ     32
#endif

/* device init functions */
qemu_irq *goldfish_pic_init(uint32_t base, qemu_irq parent_irq);

#endif  /* GOLDFISH_PIC_H */

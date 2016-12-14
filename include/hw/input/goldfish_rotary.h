/* Copyright (C) 2007-2016 The Android Open Source Project
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
#ifndef _HW_GOLDFISH_ROTARY_H
#define _HW_GOLDFISH_ROTARY_H

// Sends a REL_WHEEL event to the emulator.
// parameter is a signed value indicating the size and direction of the
// rotation.
extern int goldfish_rotary_send_rotate(int value);

#endif

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

/* This extends the list of key codes that are defined in linux_keycodes.h
 */

#pragma once

#define NSEventModifierFlagCapsLock (1 << 16)
#define NSEventModifierFlagShift (1 << 17)
#define NSEventModifierFlagControl (1 << 18)
#define NSEventModifierFlagOption (1 << 19)
#define NSEventModifierFlagCommand (1 << 20)
#define NSEventModifierFlagNumericPad (1 << 21)
#define NSEventModifierFlagDeviceIndependentFlagsMask 0xffff0000UL
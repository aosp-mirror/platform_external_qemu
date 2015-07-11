/* Copyright (C) 2015 The Android Open Source Project
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

// Interfaces between the UI and the emulator

#ifndef UI_EMULATOR_IF_H
#define UI_EMULATOR_IF_H

typedef struct UiEmulatorIf_s {
    const struct BatteryIf_s    *battery;
    const struct CellularIf_s   *cellular;
    const struct TelephonyIf_s  *telephony;
} UiEmulatorIf;

#endif // UI_EMULATOR_IF_H

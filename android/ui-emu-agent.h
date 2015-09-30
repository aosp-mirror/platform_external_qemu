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

#pragma once

// Agent iterfaces for sending commands from the UI to the emulator

typedef struct UiEmuAgent {
    const struct QAndroidBatteryAgent* battery;
    const struct CellularAgent* cellular;
    const struct FingerAgent* finger;
    const struct LocationAgent* location;
    const struct SettingsAgent* settings;
    const struct TelephonyAgent* telephony;
} UiEmuAgent;

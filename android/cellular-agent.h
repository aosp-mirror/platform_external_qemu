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

#ifndef ANDROID_CELLULAR_AGENT_H
#define ANDROID_CELLULAR_AGENT_H

enum CellularStatus { Cellular_Stat_Home,   Cellular_Stat_Roaming, Cellular_Stat_Searching, 
                      Cellular_Stat_Denied, Cellular_Stat_Unregistered };

enum CellularStandard { Cellular_Std_GSM,  Cellular_Std_HSCSD, Cellular_Std_GPRS, Cellular_Std_EDGE,
                        Cellular_Std_UMTS, Cellular_Std_HSDPA, Cellular_Std_full };

typedef struct CellularAgent {
    void (*setSignalStrength)(int zeroTo31);
    void (*setVoiceStatus)(enum CellularStatus);
    void (*setDataStatus)(enum CellularStatus);
    void (*setStandard)(enum CellularStandard);
} CellularAgent;

#endif // ANDROID_CELLULAR_AGENT_H

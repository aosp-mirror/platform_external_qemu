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

#ifndef CELLULAR_IF_H
#define CELLULAR_IF_H

enum CellularStatus { Cellular_S_Connected, Cellular_S_Unregistered,
                      Cellular_S_Roaming, Cellular_S_Searching, Cellular_S_Denied };

enum CellularDataStandard { Cellular_GSM,  Cellular_HSCSD, Cellular_GPRS,
                            Cellular_EDGE, Cellular_UMTS, Cellular_HSDPA, Cellular_LTE };

typedef struct CellularIf_s {
    void (*setSignalStrength)(int percentOfMax);
    void (*setVoiceStatus)(enum CellularStatus);
    void (*setDataStatus)(enum CellularStatus);
    void (*setDataStandard)(enum CellularDataStandard);
} CellularIf;

#endif // CELLULAR_IF_H

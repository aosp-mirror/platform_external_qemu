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

#ifndef ANDROID_CELLULAR_AGENT_IMPL_H
#define ANDROID_CELLULAR_AGENT_IMPL_H

#include "android/cellular-agent.h"

// These functions implement the interface functions
// that are defined in cellular-agent.h

void cellular_setDataStatus(enum CellularStatus);
void cellular_setSignalStrength(int zeroTo31);
void cellular_setStandard(enum CellularStandard);
void cellular_setVoiceStatus(enum CellularStatus);

#endif // ANDROID_CELLULAR_AGENT_IMPL_H

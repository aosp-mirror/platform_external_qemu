/* Copyright (C) 2017 The Android Open Source Project
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
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

// Flag used to indicate what type of component in the bugreport
typedef enum BugreportDataType {
    AdbBugreport = (1 << 0),
    AdbLogcat = (1 << 1),
    AvdDetails = (1 << 2),
    Screenshot = (1 << 3),
} BugreportDataType;

typedef struct BugreportData BugreportData;

typedef void(BugreportCallback)(BugreportData* data,
                                BugreportDataType type,
                                void* context);

BugreportData* generateBugreport(BugreportCallback callback, void* context);

// Return 1 if the bugreport has finished data collection otherwise 0
int bugreportIsReady(BugreportData* data);
// Return 0 if the entire bugreport folder is saved successfully otherwise
// return -1
int saveBugreport(BugreportData* bugreport, int savingFlag);
// Return 0 if the savePath is a directory and writable, otherwise return -1
int setSavePath(BugreportData* bugreport, const char* savePath);

void bugReportInit(void* adbInterface, void* screencap);
ANDROID_END_HEADER
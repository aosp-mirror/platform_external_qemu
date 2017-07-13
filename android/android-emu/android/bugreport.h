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

typedef enum BugreportDataType {
    AdbBugreport,
    AdbLogcat,
    AvdDetails,
    Screenshot
} BugreportDataType;

typedef struct BugreportData BugreportData;

typedef void(BugreportCallback)(const BugreportData* data,
                                BugreportDataType type,
                                void* context);

void generateBugreport(BugreportCallback callback, void* context);
// Return 0 if the entire bugreport folder is saved successfully otherwise
// return -1
int saveBugreport(int includeScreenshot, int includeAdbBugreport);
// Return 0 if issue tracker is opened in browser with fields populated
// otherwise return -1
int sendToGoogle();
void setReproSteps(const char* reproSteps);
// Return 0 if |savePath| is valid otherwie return -1
int setSavePath(const char* savePath);
void bugReportInit(void* adbInterface, int hasWindow);
ANDROID_END_HEADER
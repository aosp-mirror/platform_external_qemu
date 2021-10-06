/* Copyright (C) 2021 The Android Open Source Project
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

#ifndef _RESIZABLE_DISPLAY_CONFIG_H
#define _RESIZABLE_DISPLAY_CONFIG_H

#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

enum PresetEmulatorSizeType {
    PRESET_SIZE_PHONE = 0,
    PRESET_SIZE_UNFOLDED,
    PRESET_SIZE_TABLET,
    PRESET_SIZE_DESKTOP,
    PRESET_SIZE_MAX,    // max value for resizable type
};

struct PresetEmulatorSizeInfo {
    enum PresetEmulatorSizeType type;
    int width;
    int height;
    int dpi;
};

bool resizableEnabled();
void resizableInit();
bool getResizableConfig(enum PresetEmulatorSizeType id,
                        struct PresetEmulatorSizeInfo* info);
enum PresetEmulatorSizeType getResizableActiveConfigId();
void setResizableActiveConfigId(enum PresetEmulatorSizeType id);
void updateAndroidDisplayConfigPath(enum PresetEmulatorSizeType id);

ANDROID_END_HEADER
#endif

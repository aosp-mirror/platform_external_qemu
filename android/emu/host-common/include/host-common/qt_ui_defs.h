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
#include <stdbool.h>
#include <stdint.h>

#include "android/utils/compiler.h"
#include "android/skin/rect.h"

ANDROID_BEGIN_HEADER


typedef enum {
    SETTINGS_THEME_LIGHT,
    SETTINGS_THEME_DARK,
    SETTINGS_THEME_STUDIO_LIGHT,
    SETTINGS_THEME_STUDIO_DARK,
    SETTINGS_THEME_STUDIO_CONTRAST,
    SETTINGS_THEME_NUM_ENTRIES
} SettingsTheme;


// This gives the order of the tabbed panes on the extended window.
// This must correspond to the ordering that is set from within
// QtDesigner and written to extended.ui.
typedef enum {
    PANE_IDX_UNKNOWN = -1,
    PANE_IDX_LOCATION = 0,
    PANE_IDX_MULTIDISPLAY,
    PANE_IDX_CELLULAR,
    PANE_IDX_BATTERY,
    PANE_IDX_CAMERA,
    PANE_IDX_TELEPHONE,
    PANE_IDX_DPAD,
    PANE_IDX_TV_REMOTE,
    PANE_IDX_ROTARY,
    PANE_IDX_MICROPHONE,
    PANE_IDX_FINGER,
    PANE_IDX_VIRT_SENSORS,
    PANE_IDX_SNAPSHOT,
    PANE_IDX_BUGREPORT,
    PANE_IDX_RECORD,
    PANE_IDX_GOOGLE_PLAY,
    PANE_IDX_SETTINGS,
    PANE_IDX_HELP,
    PANE_IDX_CAR,
    PANE_IDX_CAR_ROTARY,
    PANE_IDX_SENSOR_REPLAY,
} ExtendedWindowPane;

typedef enum {
    LEFT,
    HCENTER,
    RIGHT,
} HorizontalAnchor;

typedef enum {
    TOP,
    VCENTER,
    BOTTOM,
} VerticalAnchor;

typedef struct SkinLayout SkinLayout;
typedef struct QFrame QFrame;
typedef struct SkinEvent SkinEvent;

ANDROID_END_HEADER

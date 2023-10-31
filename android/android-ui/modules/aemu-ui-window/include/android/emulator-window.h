/* Copyright (C) 2006-2016 The Android Open Source Project
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

#include <stdbool.h>                            // for bool

#include "aemu/base/c_header.h"
#include "android/cmdline-definitions.h"
#include "android/skin/file.h"                  // for SkinFile, SkinLayout
#include "android/skin/image.h"                 // for SkinImage
#include "android/skin/ui.h"                    // for SkinUI
#include "android/ui-emu-agent.h"               // for UiEmuAgent
#include "android/utils/aconfig-file.h"         // for AConfig
#include "host-common/misc.h"
#include "host-common/screen-recorder.h"  // for RecordingInfo, Record...

ANDROID_BEGIN_HEADER

typedef struct EmulatorScreenMask {
    int    width;
    int    height;
    const unsigned char* rgbaData;
} EmulatorScreenMask;

#define MY_PRESET_SIZE_MAX 4

typedef struct EmulatorWindow {
    const AConfig* aconfig;
    SkinFile*      layout_file;
    SkinFile*      layout_file_skin;
    SkinFile*      layout_file_no_skin;
    SkinFile* layout_file_folded_skin;
    SkinFile* layout_file_resizable_skins[MY_PRESET_SIZE_MAX];
    int            win_x;
    int            win_y;
    SkinUI*        ui;
    SkinImage*     onion;
    SkinRotation   onion_rotation;
    int            onion_alpha;
    bool           done;

    AndroidOptions opts[1];  /* copy of options */
    UiEmuAgent     uiEmuAgent[1];
} EmulatorWindow;

/* Gets a pointer to a EmulatorWindow structure instance. */
EmulatorWindow*
emulator_window_get(void);


void android_emulator_set_base_port(int);

/* Initializes EmulatorWindow structure instance. */
int emulator_window_init(EmulatorWindow* emulator,
                         const AConfig* aconfig,
                         const char* basepath,
                         int x,
                         int y,
                         const AndroidOptions* opts,
                         const UiEmuAgent* uiEmuAgent);

/* Uninitializes EmulatorWindow structure instance on exit. */
void
emulator_window_done(EmulatorWindow* emulator);

/* Gets emulator's layout. */
SkinLayout*
emulator_window_get_layout(EmulatorWindow* emulator);

/* Rotates the device underlying the emulator window according to the input,
 * with the top tilted away from the user by tilt_degrees. */
void emulator_window_set_device_coarse_orientation(
        SkinRotation orientation, float tilt_degrees);

/* Rotates the screen by 90 degrees either clockwise or counterclockwise
 * according to the input argument. Returns true on success, false otherwise */
bool emulator_window_rotate_90(bool clockwise);

/* Rotates the emulator UI to match the input rotation, but not the underlying
 * device.  This should be called in response to a device orientation change. */
bool emulator_window_rotate(int rotation);

/* Start recording the screen. Returns false if recording is already
 * running. */
bool emulator_window_start_recording(const RecordingInfo* info);
/* Async version of emulator_window_start_recording. Use |info->cb| for
 * recording status. */
bool emulator_window_start_recording_async(const RecordingInfo* info);
/* Stop recording the screen. */
bool emulator_window_stop_recording(void);
/* Async version of emulator_window_stop_recording. Use |info->cb| for recording
 * status. */
bool emulator_window_stop_recording_async(void);
/* Returns the current state of the screen recorder. */
RecorderStates emulator_window_recorder_state_get(void);

void emulator_window_set_screen_mask(int width, int height, const unsigned char* rgbaData);

void emulator_window_set_no_skin();

void emulator_window_set_folded_skin();
void emulator_window_set_skin(int skinIndex);

void emulator_window_restore_skin();

void emulator_window_opengles_set_display_active_config(int configId);

ANDROID_END_HEADER

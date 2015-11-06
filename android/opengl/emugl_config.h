// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// A small structure used to model the EmuGL configuration
// to use.
// |enabled| is true if GPU emulation is enabled, false otherwise.
// |backend| contains the name of the backend to use, if |enabled|
// is true.
// |status| is a string used to report error or the current status
// of EmuGL emulation.
typedef struct {
    bool enabled;
    bool use_backend;
    int bitness;
    char backend[64];
    char status[256];
} EmuglConfig;

// Initialize an EmuglConfig instance based on the AVD's hardware properties
// and the command-line -gpu option, if any.
//
// |config| is the instance to initialize.
// |gpu_mode| is the value of the hw.gpu.mode hardware property, adjusted
// for the '-gpu <mode>' option, if present.
// |bitness| is the host bitness (0, 32 or 64).
// |no_window| is true if the '-no-window' emulator flag was used.
//
// Returns true on success, or false if there was an error (e.g. bad
// mode or option value), in which case the |status| field will contain
// a small error message.
bool emuglConfig_init(EmuglConfig* config,
                      const char* gpu_mode,
                      int bitness,
                      bool no_window);

// Setup GPU emulation according to a given |backend|.
// |bitness| is the host bitness, and can be 0 (autodetect), 32 or 64.
void emuglConfig_setupEnv(const EmuglConfig* config);

ANDROID_END_HEADER

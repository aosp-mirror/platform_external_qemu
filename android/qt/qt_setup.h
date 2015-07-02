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

#ifndef ANDROID_QT_QT_SETUP_H
#define ANDROID_QT_QT_SETUP_H

#include "android/utils/compiler.h"

#include <stdbool.h>

ANDROID_BEGIN_HEADER

// Setup environment variables to run the Qt-based UI programs.
// This function should be called from the top-level launcher and will
// modify the library search path when needed.
// |is64bit| is true to indicate that a 64-bit engine/launcher program
// will be launched, false for 32-bit ones.
// Return true in case of success, false if there is no corresponding
// Qt library directory.
bool androidQtSetupEnv(bool is64bit);

ANDROID_END_HEADER

#endif  // ANDROID_QT_QT_SETUP_H


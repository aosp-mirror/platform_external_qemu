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

#ifndef ANDROID_QT_QT_PATH_H
#define ANDROID_QT_QT_PATH_H

#include "android/base/String.h"

// Return the path to the emulator's Qt library directory,
// which holds Qt's shared libraries. This path is typically
// added to the system library search path and passed to
// QCoreApplication::setLibraryPaths().
android::base::String androidQtGetLibraryDir();

// Return the path to the emulator's Qt plugins directory,
// which holds Qt's plugins. It should be added to the
// QT_QPA_PLATFORM_PLUGIN_PATH environment variable.
android::base::String androidQtGetPluginsDir();

#endif  // ANDROID_QT_QT_PATH_H

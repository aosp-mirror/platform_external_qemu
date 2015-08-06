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

#include "android/base/system/System.h"

using namespace android::base;

// Return the path to the emulator's Qt library directory.
// This is the path to the emulator executable, with either
// "/lib/qt" or "/lib64/qt" appended, depending on the bitness
// of the current execution. (And with the proper '/' or '\'
// path separator.)
String androidQt_QtDir();

#endif  // ANDROID_QT_QT_PATH_H


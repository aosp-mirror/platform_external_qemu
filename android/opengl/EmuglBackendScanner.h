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

#ifndef ANDROID_OPENG_EMUGL_BACKEND_SCANNER_H
#define ANDROID_OPENG_EMUGL_BACKEND_SCANNER_H

#include "android/base/containers/StringVector.h"

namespace android {
namespace opengl {

class EmuglBackendScanner {
public:
// Scan |execDir| for sub-directories named like <lib>/gles_<name> where
// <lib> is either 'lib' or 'lib64' depending on current host bitness,
// and <name> is an arbitrary names. Returns a vector of matching <name>
// strings, which will be empty on error.
// If |hostBitness| is 0, the this will use the current program
// bitness, otherwise the value must be either 32 or 64.
static ::android::base::StringVector scanDir(const char* execDir,
                                             int hostBitness = 0);

};

}  // namespace opengl
}  // namespace android

#endif  // ANDROID_OPENG_EMUGL_BACKEND_SCANNER_H

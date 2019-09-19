/* Copyright (C) 2007-2008 The Android Open Source Project
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

#include <iosfwd>                        // for string
#include <string>                        // for basic_string

#include "android/utils/bufprint.h"      // for bufprint, bufprint_temp_dir
#include "android/base/system/System.h"  // for System, System::kDirSeparator

using android::base::System;

// Implementation of bufprintf_xxx() functions that rely on
// android::base::System to make them mockable during unit-testing.

char* bufprint_temp_dir(char* buff, char* end) {
    return bufprint(buff, end, "%s", System::get()->getTempDir().c_str());
}

char* bufprint_temp_file(char* buff, char* end, const char* suffix) {
    return bufprint(buff, end, "%s%c%s", System::get()->getTempDir().c_str(),
                    System::kDirSeparator, suffix);
}

// Copyright 2014 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/base/misc/Utf8Utils.h"

namespace android {
namespace base {

bool utf8IsValid(const char* text, size_t textLen) {
    const char* end = text + textLen;

    while (text < end) {
        size_t n = 0;
        int ch = *text;
        if (!(ch & 0x80)) {
            n = 0;
        } else if ((ch & 0xe0) == 0xc0) {
            n = 1;
        } else if ((ch & 0xf0) == 0xe0) {
            n = 2;
        } else if ((ch & 0xf8) == 0xf0) {
            n = 3;
        } else if ((ch & 0xfc) == 0xf8) {
            n = 4;
        } else if ((ch & 0xfe) == 0xfc) {
            n = 5;
        } else {
            return false;
        }
        if ((size_t)(end - text) < n) {
            return false;
        }
        for (text++; n > 0; text++, n--) {
            if ((*text & 0xc0) != 0x80) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace base
}  // namespace android

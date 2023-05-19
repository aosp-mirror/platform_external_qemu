// Copyright 2017 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "aemu/base/ProcessControl.h"
#include "android/emulation/control/libui_agent.h"  // for LibuiKeyCodeSendFunc
#include "android/skin/charmap.h"                   // for skin_charmap_reve...
#include "android/skin/keycode-buffer.h"            // for skin_keycode_buff...
#include "android/skin/winsys.h"                    // for skin_winsys_quit_...
#include "android/skin/qt/qt-exports.h"


static int utf8_next(const unsigned char** pp, const unsigned char* end) {
    const unsigned char* p = *pp;
    int result = -1;

    if (p < end) {
        int c = *p++;
        if (c >= 128) {
            if ((c & 0xe0) == 0xc0)
                c &= 0x1f;
            else if ((c & 0xf0) == 0xe0)
                c &= 0x0f;
            else
                c &= 0x07;

            while (p < end && (p[0] & 0xc0) == 0x80) {
                c = (c << 6) | (p[0] & 0x3f);
                p++;
            }
        }
        result = c;
        *pp = p;
    }
    return result;
}

static const QAndroidLibuiAgent kLibuiAgent = {
        // convertUtf8ToKeyCodeEvents
        [](const unsigned char* text,
           int len,
           LibuiKeyCodeSendFunc sendFunc,
           void* context) -> bool {
            const auto charmap = skin_charmap_get();
            if (!charmap) {
                return false;
            }

            SkinKeycodeBuffer keycodes;
            skin_keycode_buffer_init(&keycodes, (SkinKeyCodeFlushFunc)sendFunc);
            keycodes.context = context;

            const auto end = text + len;
            while (text < end) {
                const int c = utf8_next(&text, end);
                if (c <= 0)
                    break;

                skin_charmap_reverse_map_unicode(charmap, (unsigned)c, 1,
                                                 &keycodes);
                skin_charmap_reverse_map_unicode(charmap, (unsigned)c, 0,
                                                 &keycodes);
                skin_keycode_buffer_flush(&keycodes);
            }
            return true;
        },
        // requestExit
        [](int exitCode, const char* message) {
            // Unfortunately we don't have any way of passing code/message now.
            skin_winsys_quit_request();
        },
        // requestRestart
        [](int exitCode, const char* message) {
            android::base::restartEmulator();
        },
};

extern "C" const QAndroidLibuiAgent* const gQAndroidLibuiAgent = &kLibuiAgent;

AEMU_UI_API  const QAndroidLibuiAgent* const getQAndroidLibuiAgent() {
    return gQAndroidLibuiAgent;
}
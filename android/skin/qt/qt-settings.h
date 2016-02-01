/* Copyright (C) 2015-2016 The Android Open Source Project
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

// This file contains values that are used for saving and
// restoring user settings associated with the Qt-based UI,
// as well as standard error "type" strings to uniquely identify
// error messages shown to the user. These "types" are how Qt
// handles when the user unchecks the "show this message again" box.
#pragma once

namespace Ui {
    namespace Settings {

        const QString ORG_NAME("Android Open Source Project");
        const QString ORG_DOMAIN("android.com");
        const QString APP_NAME("Emulator");

        const QString SHOW_AVD_ARCH_WARNING("showAvdArchWarning");

        // Note: The "set/" here is because these settings are
        //       associated with "settings-page", not because
        //       we're in "namespace Settings".

        const QString ALLOW_KEYBOARD_GRAB("set/allow_keyboard_grab");
        const QString ALWAYS_ON_TOP("set/alwaysOnTop");
        const QString SAVE_PATH("set/savePath");
        const QString SDK_PATH("set/sdkPath");
        const QString UI_THEME("set/theme");

        const QString LOCATION_LATITUDE("loc/latitude");
        const QString LOCATION_LONGITUDE("loc/longitude");
        const QString LOCATION_ALTITUDE("loc/altitude");
        const QString LOCATION_PLAYBACK_FILE("loc/playback_file_path");
        const QString LOCATION_PLAYBACK_SPEED("loc/playback_speed");
    }

    namespace Errors {

        const QString DRAG_AND_DROP_AMBIGUOUS("DRAG_AND_DROP_AMBIGUOUS");

        const QString LOCATION_BAD_FILE("LOCATION_BAD_FILE");
        const QString LOCATION_TABLE_ERROR("LOCATION_TABLE_ERROR");

        const QString MULTITOUCH_NOT_ENABLED("MULTITOUCH_NOT_ENABLED");

        const QString SCREENSHOT_CAPTURE_FAILED("SCREENSHOT_CAPTURE_FAILED");
        const QString SCREENSHOT_INVALID_SAVE_LOCATION("SCREENSHOT_INVALID_SAVE_LOCATION");
        const QString SCREENSHOT_PROCESS_FAILED("SCREENSHOT_PROCESS_FAILED");
        const QString SCREENSHOT_PULL_FAILED("SCREENSHOT_PULL_FAILED");

        const QString SETTINGS_BAD_SAVE_LOCATION("SETTINGS_BAD_SAVE_LOCATION");

        const QString SKIN_ALLOCATION_FAILED("SKIN_ALLOCATION_FAILED");

        const QString SMS_EMPTY_MESSAGE("SMS_EMPTY_MESSAGE");
        const QString SMS_FROM_INVALID("SMS_FROM_INVALID");
        const QString SMS_INVALID_CHAR("SMS_INVALID_CHAR");
        const QString SMS_NO_MODEM("SMS_NO_MODEM");

        const QString TELEPHONY_CALL_FAILED("TELEPHONY_CALL_FAILED");
        const QString TELEPHONY_END_CALL_FAILED("TELEPHONY_END_CALL_FAILED");
        const QString TELEPHONY_HOLD_CALL_FAILED("TELEPHONY_HOLD_CALL_FAILED");

        const QString TOOL_APK_INSTALL_FAILED("TOOL_APK_INSTALL_FAILED");
        const QString TOOL_APK_INSTALL_PENDING("TOOL_APK_INSTALL_PENDING");
        const QString TOOL_FILE_COPY_FAILED("TOOL_FILE_COPY_FAILED");
        const QString TOOL_SDK_ROOT("TOOL_SDK_ROOT");
    }
}

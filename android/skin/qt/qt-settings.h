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
// restoring user settings associated with the Qt-based UI.
//
// Settings that apply generally to the Emulator should
// use the names here directly. For example,
//     settings.value(Ui::Settings::SHOW_AVD_ARCH_WARNING, true).toBool()
//
// Settings that apply only to the current AVD should use
// the name here prefixed with "<AVD name>/". For example,
//     QString key(android_hw->avd_name);
//     key += "/" + Ui::Settings::CELLULAR_STANDARD;
//     settings.value(key, ...

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

        const QString CELLULAR_DATA_STATUS("cell/cell_data_status");
        const QString CELLULAR_STANDARD("cell/cell_standard");
        const QString CELLULAR_VOICE_STATUS("cell/cell_voice_status");

        const QString LOCATION_LATITUDE("loc/latitude");
        const QString LOCATION_LONGITUDE("loc/longitude");
        const QString LOCATION_ALTITUDE("loc/altitude");
        const QString LOCATION_PLAYBACK_FILE("loc/playback_file_path");
        const QString LOCATION_PLAYBACK_SPEED("loc/playback_speed");
    }
}

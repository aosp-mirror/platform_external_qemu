/* Copyright (C) 2015 The Android Open Source Project
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

namespace Ui {
    namespace Settings {

        const QString ORG_NAME("Android Open Source Project");
        const QString ORG_DOMAIN("android.com");
        const QString APP_NAME("Emulator");

        // Note: The "set/" here is because these settings are
        //       associated with "ext-settings", not because
        //       we're in "namespace Settings".

        const QString SAVE_PATH("set/savePath");
        const QString UI_THEME("set/theme");
    }
}

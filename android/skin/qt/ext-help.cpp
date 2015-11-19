// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-window.h"

#include "android/android.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/globals.h"
#include "android/update-check/UpdateChecker.h"
#include "android/update-check/VersionExtractor.h"
#include "ui_extended.h"

#include <QDesktopServices>
#include <QUrl>

const QString DOCS_URL =
    "http://developer.android.com/tools/help/emulator.html";
const QString FILE_BUG_URL =
    "https://code.google.com/p/android/issues/entry?template=Android%20Emulator%20Bug";
const QString SEND_FEEDBACK_URL =
    "https://code.google.com/p/android/issues/entry?template=Emulator%20Feature%20Request";

void ExtendedWindow::initHelp()
{
    // Get the version of this code
    android::update_check::VersionExtractor vEx;

    android::base::Version curVersion = vEx.getCurrentVersion();
    android::base::String  verStr =
            curVersion.isValid() ? curVersion.toString() : "Unknown";

    mExtendedUi->help_versionBox->setPlainText(verStr.c_str());

    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    mExtendedUi->help_androidVersionBox->setPlainText(apiVersionString(apiLevel));

    // Show the ADB port number
    mExtendedUi->help_adbPortBox->setPlainText( QString::number(android_adb_port) );

    // Get latest version that is available on line

    char configPath[PATH_MAX];
    bufprint_config_path(configPath, configPath + sizeof(configPath));
    android::update_check::UpdateChecker upCheck(configPath);

}

void ExtendedWindow::on_help_docs_clicked() {
    QDesktopServices::openUrl(QUrl(DOCS_URL));
}

void ExtendedWindow::on_help_fileBug_clicked() {
    QDesktopServices::openUrl(QUrl(FILE_BUG_URL));
}

void ExtendedWindow::on_help_sendFeedback_clicked() {
    QDesktopServices::openUrl(QUrl(SEND_FEEDBACK_URL));
}

QString ExtendedWindow::apiVersionString(int apiVersion)
{
    // This information was taken from the SDK Manager:
    // Appearances & Behavior > System Settings > Android SDK > SDK Platforms
    switch (apiVersion) {
        case 15: return "4.0.3 (Ice Cream Sandwich) - API 15 (Rev 5)";
        case 16: return "4.1 (Jelly Bean) - API 16 (Rev 5)";
        case 17: return "4.2 (Jelly Bean) - API 17 (Rev 3)";
        case 18: return "4.3 (Jelly Bean) - API 18 (Rev 3)";
        case 19: return "4.4 (KitKat) - API 19 (Rev 4)";
        case 20: return "4.4 (KitKat Wear) - API 20 (Rev 2)";
        case 21: return "5.0 (Lollipop) - API 21 (Rev 2)";
        case 22: return "5.1 (Lollipop) - API 22 (Rev 2)";
        case 23: return "6.0 (Marshmallow) - API 23 (Rev 1)";

        default:
            if (apiVersion < 0 || apiVersion > 99) {
                return tr("Unknown API version");
            } else {
                return "API " + QString::number(apiVersion);
            }
    }
}

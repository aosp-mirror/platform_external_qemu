/* Copyright (C) 2018 The Android Open Source Project
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

#include "android/skin/qt/screen-mask.h"

#include "android/base/files/PathUtils.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/utils/aconfig-file.h"

#include <QDir>
#include <QImageReader>
#include <QString>

using android::base::PathUtils;
using android::emulation::AdbInterface;

namespace ScreenMask {

// These allow us to re-send an ADB command
static AdbInterface* sAdbInterface = nullptr;
static const char* sRoundedContentPadding = 0;
static int sRetryCount = 0;

static QImage sScreenMaskImage;

static void sendAdbPaddingCommand();

// Load the image of the mask and set it for use on the
// AVD's display
static void loadMaskImage(AConfig* config, char* skinDir, char* skinName) {
    // Get the mask itself. The layout has the file name as
    // parts/portrait/foreground/mask.

    const char* maskFilename = aconfig_str(config, "mask", 0);
    if (!maskFilename || maskFilename[0] == '\0') {
        return;
    }

    QString maskPath = PathUtils::join(skinDir, skinName, maskFilename).c_str();

    // Read and decode this file
    QImageReader imageReader(maskPath);
    sScreenMaskImage = imageReader.read();
    if (sScreenMaskImage.isNull()) {
        return;
    }
    emulator_window_set_screen_mask(sScreenMaskImage.width(),
                                    sScreenMaskImage.height(),
                                    sScreenMaskImage.bits());
}

// Receive the results of the ADB command to set the padding value.
// Re-try if the command failed.
static void handleAdbResult(const android::emulation::OptionalAdbCommandResult& result) {

    if (++sRetryCount > 50) {
        // Too many failures. Give up.
        return;
    }

    if (!result || (result->exit_code != 0)) {
        // The command failed. Try again.
        sendAdbPaddingCommand();
    }
}

// Send the ADB command to set the padding value.
// The command is run asynchronously, so this function
// returns immediately.
static void sendAdbPaddingCommand() {
    // Use ADB to send this value to the device:
    // "adb shell settings put secure sysui_rounded_content_padding <nn>"

    if (!sAdbInterface) {
        return;
    }

    sAdbInterface->runAdbCommand(
                {"shell", "settings", "put", "secure",
                 "sysui_rounded_content_padding", sRoundedContentPadding },
                 handleAdbResult, 5000);
}

// Get the padding value and apply it to the device
static void setPadding(AConfig* config) {

    // If the AVD display has rounded corners, the items on the
    // top line of the device display need to be moved in, away
    // from the sides. Get the amount that they should be moved,
    // and send that number to the device. The layout has this
    // number as parts/portrait/foreground/padding

    sRoundedContentPadding = aconfig_str(config, "padding", 0);
    if (sRoundedContentPadding && sRoundedContentPadding[0] != '\0') {
        sendAdbPaddingCommand();
    }
}

// Handle the screen mask. This includes the mask image itself
// and any associated padding offset.
void loadMask(AdbInterface* adbInterface) {
    char* skinName;
    char* skinDir;

    sAdbInterface = adbInterface; // We'll need this later

    avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
    QString layoutPath = PathUtils::join(skinDir, skinName, "layout").c_str();
    AConfig* rootConfig = aconfig_node("", "");
    aconfig_load_file(rootConfig, layoutPath.toStdString().c_str());

    // Look for parts/portrait/foreground

    AConfig* nextConfig = aconfig_find(rootConfig, "parts");
    if (nextConfig == NULL) {
        return;
    }
    nextConfig = aconfig_find(nextConfig, "portrait");
    if (nextConfig == NULL) {
        return;
    }
    AConfig* foregroundConfig = aconfig_find(nextConfig, "foreground");

    if (foregroundConfig != NULL) {
        setPadding(foregroundConfig);
        loadMaskImage(foregroundConfig, skinDir, skinName);
    }
}

}

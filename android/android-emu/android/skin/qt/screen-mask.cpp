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
#include "android/base/memory/LazyInstance.h"
#include "android/emulator-window.h"
#include "android/globals.h"
#include "android/utils/aconfig-file.h"

#include <QDir>
#include <QImageReader>
#include <QString>

using android::base::LazyInstance;
using android::base::PathUtils;
using android::emulation::AdbInterface;

struct ScreenMaskGlobals {
    // These allow us to re-send an ADB command
    AdbInterface* adbInterface = nullptr;
    int           adbRetryCountdown = 0;

    QString       cutoutType;
    QString       cornerOverlayType;
    const char*   roundedContentPadding = 0;
    bool          sendPadding = false;
    bool          sendCornerOverlay = false;
    bool          sendCutout = false;

    QImage        screenMaskImage;
};

static LazyInstance<ScreenMaskGlobals> sGlobals = LAZY_INSTANCE_INIT;

namespace ScreenMask {

static constexpr int INITIAL_ADB_RETRY_LIMIT = 50;   // Max # retries for first ADB request
static constexpr int SUBSEQUENT_ADB_RETRY_LIMIT = 5; // Max # retries for subsquent ADB requests

static void sendAdbPaddingCommand();
static void sendAdbCornerOverlayCommand();
static void sendAdbCutoutCommand();

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
    sGlobals->screenMaskImage = imageReader.read();
    if (sGlobals->screenMaskImage.isNull()) {
        return;
    }
    emulator_window_set_screen_mask(sGlobals->screenMaskImage.width(),
                                    sGlobals->screenMaskImage.height(),
                                    sGlobals->screenMaskImage.bits());
}

// Receive the results of the ADB command.
// Either retry the current command, send the next
// command, or exit.
static void handleAdbResult(const android::emulation::OptionalAdbCommandResult& result) {
    if (--sGlobals->adbRetryCountdown <= 0 ||
            (result && (result->exit_code == 0)))
    {
        // Time out or success for this command.
        // Either way, retire this command.
        if (sGlobals->sendPadding) {
            sGlobals->sendPadding = false;
        } else if (sGlobals->sendCornerOverlay) {
            sGlobals->sendCornerOverlay = false;
        } else if (sGlobals->sendCutout) {
            sGlobals->sendCutout = false;
        }
        sGlobals->adbRetryCountdown = SUBSEQUENT_ADB_RETRY_LIMIT;
    }

    // Retry, continue with the next command, or just exit
    if (sGlobals->sendPadding) {
        sendAdbPaddingCommand();
    } else if (sGlobals->sendCornerOverlay) {
        sendAdbCornerOverlayCommand();
    } else if (sGlobals->sendCutout) {
        sendAdbCutoutCommand();
    }
}

// Send the ADB command to set the padding value.
// The command is run asynchronously, so this function
// returns immediately.
static void sendAdbPaddingCommand() {
    // Use ADB to send this value to the device:
    // "adb shell settings put secure sysui_rounded_content_padding <nn>"

    if (!sGlobals->adbInterface) {
        return;
    }

    sGlobals->adbInterface->runAdbCommand(
                {"shell", "settings", "put", "secure",
                 "sysui_rounded_content_padding", sGlobals->roundedContentPadding },
                 handleAdbResult, 5000);
}

static void setPaddingAndCutout(AConfig* config) {
    sGlobals->sendPadding = false;
    sGlobals->sendCornerOverlay = false;
    sGlobals->sendCutout = false;

    // Padding
    // If the AVD display has rounded corners, the items on the
    // top line of the device display need to be moved in, away
    // from the sides.
    //
    // Pading for Oreo:
    // Get the amount that the icons should be moved, and send
    // that number to the device. The layout has this number as
    // parts/portrait/foreground/padding

    sGlobals->roundedContentPadding = aconfig_str(config, "padding", 0);
    sGlobals->sendPadding = (sGlobals->roundedContentPadding && sGlobals->roundedContentPadding[0] != '\0');

    // Padding for P and later:
    // The padding value is set by selecting a resource overlay. The
    // layout has the overlay name as parts/portrait/foreground/corner.
    const char* cornerKeyword = aconfig_str(config, "corner", 0);
    if (cornerKeyword && cornerKeyword[0] != '\0') {
        sGlobals->cornerOverlayType = "com.android.internal.display.corner.emulation.";
        sGlobals->cornerOverlayType += cornerKeyword;
        sGlobals->sendCornerOverlay = true;
    }

    // Cutout
    // If the AVD display has a cutout, send a command to the device
    // to have it adjust the screen layout appropriately. The layout
    // has the emulated cutout name as parts/portrait/foreground/cutout.
    const char* cutoutKeyword = aconfig_str(config, "cutout", 0);
    if (cutoutKeyword && cutoutKeyword[0] != '\0') {
        sGlobals->cutoutType = "com.android.internal.display.cutout.emulation.";
        sGlobals->cutoutType += cutoutKeyword;
        sGlobals->sendCutout = true;
    }

    // Start the ADB command process
    sGlobals->adbRetryCountdown = INITIAL_ADB_RETRY_LIMIT;
    if (sGlobals->sendPadding) {
        sendAdbPaddingCommand();
    } else if (sGlobals->sendCornerOverlay) {
        sendAdbCornerOverlayCommand();
    } else if (sGlobals->sendCutout) {
        sendAdbCutoutCommand();
    }
}

// Send the ADB command to set the display cutout.
// The command is run asynchronously, so this function
// returns immediately.
static void sendAdbCutoutCommand() {
    // Use ADB to send this value to the device:
    // "adb shell cmd overlay enable-exclusive
    //                --category com.android.internal.display.cutout.emulation.<type>"

    if (!sGlobals->adbInterface) {
        return;
    }
    sGlobals->adbInterface->runAdbCommand(
                {"shell", "cmd", "overlay", "enable-exclusive",
                 "--category", sGlobals->cutoutType.toStdString().c_str()},
                 handleAdbResult, 5000);
}

// Send the ADB command to set the corner padding
// using an overlay. This technique is required
// for P+ images.
// The command is run asynchronously, so this function
// returns immediately.
static void sendAdbCornerOverlayCommand() {
    // Use ADB to send this value to the device:
    // "adb shell cmd overlay enable-exclusive
    //                --category com.android.internal.display.corner.emulation.<type>"

    if (!sGlobals->adbInterface) {
        return;
    }
    sGlobals->adbInterface->runAdbCommand(
                {"shell", "cmd", "overlay", "enable-exclusive",
                 "--category", sGlobals->cornerOverlayType.toStdString().c_str()},
                 handleAdbResult, 5000);
}

// Handle the screen mask. This includes the mask image itself
// and any associated cutout and padding offset.
void loadMask(AdbInterface* adbInterface) {
    char* skinName;
    char* skinDir;

    sGlobals->adbInterface = adbInterface; // We'll need this later

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
        setPaddingAndCutout(foregroundConfig);
        loadMaskImage(foregroundConfig, skinDir, skinName);
    }
}

}

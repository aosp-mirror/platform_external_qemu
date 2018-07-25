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
#include <QQueue>
#include <QString>

using android::base::LazyInstance;
using android::base::PathUtils;
using android::emulation::AdbInterface;

struct ScreenMaskGlobals {
    // These allow us to re-send an ADB command
    AdbInterface*        adbInterface = nullptr;
    int                  adbRetryCountdown = 0;

    QString              cutoutType;
    QString              cornerOverlayType;
    const char*          roundedContentPadding = 0;
    android::base::Lock  adbLock;
    QImage               screenMaskImage;

    QQueue<std::function<void()>> funcQueue;
};

static LazyInstance<ScreenMaskGlobals> sGlobals = LAZY_INSTANCE_INIT;

namespace ScreenMask {

static constexpr int INITIAL_ADB_RETRY_LIMIT = 50;   // Max # retries for first ADB request
static constexpr int SUBSEQUENT_ADB_RETRY_LIMIT = 5; // Max # retries for subsquent ADB requests

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
// Either retry the current command, send the next command, or exit.
//
// adbLock should be acquired before sending the first ADB command. The
// lock will remain held until this handler releases it.
static void handleAdbResult(const android::emulation::OptionalAdbCommandResult& result) {
    if (--sGlobals->adbRetryCountdown <= 0 ||
            (result && (result->exit_code == 0)))
    {
        // Time out or success for this command.
        // Either way, retire this command.
        (void)sGlobals->funcQueue.dequeue();
        if (sGlobals->funcQueue.isEmpty()) {
            sGlobals->adbLock.unlock();
            return;
        }
        sGlobals->adbRetryCountdown = SUBSEQUENT_ADB_RETRY_LIMIT;
    }
    // Invoke the function at the head of the queue.
    (sGlobals->funcQueue.head())();
}

static void setPaddingAndCutout(AConfig* config) {
    // Grab the lock and hold it until we've emptied 'funcQueue'
    sGlobals->adbLock.lock();
    sGlobals->funcQueue.clear();

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
    if (sGlobals->roundedContentPadding && sGlobals->roundedContentPadding[0] != '\0') {
        sGlobals->funcQueue.enqueue(
            [paddingValue = sGlobals->roundedContentPadding]() {
                // Use ADB to send the padding value to the device:
                // "adb shell settings put secure sysui_rounded_content_padding <nn>"
                if (!sGlobals->adbInterface) {
                    return;
                }
                sGlobals->adbInterface->runAdbCommand(
                            { "shell", "settings", "put", "secure",
                              "sysui_rounded_content_padding",
                              paddingValue },
                            handleAdbResult, 5000);
            });
    }

    // Padding for P and later:
    // The padding value is set by selecting a resource overlay. The
    // layout has the overlay name as parts/portrait/foreground/corner.
    const char* cornerKeyword = aconfig_str(config, "corner", 0);
    if (cornerKeyword && cornerKeyword[0] != '\0') {
        sGlobals->cornerOverlayType = "com.android.internal.display.corner.emulation.";
        sGlobals->cornerOverlayType += cornerKeyword;
        sGlobals->funcQueue.enqueue(
            [overlayString = sGlobals->cornerOverlayType.toStdString()]()
            {
                // Use ADB to enable a corner overlay on the device:
                // "adb shell cmd overlay enable-exclusive
                //         --category com.android.internal.display.corner.emulation.<type>"
                if (!sGlobals->adbInterface) {
                    return;
                }
                sGlobals->adbInterface->runAdbCommand(
                            { "shell", "cmd", "overlay",
                              "enable-exclusive", "--category",
                              overlayString.c_str() },
                            handleAdbResult, 5000);
            });
    }

    // Cutout
    // If the AVD display has a cutout, send a command to the device
    // to have it adjust the screen layout appropriately. The layout
    // has the emulated cutout name as parts/portrait/foreground/cutout.
    const char* cutoutKeyword = aconfig_str(config, "cutout", 0);
    if (cutoutKeyword && cutoutKeyword[0] != '\0') {
        sGlobals->cutoutType = "com.android.internal.display.cutout.emulation.";
        sGlobals->cutoutType += cutoutKeyword;
        sGlobals->funcQueue.enqueue(
            [cutoutString = sGlobals->cutoutType.toStdString()]() {
                // Use ADB to enable a cutout overlay on the device:
                // "adb shell cmd overlay enable-exclusive
                //        --category com.android.internal.display.cutout.emulation.<type>"
                if (!sGlobals->adbInterface) {
                    return;
                }
                sGlobals->adbInterface->runAdbCommand(
                            { "shell", "cmd", "overlay",
                              "enable-exclusive", "--category",
                              cutoutString.c_str() },
                            handleAdbResult, 5000);
            });
    }
    if (sGlobals->funcQueue.isEmpty()) {
        sGlobals->adbLock.unlock();
    } else {
        // Start the ADB command process
        sGlobals->adbRetryCountdown = INITIAL_ADB_RETRY_LIMIT;
        (sGlobals->funcQueue.head())();
    }
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

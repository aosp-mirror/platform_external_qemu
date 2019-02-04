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
    QImage screenMaskImage;
};

static LazyInstance<ScreenMaskGlobals> sGlobals = LAZY_INSTANCE_INIT;

namespace ScreenMask {

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

static void setPaddingAndCutout(AdbInterface* adbInterface, AConfig* config) {
    // Padding
    // If the AVD display has rounded corners, the items on the
    // top line of the device display need to be moved in, away
    // from the sides.
    //
    // Pading for Oreo:
    // Get the amount that the icons should be moved, and send
    // that number to the device. The layout has this number as
    // parts/portrait/foreground/padding
    const char* roundedContentPadding = aconfig_str(config, "padding", 0);
    if (roundedContentPadding && roundedContentPadding[0] != '\0') {
        adbInterface->enqueueCommand({ "shell", "settings", "put", "secure",
                                       "sysui_rounded_content_padding",
                                       roundedContentPadding });
    }

    // Padding for P and later:
    // The padding value is set by selecting a resource overlay. The
    // layout has the overlay name as parts/portrait/foreground/corner.
    const char* cornerKeyword = aconfig_str(config, "corner", 0);
    if (cornerKeyword && cornerKeyword[0] != '\0') {
        QString cornerOverlayType = "com.android.internal.display.corner.emulation.";
        cornerOverlayType += cornerKeyword;

        adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                       "enable-exclusive", "--category",
                                       cornerOverlayType.toStdString().c_str() });
    }

    // Cutout
    // If the AVD display has a cutout, send a command to the device
    // to have it adjust the screen layout appropriately. The layout
    // has the emulated cutout name as parts/portrait/foreground/cutout.
    const char* cutoutKeyword = aconfig_str(config, "cutout", 0);
    if (cutoutKeyword && cutoutKeyword[0] != '\0') {
        QString cutoutType = "com.android.internal.display.cutout.emulation.";
        cutoutType += cutoutKeyword;

        adbInterface->enqueueCommand({ "shell", "cmd", "overlay",
                                       "enable-exclusive", "--category",
                                       cutoutType.toStdString().c_str() });
    }
}

// Handle the screen mask. This includes the mask image itself
// and any associated cutout and padding offset.
void loadMask(AdbInterface* adbInterface) {
    char* skinName;
    char* skinDir;

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
        setPaddingAndCutout(adbInterface, foregroundConfig);
        loadMaskImage(foregroundConfig, skinDir, skinName);
    }
}

}

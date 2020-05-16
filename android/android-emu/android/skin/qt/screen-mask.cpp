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

#include <stddef.h>                                  // for NULL
#include <QImage>                                    // for QImage
#include <QImageReader>                              // for QImageReader
#include <QString>                                   // for QString
#include <string>                                    // for basic_string

#include "android/avd/info.h"                        // for avdInfo_getSkinInfo
#include "android/base/files/PathUtils.h"            // for PathUtils
#include "android/base/memory/LazyInstance.h"        // for LazyInstance
#include "android/emulator-window.h"                 // for emulator_window_...
#include "android/globals.h"                         // for android_avdInfo
#include "android/utils/aconfig-file.h"              // for aconfig_str, aco...

using android::base::LazyInstance;
using android::base::PathUtils;

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

AConfig* getForegroundConfig() {
    char* skinName;
    char* skinDir;

    avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
    QString layoutPath = PathUtils::join(skinDir, skinName, "layout").c_str();
    AConfig* rootConfig = aconfig_node("", "");
    aconfig_load_file(rootConfig, layoutPath.toStdString().c_str());

    // Look for parts/portrait/foreground
    AConfig* nextConfig = aconfig_find(rootConfig, "parts");
    if (nextConfig == nullptr) {
        return nullptr;
    }
    nextConfig = aconfig_find(nextConfig, "portrait");
    if (nextConfig == nullptr) {
        return nullptr;
    }

    return aconfig_find(nextConfig, "foreground");
}

// Handle the screen mask. This includes the mask image itself
// and any associated cutout and padding offset.
void loadMask() {
    char* skinName;
    char* skinDir;

    avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
    AConfig* foregroundConfig = getForegroundConfig();
    if (foregroundConfig != nullptr) {
        loadMaskImage(foregroundConfig, skinDir, skinName);
    }
}

} // namespace ScreenMask

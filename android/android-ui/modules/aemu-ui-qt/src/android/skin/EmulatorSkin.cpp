/* Copyright (C) 2022 The Android Open Source Project
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

#include "android/skin/EmulatorSkin.h"
#include <string.h>
#include <QDir>
#include <QImage>
#include <QRgb>
#include <QString>
#include <string>

#include "aemu/base/files/PathUtils.h"
#include "aemu/base/memory/LazyInstance.h"
#include "android/avd/info.h"
#include "android/console.h"
#include "android/utils/aconfig-file.h"

using android::base::LazyInstance;
using android::base::PathUtils;

static LazyInstance<EmulatorSkin> sEmulatorSkin = LAZY_INSTANCE_INIT;

void EmulatorSkin::reset() {
    mRawSkinPixmap.reset();
    getSkinPixmap();
    if (mRawSkinPixmap == nullptr) {
        int displayX = getConsoleAgents()->settings->hw()->hw_lcd_width;
        int displayY = getConsoleAgents()->settings->hw()->hw_lcd_height;
        mSkinPixmapIsPortrait = displayX <= displayY;
    }
}

EmulatorSkin::EmulatorSkin() {
    reset();
}

EmulatorSkin* EmulatorSkin::getInstance() {
    return sEmulatorSkin.ptr();
}

std::shared_ptr<QPixmap> EmulatorSkin::getSkinPixmap() {
    if (mRawSkinPixmap != nullptr) {
        // Already exists
        return mRawSkinPixmap;
    }

    // We need to read the skin image.
    // Where is the skin?
    char* skinName = nullptr;
    char* skinDir = nullptr;
    avdInfo_getSkinInfo(getConsoleAgents()->settings->avdInfo(), &skinName,
                        &skinDir);
    if (!skinName || !skinDir) {
        return nullptr;
    }
    // Parse the 'layout' file in the skin directory
    QString layoutPath = PathUtils::join(skinDir, skinName, "layout").c_str();
    AConfig* skinConfig = aconfig_node("", "");
    aconfig_load_file(skinConfig, layoutPath.toStdString().c_str());
    // Find the first instance of parts/*/background/image
    // ("*" will be either "portrait" or "landscape")
    AConfig* partsConfig = aconfig_find(skinConfig, "parts");
    if (partsConfig == nullptr) {
        return nullptr;  // Failed
    }
    const char* skinFileName = nullptr;
    for (AConfig* partNode = partsConfig->first_child; partNode != NULL;
         partNode = partNode->next) {
        const AConfig* backgroundNode = aconfig_find(partNode, "background");
        if (backgroundNode == NULL)
            continue;
        skinFileName = aconfig_str(backgroundNode, "image", nullptr);
        if (skinFileName != nullptr && skinFileName[0] != '\0') {
            // note, part name is not reliable way to tell the orientation
            // need to check actual size
            // mSkinPixmapIsPortrait = !strcmp(partNode->name, "portrait");
            break;
        }
    }
    if (skinFileName == nullptr || skinFileName[0] == '\0') {
        return nullptr;  // Failed
    }

    QString skinPath = QString(skinDir) + QDir::separator() + skinName +
                       QDir::separator() + skinFileName;
    // Emulator UI hides the border of the skin image (frameless style),
    // which is done by mask off the transparent pixels of the skin image.
    // But from pixel4_a, the skin image also sets transparent pixels for
    // the display. To avoid masking off the display, we replace the alpha
    // vaule of these pixels as 255 (opage).
    QImage img(skinPath);
    mSkinPixmapIsPortrait = img.width() <= img.height();
    for (int row = 0; row < img.height(); row++) {
        int left = -1, right = -1;
        for (int col = 0; col < img.width(); col++) {
            if (qAlpha(img.pixel(col, row)) != 0) {
                left = col;
                break;
            }
        }
        for (int col = img.width(); col >= 0; col--) {
            if (qAlpha(img.pixel(col, row)) != 0) {
                right = col;
                break;
            }
        }
        if (left == -1 || right == -1 || left == right) {
            continue;
        }
        for (int col = left; col <= right; col++) {
            QRgb pixel = img.pixel(col, row);
            if (qAlpha(pixel) == 0) {
                img.setPixel(
                        col, row,
                        qRgba(qRed(pixel), qGreen(pixel), qBlue(pixel), 255));
            }
        }
    }
    mRawSkinPixmap = std::make_shared<QPixmap>(QPixmap::fromImage(img));
    return mRawSkinPixmap;
}

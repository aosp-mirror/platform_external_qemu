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

#include "android/avd/info.h"
#include "android/globals.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "ui_extended.h"

#include <QtWidgets>

void ExtendedWindow::initSd()
{
    char* cardPath = NULL;

    // Do we have an SD card?
    if (android_avdInfo) {
        cardPath = avdInfo_getSdCardPath(android_avdInfo);
    }

    if (cardPath && cardPath[0] != '\0') {

        // TODO: Need to use the ADB directory service to get the correct QFileSystemModel
        QFileSystemModel *myModel = new QFileSystemModel;
        myModel->setRootPath(QDir::currentPath());

        mExtendedUi->sd_treeView->setModel(myModel);

        mExtendedUi->sd_treeView->setRootIndex(myModel->index(QDir::currentPath()));
    }
}

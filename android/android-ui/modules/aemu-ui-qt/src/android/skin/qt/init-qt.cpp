// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/init-qt.h"

#include <qglobal.h>                      // for Q_INIT_RESOURCE
#include <QCoreApplication>               // for QCoreApplication
#include <QFontDatabase>                  // for QFontDatabase

#include "android/skin/qt/qt-settings.h"  // for APP_NAME, ORG_DOMAIN, ORG_NAME
#include "android/utils/debug.h"          // for VERBOSE_PRINT, VERBOSE_init

namespace android {
namespace base {
class PathUtils;
class System;
}  // namespace base
}  // namespace android

using android::base::System;
using android::base::PathUtils;

void androidQtDefaultInit() {
    Q_INIT_RESOURCE(static_resources);
    Q_INIT_RESOURCE(resources);

    // Give Qt the fonts from our resource file
    int fontId = QFontDatabase::addApplicationFont(":/lib/fonts/Roboto");
    if (fontId < 0) {
        VERBOSE_PRINT(init,
                      "Count not load font resource: \":/lib/fonts/Roboto");
    }
    fontId = QFontDatabase::addApplicationFont(":/lib/fonts/Roboto-Bold");
    if (fontId < 0) {
        VERBOSE_PRINT(
                init,
                "Count not load font resource: \":/lib/fonts/Roboto-Bold");
    }
    fontId = QFontDatabase::addApplicationFont(":/lib/fonts/Roboto-Medium");
    if (fontId < 0) {
        VERBOSE_PRINT(
                init,
                "Count not load font resource: \":/lib/fonts/Roboto-Medium");
    }

    // Setup the application properties
    QCoreApplication::setOrganizationName(Ui::Settings::ORG_NAME);
    QCoreApplication::setOrganizationDomain(Ui::Settings::ORG_DOMAIN);
    QCoreApplication::setApplicationName(Ui::Settings::APP_NAME);
}

// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/multi-display-page.h"

#include "android/globals.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include <QSettings>
#include <string>

MultiDisplayPage::MultiDisplayPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::MultiDisplayPage()) {
    mUi->setupUi(this);
    char* skinName;
    char* skinDir;
    avdInfo_getSkinInfo(android_avdInfo, &skinName, &skinDir);
    std::string defaultDisplayDisp = std::string(skinName) +
                                     " (" +
                                     std::to_string(android_hw->hw_lcd_width) +
                                     'x' +
                                     std::to_string(android_hw->hw_lcd_height) +
                                     ')';
    mUi->defaultDisplayText->setText(defaultDisplayDisp.c_str());
}

void MultiDisplayPage::updateTheme() {
    mUi->addSecondaryDisplay->setIcon(getIconForCurrentTheme("add_display"));
}

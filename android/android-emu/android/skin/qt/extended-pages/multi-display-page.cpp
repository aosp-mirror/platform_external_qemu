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
#include "android/skin/qt/extended-pages/multi-display-item.h"
#include "android/skin/qt/qt-settings.h"
#include <QSettings>
#include <QGroupBox>
#include <QLabel>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <string>

int MultiDisplayPage::sMaxItem = 3;

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
    mItem.resize(sMaxItem + 1);
}

void MultiDisplayPage::updateTheme() {
    mUi->addSecondaryDisplay->setIcon(getIconForCurrentTheme("add_display"));
}

int MultiDisplayPage::findNextItemIndex() {
    for (int i = 1; i <= sMaxItem; i++) {
        if (mItem[i] == nullptr) {
            return i;
        }
    }
    return -1;
}

void MultiDisplayPage::on_addSecondaryDisplay_clicked() {
    int i = findNextItemIndex();
    if (i < 1) {
        return;
    }
    MultiDisplayItem *item = new MultiDisplayItem(i);
    mUi->verticalLayout_5->insertWidget(0, item);
    connect(item, SIGNAL(deleteDisplay(int)), this, SLOT(deleteSecondaryDisplay(int)));
    mItem[i] = item;
}

void MultiDisplayPage::deleteSecondaryDisplay(int id) {
    printf("item %d\n", id);
    delete(mItem[id]);
    mItem[id] = nullptr;
}

// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/multi-display-page.h"

#include "android/base/LayoutResolver.h"
#include "android/base/Log.h"
#include "android/globals.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/multi-display-item.h"
#include "android/skin/qt/qt-settings.h"
#include <QSettings>
#include <QGroupBox>
#include <QLabel>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <string>
#include <unordered_map>

int MultiDisplayPage::sMaxItem = 3;

MultiDisplayPage::MultiDisplayPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::MultiDisplayPage()) {
    mUi->setupUi(this);
    char* skinDir;
    avdInfo_getSkinInfo(android_avdInfo, &mSkinName, &skinDir);
    std::string defaultDisplayDisp = std::string(mSkinName) +
                                     " (" +
                                     std::to_string(android_hw->hw_lcd_width) +
                                     'x' +
                                     std::to_string(android_hw->hw_lcd_height) +
                                     ')';
    mUi->defaultDisplayText->setText(defaultDisplayDisp.c_str());
    mItem.resize(sMaxItem + 1);
    setSecondaryDisplaysTitle(mSecondaryItemCount);

    uint32_t width, height;
    EmulatorQtWindow::getInstance()->getMonitorRect(&width, &height);
    if (width != 0 && height != 0) {
        m_monitorAspectRatio = (double) height / (double) width;
    }

    recomputeLayout();
}

void MultiDisplayPage::setSecondaryDisplaysTitle(int count) {
    std::string s = "Secondary displays (" + std::to_string(count) + ")";
    mUi->secondaryDisplays_title->setText(s.c_str());
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
    mUi->verticalLayout_5->insertWidget(i - 1, item);
    connect(item, SIGNAL(deleteDisplay(int)), this, SLOT(deleteSecondaryDisplay(int)));
    connect(item, SIGNAL(changeDisplay(int)), this, SLOT(changeSecondaryDisplay(int)));
    mItem[i] = item;
    setSecondaryDisplaysTitle(++mSecondaryItemCount);
    recomputeLayout();
}

void MultiDisplayPage::deleteSecondaryDisplay(int id) {
    LOG(INFO) << "deleteSecondaryDisplay " << id;
    delete(mItem[id]);
    mItem[id] = nullptr;
    setSecondaryDisplaysTitle(--mSecondaryItemCount);
    recomputeLayout();
}

void MultiDisplayPage::changeSecondaryDisplay(int id) {
    recomputeLayout();
}

void MultiDisplayPage::recomputeLayout() {
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> rectangles;
    std::unordered_map<uint32_t, std::string> names;
    rectangles[0] = std::make_pair(android_hw->hw_lcd_width,
                                   android_hw->hw_lcd_height);
    names[0] = std::string(mSkinName);
    for (int i = 1; i <= sMaxItem; i++) {
        if (mItem[i] != nullptr) {
            uint32_t width, height;
            mItem[i]->getValues(&width, &height, nullptr);
            rectangles[i] =
                std::make_pair(width, height);
            names[i] = mItem[i]->getName();
        }
    }
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> offsets =
        std::move(android::base::resolveLayout(rectangles, m_monitorAspectRatio));
    mUi->multiDisplayArrangement->setLayout(rectangles, offsets, names);
}


void MultiDisplayPage::on_applyChanges_clicked() {
    LOG(INFO) << "on_applyChanges_clicked";
    for (int i = 1; i <= sMaxItem; i++) {
        uint32_t width, height, dpi;
        bool enabled;
        EmulatorQtWindow::getInstance()->getMultiDisplay(i, nullptr, nullptr,
                                                         &width, &height,
                                                         &dpi, nullptr,
                                                         &enabled);
        if (mItem[i] != nullptr) {
            uint32_t w, h, d;
            mItem[i]->getValues(&w, &h, &d);
            if (w == width && h == height && d == dpi && enabled) {
                continue;
            }
            EmulatorQtWindow::getInstance()->switchMultiDisplay(true,
                                                                i,
                                                                -1,
                                                                -1,
                                                                w,
                                                                h,
                                                                dpi,
                                                                0);
        } else {
            if (enabled) {
                EmulatorQtWindow::getInstance()->switchMultiDisplay(false,
                                                                    i,
                                                                    0,
                                                                    0,
                                                                    720,
                                                                    1080,
                                                                    213,
                                                                    0);
            }
        }

    }
}

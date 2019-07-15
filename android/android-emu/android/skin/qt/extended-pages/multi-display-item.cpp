// Copyright (C) 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "multi-display-item.h"

#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"

std::vector<MultiDisplayItem::displayType> MultiDisplayItem::sDisplayTypes =
    {{"480p(720x480)", 720, 480, 142},
     {"720p(1280x720)", 1280, 720, 213},
     {"1080p(1920x1080)", 1920, 1080, 320},
     {"4K(3840x2160)", 3840, 2160, 320},
     {"4K upscaled(3840x2160)", 3840, 2160, 640},
     {"custom", 720, 480, 142},
    };

MultiDisplayItem::MultiDisplayItem(int id, QWidget* parent)
    : QWidget(parent), mUi(new Ui::MultiDisplayItem()), mId(id) {

    mUi->setupUi(this);

    // Initialize the drop-down menu
    for (auto const& type : sDisplayTypes) {
        mUi->selectDisplayType->addItem(type.name.c_str());
    }
    mUi->width->setMinValue(0);
    mUi->width->setMaxValue(2160);
    mUi->height->setMinValue(0);
    mUi->height->setMaxValue(4096);
    mUi->dpi->setMinValue(120);
    mUi->dpi->setMaxValue(640);

    mCurrentIndex = 0; /* 480p as default */
    mUi->selectDisplayType->setCurrentIndex(mCurrentIndex);
    setValues(mCurrentIndex);
    connect(mUi->selectDisplayType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onDisplayTypeChanged(int)));

    // hide the configuration box for resolution and dpi
    hideWidthHeightDpiBox(true);

    mUi->deleteDisplay->setIcon(getIconForCurrentTheme("close"));
}

void MultiDisplayItem::onDisplayTypeChanged(int index) {
    if (index == sDisplayTypes.size() - 1) {
        int32_t x, y;
        uint32_t w, h, dpi;
        EmulatorQtWindow::getInstance()->getMultiDisplay(mId,
                                                         NULL, NULL,
                                                         &w, &h, &dpi,
                                                         NULL, NULL);
        mUi->height->setValue(h);
        mUi->width->setValue(w);
        mUi->dpi->setValue(dpi);
        hideWidthHeightDpiBox(false);
    } else {
        hideWidthHeightDpiBox(true);
        setValues(index);
    }
    mCurrentIndex = index;
}

void MultiDisplayItem::on_deleteDisplay_clicked() {
    emit deleteDisplay(mId);
}

void MultiDisplayItem::setValues(int index) {
    printf("enter setvalues index %d\n", index);
    if (index >= sDisplayTypes.size()) {
        return;
    }
    mUi->width->setValue(sDisplayTypes[index].width);
    mUi->height->setValue(sDisplayTypes[index].height);
    mUi->dpi->setValue(sDisplayTypes[index].dpi);
//    printf("setValues %d %d %d %d %d %d\n",
//           sDisplayTypes[index].width,
//           sDisplayTypes[index].height,
//           sDisplayTypes[index].dpi,
//           (int)mUi->width->value(),
//           (int)mUi->height->value(),
//           (int)mUi->dpi->value());
}

void MultiDisplayItem::getValues(int* width, int* height, uint32_t* dpi) {
    if (width) {
        *width = (int)mUi->width->value();
    }
    if (height) {
        *height = (int)mUi->height->value();
    }
    if (dpi) {
        *dpi = (uint32_t)mUi->height->value();
    }
}

void MultiDisplayItem::hideWidthHeightDpiBox(bool hide) {
    mUi->height->setHidden(hide);
    mUi->heightTitle->setHidden(hide);
    mUi->width->setHidden(hide);
    mUi->widthTitle->setHidden(hide);
    mUi->dpi->setHidden(hide);
    mUi->dpiTitle->setHidden(hide);
}

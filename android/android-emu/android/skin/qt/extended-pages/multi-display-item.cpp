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

#include "android/skin/qt/extended-pages/common.h"

std::vector<MultiDisplayItem::displayType> MultiDisplayItem::sDisplayTypes =
    {{"480p(720x480)", 720, 480, 142},
     {"720p(1280x720)", 1280, 720, 213},
     {"1080p(1920x1080)", 1920, 1080, 320},
     {"4K(3840x2160)", 3840, 2160, 320},
     {"4K upscaled(3840x2160)", 3840, 2160, 640},
     {"custom", 1280, 720, 213},
    };

MultiDisplayItem::MultiDisplayItem(int id, QWidget* parent)
    : QWidget(parent), mUi(new Ui::MultiDisplayItem()), mId(id) {

    mUi->setupUi(this);

    // Initialize the drop-down menu
    for (auto const& type : sDisplayTypes) {
        mUi->selectDisplayType->addItem(type.name.c_str());
    }
    mUi->selectDisplayType->setCurrentIndex(2); /* 1080p as default */
    connect(mUi->selectDisplayType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onDisplayTypeChanged(int)));

    // hide the configuration box for resolution and dpi
    hideWidthHeightDpiBox(true);

    mUi->deleteDisplay->setIcon(getIconForCurrentTheme("close"));
}

void MultiDisplayItem::onDisplayTypeChanged(int index) {
    if (index == sDisplayTypes.size() - 1) {
        hideWidthHeightDpiBox(false);
    } else {
        hideWidthHeightDpiBox(true);
    }
}

void MultiDisplayItem::on_deleteDisplay_clicked() {
    emit deleteDisplay(mId);
}

void MultiDisplayItem::hideWidthHeightDpiBox(bool hide) {
    mUi->height->setHidden(hide);
    mUi->heightTitle->setHidden(hide);
    mUi->width->setHidden(hide);
    mUi->widthTitle->setHidden(hide);
    mUi->dpi->setHidden(hide);
    mUi->dpiTitle->setHidden(hide);
}

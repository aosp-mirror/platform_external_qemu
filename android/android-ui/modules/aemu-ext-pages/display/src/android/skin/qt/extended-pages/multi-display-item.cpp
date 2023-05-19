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

#include <stddef.h>  // for NULL

#include <QComboBox>    // for QComboBox
#include <QLabel>       // for QLabel
#include <QPushButton>  // for QPushButton
#include <QStyle>       // for QStyle

#include "android/console.h"
#include "android/skin/backend-defs.h"
#include "android/metrics/UiEventTracker.h"
#include "android/skin/qt/angle-input-widget.h"     // for AngleInputWidget
// #include "android/skin/qt/emulator-qt-window.h"     // for EmulatorQtWindow
#include "android/skin/qt/extended-pages/common.h"  // for getIconForCurrent...
#include "android/skin/backend-defs.h"
#include "multi-display-page.h"                     // for MultiDisplayPage
#include "android/avd/info.h"                       // for avdInfo_getAvdFlavor(...)

class QFocusEvent;
class QWidget;

// name, short name, height, width, density
std::vector<MultiDisplayItem::displayType> MultiDisplayItem::sDisplayTypes = {
        {"480p(720x480)", "480p", 720, 480, 142},
        {"720p(1280x720)", "720p", 1280, 720, 213},
        {"1080p(1920x1080)", "1080p", 1920, 1080, 320},
        {"4K(3840x2160)", "4K", 3840, 2160, 320},
        {"4K upscaled(3840x2160)", "4K\nupscaled", 3840, 2160, 640},
        {"custom", "custom", 1080, 720, 213},
};

std::vector<MultiDisplayItem::displayType> MultiDisplayItem::sDisplayTypesAutoCluster = {
        {"Cluster(400x600) portrait", "Cluster portrait", 600, 400, 120},
        {"Cluster(800x600) landscape", "Cluster landscape", 600, 800, 120},
        {"custom", "custom", 600, 400, 120},
};

std::vector<MultiDisplayItem::displayType> MultiDisplayItem::sDisplayTypesAutoSecondary = {
        {"768p(1024x768) landscape", "768p", 768, 1024, 120}, // TODO: figure out if 1024p or 768p
        {"1024p(1024x768) portrait", "1024p", 1024, 768, 120},
        {"1080p(1920x1080) landscape", "1080p", 1080, 1920, 320},
        {"4K(3840x2160) portrait", "4Kportrait", 3840, 2160, 320},
        {"4K(3840x2160) landscape", "4Klandscape", 2160, 3840, 320},
        {"custom", "custom", 1080, 720, 213},
};

MultiDisplayItem::MultiDisplayItem(int id, QWidget* parent)
    : QWidget(parent),
      mMultiDisplayPage(reinterpret_cast<MultiDisplayPage*>(parent)),
      mUi(new Ui::MultiDisplayItem()),
      mId(id),
      mDropDownTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::OPTION_SELECTED,
              android_studio::EmulatorUiEvent::EXTENDED_DISPLAYS_TAB)) {
    mUi->setupUi(this);
    this->setStyleSheet(parent->styleSheet());

    // Initialize the drop-down menu
    for (auto const& type : getDisplayTypes()) {
        mUi->selectDisplayType->addItem(type.name.c_str());
    }
    mUi->width->setMinValue(0);
    mUi->width->setMaxValue(7680);
    mUi->height->setMinValue(0);
    mUi->height->setMaxValue(7680);
    mUi->dpi->setMinValue(120);
    mUi->dpi->setMaxValue(640);

    mCurrentIndex = 1; /* 720p as default */
    mUi->selectDisplayType->setCurrentIndex(mCurrentIndex);
    setValues(mCurrentIndex);
    connect(mUi->selectDisplayType, SIGNAL(currentIndexChanged(int)), this,
            SLOT(onDisplayTypeChanged(int)));
    connect(this, SIGNAL(changeDisplay(int)), mMultiDisplayPage,
            SLOT(changeSecondaryDisplay(int)));
    connect(this, SIGNAL(deleteDisplay(int)), mMultiDisplayPage,
            SLOT(deleteSecondaryDisplay(int)));
    connect(mUi->height, SIGNAL(valueChanged(double)), this,
            SLOT(onCustomParaChanged(double)));
    connect(mUi->width, SIGNAL(valueChanged(double)), this,
            SLOT(onCustomParaChanged(double)));
    connect(mUi->dpi, SIGNAL(valueChanged(double)), this,
            SLOT(onCustomParaChanged(double)));
    std::string s = "Display " + std::to_string(mId);
    mUi->display_title->setText(s.c_str());
    mUi->deleteDisplay->setIcon(getIconForCurrentTheme("close"));

    // hide the configuration box for resolution and dpi
    hideWidthHeightDpiBox(true);
}

MultiDisplayItem::MultiDisplayItem(int id,
                                   uint32_t width,
                                   uint32_t height,
                                   uint32_t dpi,
                                   QWidget* parent)
    : MultiDisplayItem(id, parent) {
    // check with displayType fits the width, height and dpi, then set to it.
    // If not found, set to "custom" type.
    int i = 0;
    std::vector<MultiDisplayItem::displayType> displayTypes = getDisplayTypes();
    for (; i < displayTypes.size() - 1; i++) {
        if (displayTypes[i].width == width &&
            displayTypes[i].height == height && displayTypes[i].dpi == dpi) {
            break;
        }
    }
    mCurrentIndex = i;
    if (mUi->selectDisplayType->currentIndex() != mCurrentIndex) {
        mUi->selectDisplayType->setCurrentIndex(mCurrentIndex);
        // This selection is not made by the user, so we have to adjust our count.
        mDropDownTracker->increment(displayTypes[mCurrentIndex].shortName, -1);
        onDisplayTypeChanged(mCurrentIndex);
    }
}

void MultiDisplayItem::onDisplayTypeChanged(int index) {
    std::vector<MultiDisplayItem::displayType> displayTypes = getDisplayTypes();
    mDropDownTracker->increment(displayTypes[index].shortName);
    if (index == displayTypes.size() - 1) {
        uint32_t w, h, dpi;
        bool enabled;
        getConsoleAgents()->emu->getMultiDisplay(mId, NULL, NULL, &w, &h, &dpi,
                                                 NULL, &enabled);
        if (enabled) {
            mUi->height->setValue(h);
            mUi->width->setValue(w);
            mUi->dpi->setValue(dpi);
        } else {
            setValues(index);
        }
        hideWidthHeightDpiBox(false);
    } else {
        hideWidthHeightDpiBox(true);
        setValues(index);
    }
    mCurrentIndex = index;
    emit changeDisplay(mId);
}

void MultiDisplayItem::onCustomParaChanged(double /*value*/) {
    uint32_t w, h, d;
    getValues(&w, &h, &d);
    if (getConsoleAgents()->multi_display->multiDisplayParamValidate(mId, w, h,
                                                                     d, 0)) {
        emit changeDisplay(mId);
    }
}

void MultiDisplayItem::on_deleteDisplay_clicked() {
    emit deleteDisplay(mId);
}

void MultiDisplayItem::setValues(int index) {
    std::vector<MultiDisplayItem::displayType> displayTypes = getDisplayTypes();
    if (index >= displayTypes.size()) {
        return;
    }
    mUi->width->setValue(displayTypes[index].width);
    mUi->height->setValue(displayTypes[index].height);
    mUi->dpi->setValue(displayTypes[index].dpi);
}

void MultiDisplayItem::getValues(uint32_t* width,
                                 uint32_t* height,
                                 uint32_t* dpi) {
    if (width) {
        *width = (uint32_t)mUi->width->value();
    }
    if (height) {
        *height = (uint32_t)mUi->height->value();
    }
    if (dpi) {
        *dpi = (uint32_t)mUi->dpi->value();
    }
}

const std::string& MultiDisplayItem::getName() {
    return getDisplayTypes()[mCurrentIndex].shortName;
}

void MultiDisplayItem::hideWidthHeightDpiBox(bool hide) {
    mUi->height->setHidden(hide);
    mUi->heightTitle->setHidden(hide);
    mUi->width->setHidden(hide);
    mUi->widthTitle->setHidden(hide);
    mUi->dpi->setHidden(hide);
    mUi->dpiTitle->setHidden(hide);
}

void MultiDisplayItem::focusInEvent(QFocusEvent* event) {
    QWidget::focusInEvent(event);
    mMultiDisplayPage->setArrangementHighLightDisplay(mId);
    mUi->selectDisplayType->setProperty("ColorGroup", "focused");
    style()->polish(mUi->selectDisplayType);
}

void MultiDisplayItem::focusOutEvent(QFocusEvent* event) {
    QWidget::focusOutEvent(event);
    mMultiDisplayPage->setArrangementHighLightDisplay(-1);
    mUi->selectDisplayType->setProperty("ColorGroup", "");
    style()->polish(mUi->selectDisplayType);
}

void MultiDisplayItem::updateTheme() {
    mUi->deleteDisplay->setIcon(getIconForCurrentTheme("close"));
}

std::vector<MultiDisplayItem::displayType> const& MultiDisplayItem::getDisplayTypes() const {
    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_ANDROID_AUTO) {
        if (mId == 1) { // Cluster display have different recommended sizes.
            return sDisplayTypesAutoCluster;
        } else {
            return sDisplayTypesAutoSecondary;
        }
    } else {
        return sDisplayTypes;
    }
}

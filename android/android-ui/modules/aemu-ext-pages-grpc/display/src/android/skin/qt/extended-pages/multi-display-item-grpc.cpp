// Copyright 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "multi-display-item-grpc.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QVariant>

#include "aemu/base/logging/Log.h"
#include "android/avd/info.h"
#include "android/avd/util.h"
#include "android/console.h"
#include "android/metrics/UiEventTracker.h"
#include "android/skin/qt/angle-input-widget.h"
#include "android/skin/qt/extended-pages/common.h"
#include "host-common/window_agent.h"
#include "multi-display-page-grpc.h"
#include "studio_stats.pb.h"

#define MULTI_DISPLAY_DEBUG 0

/* set  >1 for very verbose debugging */
#if MULTI_DISPLAY_DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

// name, short name, height, width, density
std::vector<MultiDisplayItemGrpc::displayType>
        MultiDisplayItemGrpc::sDisplayTypes = {
                {"480p(720x480)", "480p", 720, 480, 142},
                {"720p(1280x720)", "720p", 1280, 720, 213},
                {"1080p(1920x1080)", "1080p", 1920, 1080, 320},
                {"4K(3840x2160)", "4K", 3840, 2160, 320},
                {"4K upscaled(3840x2160)", "4K\nupscaled", 3840, 2160, 640},
                {"custom", "custom", 1080, 720, 213},
};

std::vector<MultiDisplayItemGrpc::displayType>
        MultiDisplayItemGrpc::sDisplayTypesAutoCluster = {
                {"Cluster(400x600) portrait", "Cluster portrait", 600, 400,
                 120},
                {"Cluster(800x600) landscape", "Cluster landscape", 600, 800,
                 120},
                {"custom", "custom", 600, 400, 120},
};

std::vector<MultiDisplayItemGrpc::displayType>
        MultiDisplayItemGrpc::sDisplayTypesAutoSecondary = {
                {"768p(1024x768) landscape", "768p", 768, 1024,
                 120},  // TODO: figure out if 1024p or 768p
                {"1024p(1024x768) portrait", "1024p", 1024, 768, 120},
                {"1080p(1920x1080) landscape", "1080p", 1080, 1920, 320},
                {"4K(3840x2160) portrait", "4Kportrait", 3840, 2160, 320},
                {"4K(3840x2160) landscape", "4Klandscape", 2160, 3840, 320},
                {"custom", "custom", 1080, 720, 213},
};

MultiDisplayItemGrpc::MultiDisplayItemGrpc(int id, QWidget* parent)
    : QWidget(parent),
      mMultiDisplayPageGrpc(reinterpret_cast<MultiDisplayPageGrpc*>(parent)),
      mUi(new Ui::MultiDisplayItemGrpc()),
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
    connect(this, SIGNAL(changeDisplay(int)), mMultiDisplayPageGrpc,
            SLOT(changeSecondaryDisplay(int)));
    connect(this, SIGNAL(deleteDisplay(int)), mMultiDisplayPageGrpc,
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

MultiDisplayItemGrpc::MultiDisplayItemGrpc(int id,
                                           uint32_t width,
                                           uint32_t height,
                                           uint32_t dpi,
                                           QWidget* parent)
    : MultiDisplayItemGrpc(id, parent) {
    setValues(width, height, dpi);
}

void MultiDisplayItemGrpc::onDisplayTypeChanged(int index) {
    std::vector<MultiDisplayItemGrpc::displayType> displayTypes =
            getDisplayTypes();
    mDropDownTracker->increment(displayTypes[index].shortName);
    if (index == displayTypes.size() - 1) {
        hideWidthHeightDpiBox(false);
    } else {
        hideWidthHeightDpiBox(true);
        setValues(index);
    }
    mCurrentIndex = index;
    emit changeDisplay(mId);
}

void MultiDisplayItemGrpc::onCustomParaChanged(double /*value*/) {
    DD("onCustomParaChanged");
    uint32_t w, h, dpi;
    getValues(&w, &h, &dpi);

    // According the Android 9 CDD,
    // * 120 <= dpi <= 640
    // * 320 * (dpi / 160) <= width
    // * 320 * (dpi / 160) <= height
    //
    // Also we don't want a screen too big to limit the performance impact.
    // * 4K might be a good upper limit

    auto windowAgent = getConsoleAgents()->emu;
    if (dpi < 120 || dpi > 640) {
        windowAgent->showMessage("dpi should be between 120 and 640",
                                 WINDOW_MESSAGE_ERROR, 1000);
        derror("Display dpi should be between 120 and 640, not %d", dpi);
        return;
    }
    if (w < 320 * dpi / 160 || h < 320 * dpi / 160) {
        windowAgent->showMessage("width and height should be >= 320dp",
                                 WINDOW_MESSAGE_ERROR, 1000);
        derror("Display width and height should be >= 320dp, not %d", dpi);
        return;
    }
    if (!((w <= 7680 && h <= 4320) || (w <= 4320 && h <= 7680))) {
        windowAgent->showMessage("resolution should not exceed 8k (7680*4320)",
                                 WINDOW_MESSAGE_ERROR, 1000);
        derror("Display resolution should not exceed 8k (7680x4320) vs (%dx%d)",
               w, h);
        return;
    }

    emit changeDisplay(mId);
}

void MultiDisplayItemGrpc::on_deleteDisplay_clicked() {
    emit deleteDisplay(mId);
}

void MultiDisplayItemGrpc::setValues(int index) {
    DD("setValues: %d", index);
    std::vector<MultiDisplayItemGrpc::displayType> displayTypes =
            getDisplayTypes();
    if (index >= displayTypes.size()) {
        return;
    }

    mUi->width->setValue(displayTypes[index].width);
    mUi->height->setValue(displayTypes[index].height);
    mUi->dpi->setValue(displayTypes[index].dpi);
}

void MultiDisplayItemGrpc::setValues(uint32_t width,
                                     uint32_t height,
                                     uint32_t dpi) {
    DD("setValues: (%d, %d, %d)", width, height, dpi);
    mUi->width->setValue(width);
    mUi->height->setValue(height);
    mUi->dpi->setValue(dpi);

    // check with displayType fits the width, height and dpi, then set to it.
    // If not found, set to "custom" type.
    int i = 0;

    std::vector<MultiDisplayItemGrpc::displayType> displayTypes =
            getDisplayTypes();
    for (; i < displayTypes.size() - 1; i++) {
        if (displayTypes[i].width == width &&
            displayTypes[i].height == height && displayTypes[i].dpi == dpi) {
            break;
        }
    }
    mCurrentIndex = i;
    if (mUi->selectDisplayType->currentIndex() != mCurrentIndex) {
        mUi->selectDisplayType->setCurrentIndex(mCurrentIndex);
        // This selection is not made by the user, so we have to adjust our
        // count.
        mDropDownTracker->increment(displayTypes[mCurrentIndex].shortName, -1);

        onDisplayTypeChanged(mCurrentIndex);
    }
}

void MultiDisplayItemGrpc::getValues(uint32_t* width,
                                     uint32_t* height,
                                     uint32_t* dpi) {
    DD("getValues");
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

const std::string& MultiDisplayItemGrpc::getName() {
    return getDisplayTypes()[mCurrentIndex].shortName;
}

void MultiDisplayItemGrpc::hideWidthHeightDpiBox(bool hide) {
    DD("hideWidthHeightDpiBox: %d", hide);
    mUi->height->setHidden(hide);
    mUi->heightTitle->setHidden(hide);
    mUi->width->setHidden(hide);
    mUi->widthTitle->setHidden(hide);
    mUi->dpi->setHidden(hide);
    mUi->dpiTitle->setHidden(hide);
}

void MultiDisplayItemGrpc::focusInEvent(QFocusEvent* event) {
    DD("focusInEvent: %p", event);
    QWidget::focusInEvent(event);
    mMultiDisplayPageGrpc->setArrangementHighLightDisplay(mId);
    mUi->selectDisplayType->setProperty("ColorGroup", "focused");
    style()->polish(mUi->selectDisplayType);
}

void MultiDisplayItemGrpc::focusOutEvent(QFocusEvent* event) {
    DD("focusOutEvent: %p", event);
    QWidget::focusOutEvent(event);
    mMultiDisplayPageGrpc->setArrangementHighLightDisplay(-1);
    mUi->selectDisplayType->setProperty("ColorGroup", "");
    style()->polish(mUi->selectDisplayType);
}

void MultiDisplayItemGrpc::updateTheme() {
    DD("updateTheme");
    mUi->deleteDisplay->setIcon(getIconForCurrentTheme("close"));
}

std::vector<MultiDisplayItemGrpc::displayType> const&
MultiDisplayItemGrpc::getDisplayTypes() const {
    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) ==
        AVD_ANDROID_AUTO) {
        if (mId == 1) {  // Cluster display have different recommended sizes.
            return sDisplayTypesAutoCluster;
        } else {
            return sDisplayTypesAutoSecondary;
        }
    } else {
        return sDisplayTypes;
    }
}

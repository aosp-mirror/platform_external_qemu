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
#include "android/skin/qt/extended-pages/multi-display-page-grpc.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QSize>
#include <QVBoxLayout>
#include <QtCore>
#include <algorithm>
#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "aemu/base/LayoutResolver.h"
#include "android/avd/info.h"
#include "android/base/system/System.h"

#include "android/cmdline-option.h"
#include "android/console.h"
#include "android/emulation/control/globals_agent.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/UiEventTracker.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/multi-display-arrangement-grpc.h"
#include "android/skin/qt/extended-pages/multi-display-item-grpc.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/utils/system.h"
#include "emulator_controller.pb.h"
#include "studio_stats.pb.h"

// Allow at most 5 reports every 60 seconds.
static constexpr uint64_t kReportWindowDurationUs = 1000 * 1000 * 60;
static constexpr uint32_t kMaxReportsPerWindow = 5;

using android::emulation::control::DisplayConfiguration;
using android::emulation::control::DisplayConfigurations;
using android::emulation::control::EmulatorGrpcClient;
using android::emulation::control::Notification;

#define MULTI_DISPLAY_DEBUG 0

/* set  >1 for very verbose debugging */
#if MULTI_DISPLAY_DEBUG <= 1
#define DD(...) (void)0
#else
#define DD(...) dinfo(__VA_ARGS__)
#endif

MultiDisplayPageGrpc::MultiDisplayPageGrpc(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::MultiDisplayPageGrpc()),
      mServiceClient(EmulatorGrpcClient::me()),
      mApplyChangesTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::BUTTON_PRESS,
              android_studio::EmulatorUiEvent::EXTENDED_DISPLAYS_TAB)) {
    qRegisterMetaType<DisplayConfigurations>();
    mUi->setupUi(this);

    connect(this, SIGNAL(fireDisplayChangeNotification(DisplayConfigurations)),
            this, SLOT(handleDisplayChangeNotification(DisplayConfigurations)));

    // set default display title
    char* skinName;
    char* skinDir;
    avdInfo_getSkinInfo(getConsoleAgents()->settings->avdInfo(), &skinName,
                        &skinDir);
    mSkinName = skinName;
    AFREE(skinName);
    AFREE(skinDir);

    std::string defaultDisplayDisp =
            mSkinName + " (" +
            std::to_string(getConsoleAgents()->settings->hw()->hw_lcd_width) +
            'x' +
            std::to_string(getConsoleAgents()->settings->hw()->hw_lcd_height) +
            ')';
    mUi->defaultDisplayText->setText(defaultDisplayDisp.c_str());

    // Do not show layout panel when studio is active.
    if (getConsoleAgents()
                ->settings->android_cmdLineOptions()
                ->qt_hide_window) {
        mUi->horizontalLayout_2->removeItem(mUi->verticalLayout_3);
        mUi->multiDisplayArrangement->hide();
        mUi->arrangement->hide();
    }

    // set secondary display title
    setSecondaryDisplaysTitle(mItem.size());

    uint32_t width, height;
    EmulatorQtWindow::getInstance()->getMonitorRect(&width, &height);
    if (width != 0 && height != 0) {
        m_monitorAspectRatio = (double)height / (double)width;
    }

    mUi->verticalLayout_5->setAlignment(Qt::AlignTop);

    mServiceClient.getDisplayConfigurationsAsync([&](auto status) {
        if (status.ok()) {
            {
                DisplayConfigurations newConfig = *status.value();
                emit(fireDisplayChangeNotification(newConfig));
            };
        }
    });

    mServiceClient.registerNotificationListener(
            [&](const Notification* notification) {
                if (notification
                            ->has_displayconfigurationschangednotification()) {
                    {
                        DisplayConfigurations newConfig =
                                notification
                                        ->displayconfigurationschangednotification()
                                        .displayconfigurations();
                        emit(fireDisplayChangeNotification(newConfig));
                    };
                }
            },
            [](auto status) { /* ignored */ });
}

void MultiDisplayPageGrpc::setSecondaryDisplaysTitle(int count) {
    std::string s = "Secondary displays (" + std::to_string(count) + ")";
    mUi->secondaryDisplays_title->setText(s.c_str());
}

void MultiDisplayPageGrpc::handleDisplayChangeNotification(
        DisplayConfigurations config) {
    DD("handleDisplayChangeNotification: %s", config.DebugString().c_str());

    mMaxDisplays = config.userconfigurable();
    // Let's not recompute the layout due to individual items being added.
    mNoRecompute = true;
    mItem.clear();
    for (const auto& display : config.displays()) {
        if (display.display() == 0)
            continue;
        mItem[display.display()] = std::make_unique<MultiDisplayItemGrpc>(
                display.display(), display.width(), display.height(),
                display.dpi(), this);

        DD("add MultiDisplayItemGrpc %d and insert widget %dx%d (%d)",
           display.display(), display.width(), display.height(), display.dpi());
    }
    // Now we are going to add the elements to the layout in the right order.
    std::vector<int> keys;
    for (const auto& [key, ignored] : mItem) {
        keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end());

    for (int key : keys) {
        mUi->verticalLayout_5->addWidget(mItem[key].get());
    }

    setSecondaryDisplaysTitle(mItem.size());
    mNoRecompute = false;
    recomputeLayout();
}

void MultiDisplayPageGrpc::updateTheme(const QString& styleString) {
    auto icon = getIconForCurrentTheme("add_display");
    mUi->addSecondaryDisplay->setIcon(icon);
    mUi->addSecondaryDisplay->setIconSize(QSize(24, 24));
    mUi->scrollAreaWidgetContents->setStyleSheet(styleString);
    for (auto& [id, item] : mItem) {
        item->updateTheme();
    }
}

int MultiDisplayPageGrpc::findNextItemIndex() {
    for (int i = 1; i < mMaxDisplays; i++) {
        if (mItem.count(i) == 0)
            return i;
    }
    return -1;
}

void MultiDisplayPageGrpc::on_addSecondaryDisplay_clicked() {
    int i = findNextItemIndex();
    auto item = std::make_unique<MultiDisplayItemGrpc>(i, this);
    DD("add MultiDisplayItemGrpc: %d and insert widget", i);
    mUi->verticalLayout_5->insertWidget(i - 1, item.get());
    setSecondaryDisplaysTitle(mItem.size() + 1);
    recomputeLayout();
    item->setFocus();
    mItem[i] = std::move(item);
    if (findNextItemIndex() < 0) {
        mUi->addSecondaryDisplay->setEnabled(false);
    }
}

void MultiDisplayPageGrpc::deleteSecondaryDisplay(int id) {
    DD("Deleting %d", id);
    mItem.erase(id);
    setSecondaryDisplaysTitle(mItem.size());
    recomputeLayout();

    if (!mUi->addSecondaryDisplay->isEnabled()) {
        mUi->addSecondaryDisplay->setEnabled(true);
    }
}

// Called when UI change the display type, or customize display
void MultiDisplayPageGrpc::changeSecondaryDisplay(int id) {
    recomputeLayout();
}

void MultiDisplayPageGrpc::recomputeLayout() {
    if (mNoRecompute)
        return;

    DD("Recomputing layout");
    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> rectangles;
    std::unordered_map<uint32_t, std::string> names;
    rectangles[0] =
            std::make_pair(getConsoleAgents()->settings->hw()->hw_lcd_width,
                           getConsoleAgents()->settings->hw()->hw_lcd_height);
    names[0] = mSkinName;
    for (auto& [id, item] : mItem) {
        uint32_t width, height;
        item->getValues(&width, &height, nullptr);
        DD("Recomputing layout: %d -- %d (%dx%d)", id, item->id(), width,
           height);
        rectangles[id] = std::make_pair(width, height);
        names[id] = item->getName();
    }

    std::unordered_map<uint32_t, std::pair<uint32_t, uint32_t>> offsets =
            android::base::resolveLayout(rectangles, m_monitorAspectRatio);
    mUi->multiDisplayArrangement->setLayout(rectangles, offsets, names);
}

void MultiDisplayPageGrpc::on_applyChanges_clicked() {
    mApplyCnt++;
    uint32_t numDisplay = 0;
    std::string name;
    DisplayConfigurations updated;
    for (auto& [id, item] : mItem) {
        uint32_t w, h, d;
        item->getValues(&w, &h, &d);
        auto config = updated.add_displays();
        config->set_width(w);
        config->set_height(h);
        config->set_dpi(d);
        config->set_display(id);
    }

    DD("Setting configuration to %s", updated.ShortDebugString().c_str());
    mServiceClient.setDisplayConfigurationsAsync(updated, [&](auto status) {
        if (!status.ok()) {
            {
                std::string msg =
                        "Unable to change display configuration due to: " +
                        std::string(status.status().message());
                getConsoleAgents()->emu->showMessage(
                        msg.c_str(), WindowMessageType::WINDOW_MESSAGE_ERROR,
                        60 * 1000);
            }
        }
    });
    mApplyChangesTracker->increment(name);
}

void MultiDisplayPageGrpc::setArrangementHighLightDisplay(int id) {
    mUi->multiDisplayArrangement->setHighLightDisplay(id);
}

// Called when closing emulator
void MultiDisplayPageGrpc::sendMetrics() {
    const uint64_t now = android::base::System::get()->getHighResTimeUs();

    // Reset the metrics reporting limiter if enough time has passed.
    if (mReportWindowStartUs + kReportWindowDurationUs < now) {
        mReportWindowStartUs = now;
        mReportWindowCount = 0;
    }

    if (mReportWindowCount > kMaxReportsPerWindow) {
        DD("Dropping metrics, too many recent reports.");
        return;
    }

    ++mReportWindowCount;

    android_studio::EmulatorMultiDisplay metrics;
    metrics.set_apply_count(mApplyCnt);
    metrics.set_max_displays(mMaxDisplayCnt);
    android::metrics::MetricsReporter::get().report(
            [metrics](android_studio::AndroidStudioEvent* event) {
                event->mutable_emulator_details()
                        ->mutable_multi_display()
                        ->CopyFrom(metrics);
            });
    mApplyCnt = 0;
    mMaxDisplayCnt = 0;
}

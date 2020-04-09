// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/multi-display-page.h"

#include <qnamespace.h>
#include <QLabel>
#include <QSize>
#include <QVBoxLayout>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>

#include "android/avd/info.h"
#include "android/base/LayoutResolver.h"
#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/globals.h"
#include "android/metrics/MetricsReporter.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/multi-display-arrangement.h"
#include "android/skin/qt/extended-pages/multi-display-item.h"
#include "android/skin/qt/raised-material-button.h"
#include "studio_stats.pb.h"

class QString;
class QWidget;

// Allow at most 5 reports every 60 seconds.
static constexpr uint64_t kReportWindowDurationUs = 1000 * 1000 * 60;
static constexpr uint32_t kMaxReportsPerWindow = 5;

int MultiDisplayPage::sMaxItem = 3;

MultiDisplayPage::MultiDisplayPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::MultiDisplayPage()) {
    mUi->setupUi(this);

    // set default display title
    char* skinDir;
    avdInfo_getSkinInfo(android_avdInfo, &mSkinName, &skinDir);
    std::string defaultDisplayDisp = std::string(mSkinName) +
                                     " (" +
                                     std::to_string(android_hw->hw_lcd_width) +
                                     'x' +
                                     std::to_string(android_hw->hw_lcd_height) +
                                     ')';
    mUi->defaultDisplayText->setText(defaultDisplayDisp.c_str());

    // set secondary display title
    setSecondaryDisplaysTitle(mSecondaryItemCount);

    uint32_t width, height;
    EmulatorQtWindow::getInstance()->getMonitorRect(&width, &height);
    if (width != 0 && height != 0) {
        m_monitorAspectRatio = (double) height / (double) width;
    }

    mUi->verticalLayout_5->setAlignment(Qt::AlignTop);
    // read multi-displays
    mItem.resize(sMaxItem + 1);
    for (int i = 1; i <= sMaxItem; i++) {
        uint32_t w, h, d;
        bool e;
        if (EmulatorQtWindow::getInstance()->getMultiDisplay(i, nullptr, nullptr,
                                                             &w, &h, &d, nullptr,
                                                             &e)
            && e && d != 0) {
            MultiDisplayItem *item = new MultiDisplayItem(i, w, h, d, this);
            LOG(VERBOSE) << "add MultiDisplayItem " << i << " and insert widget";
            mUi->verticalLayout_5->insertWidget(i - 1, item);
            mItem[i] = item;
            ++mSecondaryItemCount;
            setSecondaryDisplaysTitle(mSecondaryItemCount);
        }
    }
    recomputeLayout();

    connect(EmulatorQtWindow::getInstance(), SIGNAL(updateMultiDisplayPage(int)),
            this, SLOT(updateSecondaryDisplay(int)));
}

void MultiDisplayPage::setSecondaryDisplaysTitle(int count) {
    std::string s = "Secondary displays (" + std::to_string(count) + ")";
    mUi->secondaryDisplays_title->setText(s.c_str());
}

void MultiDisplayPage::updateTheme(const QString& styleString) {
    auto icon = getIconForCurrentTheme("add_display");
    mUi->addSecondaryDisplay->setIcon(icon);
    mUi->addSecondaryDisplay->setIconSize(QSize(24, 24));
    mUi->scrollAreaWidgetContents->setStyleSheet(styleString);
    for (int i = 1; i <= sMaxItem; i++) {
        if (mItem[i] != nullptr) {
            mItem[i]->updateTheme();
        }
    }
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
    if (i < 0) {
        return;
    }
    MultiDisplayItem *item = new MultiDisplayItem(i, this);
    LOG(VERBOSE) << "add MultiDisplayItem " << i << " and insert widget";
    mUi->verticalLayout_5->insertWidget(i - 1, item);
    mItem[i] = item;
    setSecondaryDisplaysTitle(++mSecondaryItemCount);
    recomputeLayout();
    item->setFocus();

    if (findNextItemIndex() < 0) {
        mUi->addSecondaryDisplay->setEnabled(false);
    }
}

void MultiDisplayPage::deleteSecondaryDisplay(int id) {
    LOG(VERBOSE) << "deleteSecondaryDisplay " << id;
    delete(mItem[id]);
    mItem[id] = nullptr;
    setSecondaryDisplaysTitle(--mSecondaryItemCount);
    recomputeLayout();
    if (!mUi->addSecondaryDisplay->isEnabled()) {
        mUi->addSecondaryDisplay->setEnabled(true);
    }
}

// Called when UI change the display type, or customize display
void MultiDisplayPage::changeSecondaryDisplay(int id) {
    recomputeLayout();
}

// Called when display add/del/modify from UI, console, config.ini,
// snapshot, but from UI will be filtered out in the function.
void MultiDisplayPage::updateSecondaryDisplay(int i) {
    uint32_t width, height, dpi;
    bool enabled;
    EmulatorQtWindow::getInstance()->getMultiDisplay(i, nullptr, nullptr,
                                                     &width, &height,
                                                     &dpi, nullptr,
                                                     &enabled);
    if (!enabled) {
        if (mItem[i] == nullptr) {
            LOG(VERBOSE) << "already void MultiDisplayItem";
            return;
        } else {
            deleteSecondaryDisplay(i);
        }
    } else {
        if (mItem[i] == nullptr) {
            LOG(VERBOSE) << "create MultiDisplayItem " << i << " and insert widget";
            MultiDisplayItem *item = new MultiDisplayItem(i, width, height, dpi, this);
            mUi->verticalLayout_5->insertWidget(i - 1, item);
            mItem[i] = item;
            ++mSecondaryItemCount;
            setSecondaryDisplaysTitle(mSecondaryItemCount);
            recomputeLayout();
        } else {
            uint32_t w, h, d;
            mItem[i]->getValues(&w, &h, &d);
            if (width != w || height != h || dpi != d) {
                LOG(VERBOSE) << "delete MultiDisplayItem " << i;
                delete(mItem[i]);
                mItem[i] = nullptr;
                MultiDisplayItem *item = new MultiDisplayItem(i, width, height, dpi, this);
                mItem[i] = item;
                LOG(VERBOSE) << "add MultiDisplayItem " << i << " and insert widget";
                mUi->verticalLayout_5->insertWidget(i - 1, item);
                recomputeLayout();
            } else {
                LOG(VERBOSE) << "same MultiDisplayItem, not update UI";
            }
        }
    }
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
        android::base::resolveLayout(rectangles, m_monitorAspectRatio);
    mUi->multiDisplayArrangement->setLayout(rectangles, offsets, names);
}

void MultiDisplayPage::on_applyChanges_clicked() {
    mApplyCnt++;
    uint32_t numDisplay = 0;
    for (int i = 1; i <= sMaxItem; i++) {
        uint32_t width, height, dpi;
        bool enabled;
        EmulatorQtWindow::getInstance()->getMultiDisplay(i, nullptr, nullptr,
                                                         &width, &height,
                                                         &dpi, nullptr,
                                                         &enabled);
        if (mItem[i] != nullptr) {
            numDisplay++;
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
                                                                d,
                                                                0);
        } else {
            if (enabled) {
                EmulatorQtWindow::getInstance()->switchMultiDisplay(false,
                                                                    i,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0,
                                                                    0);
            }
        }
    }
    if (numDisplay > mMaxDisplayCnt) {
        mMaxDisplayCnt = numDisplay;
    }
}

void MultiDisplayPage::setArrangementHighLightDisplay(int id) {
    mUi->multiDisplayArrangement->setHighLightDisplay(id);
}

// Called when closing emulator
void MultiDisplayPage::sendMetrics() {
    const uint64_t now = android::base::System::get()->getHighResTimeUs();

    // Reset the metrics reporting limiter if enough time has passed.
    if (mReportWindowStartUs + kReportWindowDurationUs < now) {
        mReportWindowStartUs = now;
        mReportWindowCount = 0;
    }

    if (mReportWindowCount > kMaxReportsPerWindow) {
        LOG(INFO) << "Dropping metrics, too many recent reports.";
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

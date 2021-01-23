// Copyright 2019 The Android Open Source Project
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
#include "android/skin/qt/extended-pages/ui-metrics-page.h"

#include <QCloseEvent>
#include <QIntValidator>
#include <QLineEdit>
#include <QStandardPaths>

#include "android/base/StringFormat.h"
#include "android/base/files/PathUtils.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/ui-metrics-widget.h"
#include "ui_ui-metrics-page.h"

#include <ctime>

class QCloseEvent;
class QHideEvent;
class QShowEvent;
class QWidget;

using namespace android::base;
UiMetricsPage::UiMetricsPage(QWidget* parent)
    : QFrame(parent), mUi(new Ui::UiMetricsPage()) {
#ifdef __linux__
    // On Linux, a Dialog does not show a Close button and a
    // Tool has a Close button that shows a dot rather than
    // an X. So we use a SubWindow.
    setWindowFlags(Qt::SubWindow | Qt::WindowCloseButtonHint);
#else
    // On Windows, a SubWindow does not show a Close button.
    // A Tool for Windows and Mac
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
#endif
    mUi->setupUi(this);

    EmulatorQtWindow* win = EmulatorQtWindow::getInstance();
    uint32_t width = kDefaultScreenSize;
    uint32_t height = kDefaultScreenSize;
    if (win)
        win->getMonitorRect(&width, &height);
    mUi->xLineEdit->setValidator(new QIntValidator(0, width, this));
    mUi->yLineEdit->setValidator(new QIntValidator(0, height, this));
    mUi->exportButton->setDisabled(true);

    connect(this, SIGNAL(windowHidden()), this, SLOT(disableCollection()));
}

UiMetricsPage::~UiMetricsPage() {}

void UiMetricsPage::hideEvent(QHideEvent* event) {
    emit windowHidden();
}

void UiMetricsPage::on_playStopButton_pressed() {
    if (mInFlight) {
        mUi->exportButton->setDisabled(false);
        disableCollection();
    } else {
        mUi->exportButton->setDisabled(true);
        enableCollection();
    }
    mInFlight = !mInFlight;
}

void UiMetricsPage::on_exportButton_pressed() {
    if (mInFlight)
        return;
    // Save to desktop location
    const auto desktopDir =
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
                    .toStdString();
    if (!desktopDir.empty()) {
        std::time_t t = std::time(nullptr);
        std::tm* tm = std::localtime(&t);
        auto file =
                StringFormat("ui-metrics-%d-%d-%d:%d:%d.csv", tm->tm_mon,
                             tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        const auto path = PathUtils::join(desktopDir, file);
        mUi->uiMetricsWidget->saveToPath(path);
    }
}

void UiMetricsPage::enableCollection() {
    int x = mUi->xLineEdit->text().toInt();
    int y = mUi->yLineEdit->text().toInt();
    mUi->playStopButton->setIcon(getIconForCurrentTheme("stop"));
    mUi->playStopButton->setProperty("themeIconName", "stop");
    mUi->playStopButton->setText("Stop");
    mUi->uiMetricsWidget->onCollectionEnabled(x, y);
}

void UiMetricsPage::disableCollection() {
    mUi->playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->playStopButton->setProperty("themeIconName", "play_arrow");
    mUi->playStopButton->setText("Play");
    mUi->uiMetricsWidget->onCollectionDisabled();
}

void UiMetricsPage::closeEvent(QCloseEvent* event) {
    hide();
    event->ignore();
}

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

#include <QCloseEvent>  // for QCloseEvent
#include <QIntValidator>
#include <QLineEdit>
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/ui-metrics-widget.h"
#include "ui_ui-metrics-page.h"

class QCloseEvent;
class QHideEvent;
class QShowEvent;
class QWidget;

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

    connect(this, SIGNAL(windowHidden()), this, SLOT(disableCollection()));
}

UiMetricsPage::~UiMetricsPage() {}

void UiMetricsPage::hideEvent(QHideEvent* event) {
    emit windowHidden();
}

void UiMetricsPage::on_playStopButton_pressed() {
    if (mInFlight) {
        disableCollection();
    } else {
        enableCollection();
    }
    mInFlight = !mInFlight;
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

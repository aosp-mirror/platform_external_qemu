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
#include "android/skin/qt/extended-pages/perfstats-page.h"

#include <QCloseEvent>                             // for QCloseEvent

#include "android/skin/qt/perf-stats-3d-widget.h"  // for PerfStats3DWidget
#include "ui_perfstats-page.h"                     // for PerfStatsPage

class QCloseEvent;
class QHideEvent;
class QShowEvent;
class QWidget;

PerfStatsPage::PerfStatsPage(QWidget* parent)
    : QFrame(parent), mUi(new Ui::PerfStatsPage()) {
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
    connect(this, SIGNAL(windowVisible()),
            this, SLOT(enableCollection()));

    connect(this, SIGNAL(windowHidden()),
            this, SLOT(disableCollection()));
}

PerfStatsPage::~PerfStatsPage() { }

void PerfStatsPage::showEvent(QShowEvent* event) {
    emit windowVisible();
}

void PerfStatsPage::hideEvent(QHideEvent* event) {
    emit windowHidden();
}

void PerfStatsPage::enableCollection() {
    mUi->perfstatsWidget->onCollectionEnabled();
}

void PerfStatsPage::disableCollection() {
    mUi->perfstatsWidget->onCollectionDisabled();
}

void PerfStatsPage::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

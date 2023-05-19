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

#include "android/skin/qt/extended-pages/record-and-playback-page.h"

#include <QTabWidget>                                             // for QTa...

#include "android/skin/qt/extended-pages/record-macro-page.h"     // for Rec...
#include "android/skin/qt/extended-pages/record-screen-page.h"    // for Rec...
#include "android/skin/qt/extended-pages/record-settings-page.h"  // for Rec...

class QWidget;

RecordAndPlaybackPage::RecordAndPlaybackPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordAndPlaybackPage()) {
    mUi->setupUi(this);

    connect(mUi->recordSettings, SIGNAL(on_toggleMacros_toggled(bool)), 
            mUi->recordMacro, SLOT(enablePresetMacros(bool)));

    connect(mUi->recordMacro, SIGNAL(setRecordingStateSignal(bool)), this,
            SLOT(setRecordingStateSlot(bool)));
}

void RecordAndPlaybackPage::updateTheme() {
    mUi->recordScreen->updateTheme();
}

void RecordAndPlaybackPage::enableCustomMacros() {
    mUi->tabWidget->setTabText(1, tr("Macro Record"));
}

void RecordAndPlaybackPage::removeSettingsTab() {
    mUi->tabWidget->removeTab(2);
}

void RecordAndPlaybackPage::setRecordingStateSlot(bool state) {
    emit ensureVirtualSceneWindowCreated();
    emit setRecordingStateSignal(state);
}

void RecordAndPlaybackPage::focusMacroRecordTab() {
    mUi->tabWidget->setCurrentIndex(1);
}

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

#include "android/skin/qt/extended-pages/record-macro-page.h"

#include "android/base/StringFormat.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/control/automation_agent.h"
#include "android/skin/qt/extended-pages/record-macro-saved-item.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/video-player/QtVideoPlayerNotifier.h"

#include <QMessageBox>
#include <iostream>
#include <sstream>

using namespace android::base;

const QAndroidAutomationAgent* RecordMacroPage::sAutomationAgent = nullptr;

RecordMacroPage::RecordMacroPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordMacroPage()) {
    mUi->setupUi(this);
    loadUi();
}

// static
void RecordMacroPage::setAutomationAgent(const QAndroidAutomationAgent* agent) {
    sAutomationAgent = agent;
}

void RecordMacroPage::loadUi() {
    // Clear all items. Might need to optimize this and keep track of existing.
    mUi->macroList->clear();

    const std::string macrosPath = getMacrosDirectory();
    const std::vector<std::string> macroFileNames =
            System::get()->scanDirEntries(macrosPath);

    // For every macro, create a macroSavedItem with its name.
    for (const auto& macroName : macroFileNames) {
        RecordMacroSavedItem* macroSavedItem = new RecordMacroSavedItem();
        macroSavedItem->setName(macroName.c_str());
        macroSavedItem->setDisplayInfo("Preset macro");

        QListWidgetItem* listItem = new QListWidgetItem(mUi->macroList);
        listItem->setSizeHint(macroSavedItem->sizeHint());

        mUi->macroList->addItem(listItem);
        mUi->macroList->setItemWidget(listItem, macroSavedItem);
    }

    setMacroUiState(MacroUiState::Waiting);
}

void RecordMacroPage::on_playButton_clicked() {
    // Stop and reset automation.
    sAutomationAgent->stopPlayback();

    // Get macro name from item widget.
    QListWidgetItem* listItem = mUi->macroList->selectedItems().first();
    RecordMacroSavedItem* macroSavedItem = qobject_cast<RecordMacroSavedItem*>(
            mUi->macroList->itemWidget(listItem));
    const std::string macroName = macroSavedItem->getName();
    const std::string macroAbsolutePath =
            PathUtils::join(getMacrosDirectory(), macroName);

    auto result = sAutomationAgent->startPlayback(macroAbsolutePath);
    if (result.err()) {
        std::ostringstream errString;
        errString << result.unwrapErr();

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText("An error ocurred.");
        msgBox.setInformativeText(errString.str().c_str());
        msgBox.setDefaultButton(QMessageBox::Save);

        int ret = msgBox.exec();
        return;
    }

    mCurrentMacroName = macroName;
    mMacroPlaying = true;
    setMacroUiState(MacroUiState::Playing);
}

void RecordMacroPage::on_macroList_itemPressed(QListWidgetItem* listItem) {
    RecordMacroSavedItem* macroSavedItem = qobject_cast<RecordMacroSavedItem*>(
            mUi->macroList->itemWidget(listItem));
    const std::string macroName = macroSavedItem->getName();

    if (mMacroPlaying && mCurrentMacroName == macroName) {
        setMacroUiState(MacroUiState::Playing);
    } else {
        setMacroUiState(MacroUiState::Selected);
    }

    showPreview(macroName);
}

// For dragging and clicking outside the items in the item list.
void RecordMacroPage::on_macroList_itemSelectionChanged() {
    setMacroUiState(MacroUiState::Waiting);
}

std::string RecordMacroPage::getMacrosDirectory() {
    return PathUtils::join(System::get()->getLauncherDirectory(), "resources",
                           "macros");
}

std::string RecordMacroPage::getMacroPreviewsDirectory() {
    return PathUtils::join(System::get()->getLauncherDirectory(), "resources",
                           "macroPreviews");
}

void RecordMacroPage::setMacroUiState(MacroUiState state) {
    mState = state;

    switch (mState) {
        case MacroUiState::Waiting: {
            mUi->previewLabel->setText("Select a macro to preview");
            mUi->previewLabel->show();
            mUi->previewOverlay->show();
            mUi->playButton->setEnabled(false);
            break;
        }
        case MacroUiState::Selected: {
            mUi->previewLabel->hide();
            mUi->previewOverlay->hide();
            mUi->playButton->setEnabled(true);
            break;
        }
        case MacroUiState::Playing: {
            mUi->previewLabel->setText("Macro playing on the Emulator");
            mUi->previewLabel->show();
            mUi->previewOverlay->show();
            mUi->playButton->setEnabled(true);
            break;
        }
    }
}

void RecordMacroPage::showPreview(const std::string& previewName) {
    const std::string previewPath =
            PathUtils::join(getMacroPreviewsDirectory(), previewName + ".gif");

    auto videoPlayerNotifier =
            std::unique_ptr<android::videoplayer::QtVideoPlayerNotifier>(
                    new android::videoplayer::QtVideoPlayerNotifier());
    connect(videoPlayerNotifier.get(), SIGNAL(updateWidget()), this,
            SLOT(updateVideoView()));
    mVideoPlayer = android::videoplayer::VideoPlayer::create(
            previewPath, mUi->videoWidget, std::move(videoPlayerNotifier));

    mVideoPlayer->scheduleRefresh(20);
    mVideoPlayer->start();
}

void RecordMacroPage::updateVideoView() {
    mUi->videoWidget->repaint();
}

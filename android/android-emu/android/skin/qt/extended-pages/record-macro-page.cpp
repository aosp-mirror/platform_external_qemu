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
    std::vector<std::string> macroFileNames =
            System::get()->scanDirEntries(macrosPath);

    // For every macro, create a macroSavedItem with its name.
    for (auto& macroName : macroFileNames) {
        std::replace(macroName.begin(), macroName.end(), '_', ' ');
        RecordMacroSavedItem* macroSavedItem = new RecordMacroSavedItem();
        macroSavedItem->setName(macroName.c_str());
        macroSavedItem->setDisplayInfo(tr("Preset macro"));

        QListWidgetItem* listItem = new QListWidgetItem(mUi->macroList);
        listItem->setSizeHint(macroSavedItem->sizeHint());

        mUi->macroList->addItem(listItem);
        mUi->macroList->setItemWidget(listItem, macroSavedItem);
    }

    setMacroUiState(MacroUiState::Waiting);
}

void RecordMacroPage::on_playStopButton_clicked() {
    // Stop and reset automation.
    sAutomationAgent->stopPlayback();

    QListWidgetItem* listItem = mUi->macroList->selectedItems().first();
    if (mState == MacroUiState::Playing) {
        stopButtonClicked(listItem);
    } else {
        playButtonClicked(listItem);
    }
}

void RecordMacroPage::on_macroList_itemPressed(QListWidgetItem* listItem) {
    RecordMacroSavedItem* macroSavedItem = getItemWidget(listItem);
    std::string macroName = macroSavedItem->getName();
    std::replace(macroName.begin(), macroName.end(), ' ', '_');

    if (mMacroPlaying && mCurrentMacroName == macroName) {
        setMacroUiState(MacroUiState::Playing);
    } else {
        setMacroUiState(MacroUiState::Selected);
        showPreview(macroName);
    }
}

// For dragging and clicking outside the items in the item list.
void RecordMacroPage::on_macroList_itemSelectionChanged() {
    if (mVideoPlayer && mVideoPlayer->isRunning()) {
        mVideoPlayer->stop();
    }

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
            mUi->previewLabel->setText(tr("Select a macro to preview"));
            mUi->previewLabel->show();
            mUi->previewOverlay->show();
            mUi->playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
            mUi->playStopButton->setProperty("themeIconName", "play_arrow");
            mUi->playStopButton->setText(tr("PLAY "));
            mUi->playStopButton->setEnabled(false);
            break;
        }
        case MacroUiState::Selected: {
            mUi->previewLabel->hide();
            mUi->previewOverlay->hide();
            mUi->playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
            mUi->playStopButton->setProperty("themeIconName", "play_arrow");
            mUi->playStopButton->setText(tr("PLAY "));
            mUi->playStopButton->setEnabled(true);
            break;
        }
        case MacroUiState::PreviewFinished: {
            mUi->previewLabel->setText(tr("Click anywhere to replay preview"));
            mUi->previewLabel->show();
            mUi->previewOverlay->show();
            mUi->playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
            mUi->playStopButton->setProperty("themeIconName", "play_arrow");
            mUi->playStopButton->setText(tr("PLAY "));
            mUi->playStopButton->setEnabled(true);
            break;
        }
        case MacroUiState::Playing: {
            mUi->previewLabel->setText(tr("Macro playing on the Emulator"));
            mUi->previewLabel->show();
            mUi->previewOverlay->show();
            mUi->playStopButton->setIcon(getIconForCurrentTheme("stop"));
            mUi->playStopButton->setProperty("themeIconName", "stop");
            mUi->playStopButton->setText(tr("STOP "));
            mUi->playStopButton->setEnabled(true);
            break;
        }
    }
}

void RecordMacroPage::playButtonClicked(QListWidgetItem* listItem) {
    RecordMacroSavedItem* macroSavedItem = getItemWidget(listItem);
    macroSavedItem->setDisplayInfo(tr("Now playing..."));
    mVideoPlayer->stop();

    std::string macroName = macroSavedItem->getName();
    std::replace(macroName.begin(), macroName.end(), ' ', '_');
    const std::string macroAbsolutePath =
            PathUtils::join(getMacrosDirectory(), macroName);

    auto result = sAutomationAgent->startPlayback(macroAbsolutePath);
    if (result.err()) {
        std::ostringstream errString;
        errString << result.unwrapErr();

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(tr("An error ocurred."));
        msgBox.setInformativeText(errString.str().c_str());
        msgBox.setDefaultButton(QMessageBox::Save);

        int ret = msgBox.exec();
        return;
    }

    disableMacroItemsExcept(listItem);

    mCurrentMacroName = macroName;
    mMacroPlaying = true;
    setMacroUiState(MacroUiState::Playing);
}

void RecordMacroPage::stopButtonClicked(QListWidgetItem* listItem) {
    RecordMacroSavedItem* macroSavedItem = getItemWidget(listItem);
    macroSavedItem->setDisplayInfo(tr("Preset macro"));

    enableMacroItems();

    mMacroPlaying = false;
    setMacroUiState(MacroUiState::PreviewFinished);
}

void RecordMacroPage::showPreview(const std::string& previewName) {
    const std::string previewPath =
            PathUtils::join(getMacroPreviewsDirectory(), previewName + ".mp4");

    auto videoPlayerNotifier =
            std::unique_ptr<android::videoplayer::QtVideoPlayerNotifier>(
                    new android::videoplayer::QtVideoPlayerNotifier());
    connect(videoPlayerNotifier.get(), SIGNAL(updateWidget()), this,
            SLOT(updatePreviewVideoView()));
    connect(videoPlayerNotifier.get(), SIGNAL(videoStopped()), this,
            SLOT(previewVideoPlayingFinished()));
    mVideoPlayer = android::videoplayer::VideoPlayer::create(
            previewPath, mUi->videoWidget, std::move(videoPlayerNotifier));

    mVideoPlayer->scheduleRefresh(20);
    mVideoPlayer->start();
}

RecordMacroSavedItem* RecordMacroPage::getItemWidget(
        QListWidgetItem* listItem) {
    return qobject_cast<RecordMacroSavedItem*>(
            mUi->macroList->itemWidget(listItem));
}

void RecordMacroPage::updatePreviewVideoView() {
    mUi->videoWidget->repaint();
}

void RecordMacroPage::previewVideoPlayingFinished() {
    setMacroUiState(MacroUiState::PreviewFinished);
}

void RecordMacroPage::mousePressEvent(QMouseEvent* event) {
    if (mState == MacroUiState::PreviewFinished) {
        QListWidgetItem* listItem = mUi->macroList->selectedItems().first();
        std::string macroName = getItemWidget(listItem)->getName();
        std::replace(macroName.begin(), macroName.end(), ' ', '_');
        showPreview(macroName);
        setMacroUiState(MacroUiState::Selected);
    }
}

void RecordMacroPage::disableMacroItemsExcept(QListWidgetItem* listItem) {
    for (int i = 0; i < mUi->macroList->count(); ++i) {
        QListWidgetItem* item = mUi->macroList->item(i);
        if (item != listItem) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            RecordMacroSavedItem* macroItem = getItemWidget(item);
            macroItem->setEnabled(false);
        }
    }
}

void RecordMacroPage::enableMacroItems() {
    for (int i = 0; i < mUi->macroList->count(); ++i) {
        QListWidgetItem* item = mUi->macroList->item(i);
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
        RecordMacroSavedItem* macroItem = getItemWidget(item);
        macroItem->setEnabled(true);
    }
}

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

#include <qdialog.h>
#include <qdialogbuttonbox.h>
#include <qmessagebox.h>
#include <qnamespace.h>
#include <qobject.h>
#include <qstandardpaths.h>
#include <qstring.h>
#include <QAbstractItemModel>
#include <QByteArray>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFlags>
#include <QHash>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QRegExp>
#include <QSize>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QVariant>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iostream>
#include <sstream>
#include <type_traits>
#include <utility>

#include "android/automation/AutomationController.h"
#include "android/base/EnumFlags.h"
#include "android/base/Log.h"
#include "android/base/Result.h"
#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/emulation/control/automation_agent.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/featurecontrol/Features.h"
#include "android/metrics/MetricsReporter.h"
#include "android/recording/video/player/VideoPlayer.h"
#include "android/recording/video/player/VideoPlayerNotifier.h"
#include "android/settings-agent.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/extended-pages/record-macro-edit-dialog.h"
#include "android/skin/qt/extended-pages/record-macro-saved-item.h"
#include "android/skin/qt/logging-category.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/video-player/QtVideoPlayerNotifier.h"
#include "android/skin/qt/video-player/VideoInfo.h"
#include "android/skin/qt/video-player/VideoPlayerWidget.h"
#include "studio_stats.pb.h"
#include "ui_record-macro-page.h"

class CCListItem;
class QHideEvent;
class QLineEdit;
class QListWidgetItem;
class QMouseEvent;
class QPushButton;
class QShowEvent;
class QVBoxLayout;
class QWidget;

static const char SECONDS_RECORDING[] = "%1s Recording...";

using android::metrics::MetricsReporter;
using android_studio::EmulatorAutomation;
using EmulatorAutomationPresetMacro =
        android_studio::EmulatorAutomation::EmulatorAutomationPresetMacro;

using namespace android::base;

// Allow at most 5 reports every 60 seconds.
static constexpr uint64_t kReportWindowDurationUs = 1000 * 1000 * 60;
static constexpr uint32_t kMaxReportsPerWindow = 5;

const QAndroidAutomationAgent* RecordMacroPage::sAutomationAgent = nullptr;
RecordMacroPage* RecordMacroPage::sInstance = nullptr;

RecordMacroPage::RecordMacroPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordMacroPage()) {
    mUi->setupUi(this);
    sInstance = this;

    mRecordEnabled = android::featurecontrol::isEnabled(
            android::featurecontrol::MacroUi);
    if (!mRecordEnabled) {
        mUi->recButton->hide();
    }

    loadUi();

    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &RecordMacroPage::updateElapsedTime);

    QObject::connect(this, &RecordMacroPage::playbackFinishedSignal, this,
                     &RecordMacroPage::playbackFinished);
    QObject::connect(mUi->macroList->selectionModel(), &QItemSelectionModel::selectionChanged,
                     this, &RecordMacroPage::onSelectionChanged);
}

RecordMacroPage::~RecordMacroPage() {
    sInstance = nullptr;
}

// static
void RecordMacroPage::setAutomationAgent(const QAndroidAutomationAgent* agent) {
    sAutomationAgent = agent;
}

void RecordMacroPage::showEvent(QShowEvent* event) {
    mAutomationMetrics = AutomationMetrics();
    // Some assets need to be reloaded.
    if (mFirstShowEvent) {
        loadUi();
        mFirstShowEvent = false;
    }
}

void RecordMacroPage::hideEvent(QHideEvent* event) {
    if (!mAutomationMetrics.previewReplayCount &&
        !mAutomationMetrics.playbackCount) {
        LOG(INFO) << "Dropping metrics, nothing to report.";
        return;
    }

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

    reportAllMetrics();
}

void RecordMacroPage::loadUi() {
    // Clear all items. Might need to optimize this and keep track of existing.
    mUi->macroList->clear();

    // Lengths as QStrings have to be initialized here to use tr().
    mLengths = {{"Reset_position", tr("0:00")},
                {"Track_horizontal_plane", tr("0:16")},
                {"Track_vertical_plane", tr("0:12")},
                {"Walk_to_image_room", tr("0:14")}};
    mDescriptions.clear();

    const std::string macrosPath = getMacrosDirectory();
    std::vector<std::string> macroFileNames =
            System::get()->scanDirEntries(macrosPath);

    // For every macro, create a macroSavedItem with its name.
    for (auto& macroName : macroFileNames) {
        createMacroItem(macroName, true);
    }

    if (mRecordEnabled) {
        loadCustomMacros();
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
        reportMacroPlayed();
        playButtonClicked(listItem);
    }
}

void RecordMacroPage::on_macroList_itemPressed(QListWidgetItem* listItem) {
    // An item can be pressed, but deselected (via Ctrl + Click). Verify that the item
    // is selected.
    if (!listItem->isSelected()) {
        return;
    }

    const std::string macroName = getMacroNameFromItem(listItem);

    if (mMacroPlaying && mCurrentMacroName == macroName) {
        setMacroUiState(MacroUiState::Playing);
        showPreviewFrame(macroName);
    } else {
        setMacroUiState(MacroUiState::Selected);
        showPreview(macroName);
    }
}

void RecordMacroPage::onSelectionChanged(
        const QItemSelection& selected,
        const QItemSelection& deselected) {
    // Stop video playback.
    if (mVideoPlayer && mVideoPlayer->isRunning()) {
        mVideoPlayer->stop();
    }
    setMacroUiState(MacroUiState::Waiting);

    if (!deselected.isEmpty()) {
        for (auto i : deselected.indexes()) {
            auto item = qobject_cast<RecordMacroSavedItem*>(mUi->macroList->indexWidget(i));
            item->setSelected(false);
        }
    }
    if (!selected.isEmpty()) {
        for (auto i : selected.indexes()) {
            auto item = qobject_cast<RecordMacroSavedItem*>(mUi->macroList->indexWidget(i));
            item->setSelected(true);
        }
    }
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
            if (!mRecording) {
                mUi->previewLabel->setText(tr("Select a macro to preview"));
                mUi->previewLabel->show();
            }
            mUi->previewOverlay->show();
            mUi->replayIcon->hide();
            mUi->playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
            mUi->playStopButton->setProperty("themeIconName", "play_arrow");
            mUi->playStopButton->setText(tr("PLAY "));
            mUi->playStopButton->setEnabled(false);
            break;
        }
        case MacroUiState::Selected: {
            mUi->previewLabel->hide();
            mUi->previewOverlay->hide();
            mUi->replayIcon->hide();
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
            mUi->replayIcon->setPixmap(getIconForCurrentTheme("refresh").pixmap(QSize(36, 36)));
            mUi->replayIcon->show();
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
            mUi->replayIcon->hide();
            mUi->playStopButton->setIcon(getIconForCurrentTheme("stop_red"));
            mUi->playStopButton->setProperty("themeIconName", "stop_red");
            mUi->playStopButton->setText(tr("STOP "));
            mUi->playStopButton->setEnabled(true);
            break;
        }
        case MacroUiState::PreRecording: {
            mUi->playStopButton->setEnabled(false);
            break;
        }
        case MacroUiState::Recording: {
            mUi->playStopButton->setEnabled(false);
            break;
        }
    }

    if (mRecordEnabled) {
        setRecordState();
    }
}

void RecordMacroPage::playButtonClicked(QListWidgetItem* listItem) {
    RecordMacroSavedItem* macroSavedItem = getItemWidget(listItem);
    macroSavedItem->setDisplayInfo(tr("Now playing..."));
    if (mVideoPlayer && mVideoPlayer->isRunning()) {
        mVideoPlayer->stop();
    }

    const std::string macroName = getMacroNameFromItem(listItem);
    // Check if preset or custom.
    std::string macroAbsolutePath;
    if (macroSavedItem->isPreset()) {
        macroAbsolutePath = PathUtils::join(getMacrosDirectory(), macroName);
    } else {
        macroAbsolutePath =
                PathUtils::join(getCustomMacrosDirectory(), macroName);
    }

    disableMacroItemsExcept(listItem);

    mCurrentMacroName = macroName;
    mMacroPlaying = true;
    mMacroItemPlaying = macroSavedItem;
    mListItemPlaying = listItem;
    setMacroUiState(MacroUiState::Playing);

    // Update every second.
    mSec = 0;
    mMacroItemPlaying->setDisplayTime(getTimerString(0));
    mTimer.start(1000);

    // Start timer for report.
    mElapsedTimeTimer.start();

    showPreviewFrame(macroName);

    auto result = sAutomationAgent->startPlaybackWithCallback(
            macroAbsolutePath, &RecordMacroPage::stopCurrentMacro);
    if (result.err()) {
        std::ostringstream errString;
        errString << result.unwrapErr();
        stopButtonClicked(listItem);
        displayErrorBox(errString.str());
        return;
    }

    if (macroSavedItem->isPreset()) {
        reportPresetMacroPlayed(macroName);
    }
}

void RecordMacroPage::stopButtonClicked(QListWidgetItem* listItem) {
    const std::string macroName = getMacroNameFromItem(listItem);
    RecordMacroSavedItem* macroSavedItem = getItemWidget(listItem);
    macroSavedItem->setDisplayInfo(mDescriptions[macroName]);

    enableMacroItems();

    mMacroPlaying = false;
    mUi->macroList->setCurrentItem(listItem);
    setMacroUiState(MacroUiState::PreviewFinished);

    mTimer.stop();
    mMacroItemPlaying->setDisplayTime(mLengths[mCurrentMacroName]);

    reportTotalDuration();
    showPreviewFrame(macroName);
}

void RecordMacroPage::showPreview(const std::string& macroName) {
    // Guard for missing preview.
    if (mRecordEnabled && !isPreviewAvailable(macroName)) {
        mUi->previewLabel->setText(tr("No preview available."));
        mUi->previewLabel->show();
        mUi->previewOverlay->show();
        mUi->replayIcon->hide();
        showPreviewFrame("Reset_position");
        return;
    }

    const std::string previewPath =
            PathUtils::join(getMacroPreviewsDirectory(), macroName + ".mp4");

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
    // Preview can be stopped by deselecting the list item. Verify the item is still selected
    // before reshowing the preview.
    if (mUi->macroList->selectedItems().isEmpty()) {
        return;
    }

    setMacroUiState(MacroUiState::PreviewFinished);

    QListWidgetItem* listItem = mUi->macroList->selectedItems().first();
    const std::string macroName = getMacroNameFromItem(listItem);
    showPreviewFrame(macroName);
}

void RecordMacroPage::mousePressEvent(QMouseEvent* event) {
    if (mState == MacroUiState::PreviewFinished) {
        // Preview could have been deselected. Verify the item is still selected before
        // replaying the preview.
        if (mUi->macroList->selectedItems().isEmpty()) {
            return;
        }

        QListWidgetItem* listItem = mUi->macroList->selectedItems().first();
        const std::string macroName = getMacroNameFromItem(listItem);
        setMacroUiState(MacroUiState::Selected);

        showPreview(macroName);
        reportPreviewPlayedAgain();
    }
}

void RecordMacroPage::disableMacroItemsExcept(QListWidgetItem* listItem) {
    // Make selection show that macro is playing.
    SettingsTheme theme = getSelectedTheme();
    mUi->macroList->setStyleSheet(
            "QListWidget::item:focus, QListView::item:selected { "
            "background-color: " +
            Ui::stylesheetValues(theme)[Ui::MACRO_BKG_COLOR_VAR] + "}");

    for (int i = 0; i < mUi->macroList->count(); ++i) {
        QListWidgetItem* item = mUi->macroList->item(i);
        RecordMacroSavedItem* macroItem = getItemWidget(item);
        macroItem->setEditButtonEnabled(false);
        if (item != listItem) {
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            macroItem->setEnabled(false);
        }
    }
}

void RecordMacroPage::disableMacroItems() {
    for (int i = 0; i < mUi->macroList->count(); ++i) {
        QListWidgetItem* item = mUi->macroList->item(i);
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        RecordMacroSavedItem* macroItem = getItemWidget(item);
        macroItem->setEnabled(false);
    }
}

void RecordMacroPage::enableMacroItems() {
    // Return selection to normal.
    mUi->macroList->setStyleSheet("");

    for (int i = 0; i < mUi->macroList->count(); ++i) {
        QListWidgetItem* item = mUi->macroList->item(i);
        item->setFlags(item->flags() | Qt::ItemIsEnabled);
        RecordMacroSavedItem* macroItem = getItemWidget(item);
        macroItem->setEnabled(true);
        if (!macroItem->isPreset()) {
            macroItem->setEditButtonEnabled(true);
        }
    }
}

void RecordMacroPage::showPreviewFrame(const std::string& macroName) {
    // Guard for missing preview.
    if (mRecordEnabled && !isPreviewAvailable(macroName)) {
        return;
    }

    const std::string previewPath =
            PathUtils::join(getMacroPreviewsDirectory(), macroName + ".mp4");

    mVideoInfo.reset(
            new android::videoplayer::VideoInfo(mUi->videoWidget, previewPath));
    connect(mVideoInfo.get(), SIGNAL(updateWidget()), this,
            SLOT(updatePreviewVideoView()));

    mVideoInfo->show();
}

std::string RecordMacroPage::getMacroNameFromItem(QListWidgetItem* listItem) {
    QVariant data = listItem->data(Qt::UserRole);
    QString macroName = data.toString();
    return macroName.toUtf8().constData();
}

void RecordMacroPage::updateElapsedTime() {
    mSec++;
    switch (mState) {
        case MacroUiState::PreRecording: {
            // Countdown from 3 to 0.
            QString qs = tr("%1").arg(3 - mSec);
            mUi->countLabel->setText(qs);
            if (mSec == 3) {
                startRecording();
                return;
            }
            break;
        }
        case MacroUiState::Recording: {
            QString qs = tr(SECONDS_RECORDING).arg(mSec);
            mUi->recordingLabel->setText(qs);
            break;
        }
        default: {
            mMacroItemPlaying->setDisplayTime(getTimerString(mSec));
            break;
        }
    }
    mTimer.start(1000);
}

QString RecordMacroPage::getTimerString(int seconds) {
    QString qs = tr("%1:%2%3 / ")
                         .arg(seconds / 60)
                         .arg(seconds % 60 > 9 ? "" : "0")
                         .arg(seconds % 60);
    qs.append(mLengths[mCurrentMacroName]);
    return qs;
}

// static
void RecordMacroPage::stopCurrentMacro() {
    sInstance->emitPlaybackFinished();
}

void RecordMacroPage::emitPlaybackFinished() {
    emit playbackFinishedSignal();
}

void RecordMacroPage::playbackFinished() {
    stopButtonClicked(mListItemPlaying);
}

void RecordMacroPage::reportMacroPlayed() {
    mAutomationMetrics.playbackCount++;
}

void RecordMacroPage::reportPreviewPlayedAgain() {
    mAutomationMetrics.previewReplayCount++;
}

void RecordMacroPage::reportTotalDuration() {
    mAutomationMetrics.totalDurationMs += mElapsedTimeTimer.elapsed();
}

void RecordMacroPage::reportMacroRecorded() {
    mAutomationMetrics.recordMacroCount++;
}

void RecordMacroPage::reportMacroDeleted() {
    mAutomationMetrics.deleteMacroCount++;
}

void RecordMacroPage::reportMacroEdited() {
    mAutomationMetrics.editMacroCount++;
}

void RecordMacroPage::reportPresetMacroPlayed(const std::string& macroName) {
    EmulatorAutomationPresetMacro presetMacro;

    if (macroName == "Reset_position") {
        presetMacro =
                EmulatorAutomation::EMULATOR_AUTOMATION_PRESET_MACRO_RESET;
    } else if (macroName == "Track_horizontal_plane") {
        presetMacro = EmulatorAutomation::
                EMULATOR_AUTOMATION_PRESET_MACRO_TRACK_HORIZONTAL;
    } else if (macroName == "Track_vertical_plane") {
        presetMacro = EmulatorAutomation::
                EMULATOR_AUTOMATION_PRESET_MACRO_TRACK_VERTICAL;
    } else if (macroName == "Walk_to_image_room") {
        presetMacro =
                EmulatorAutomation::EMULATOR_AUTOMATION_PRESET_MACRO_IMAGE_ROOM;
    } else {
        return;
    }

    mAutomationMetrics.presetsPlayed.push_back(presetMacro);
}

void RecordMacroPage::reportAllMetrics() {
    EmulatorAutomation metrics;

    metrics.set_macro_playback_count(mAutomationMetrics.playbackCount);
    metrics.set_preview_replay_count(mAutomationMetrics.previewReplayCount);
    metrics.set_total_duration_ms(mAutomationMetrics.totalDurationMs);
    for (auto presetMacro : mAutomationMetrics.presetsPlayed) {
        metrics.add_played_preset_macro(presetMacro);
    }
    metrics.set_record_macro_count(mAutomationMetrics.recordMacroCount);
    metrics.set_delete_macro_count(mAutomationMetrics.deleteMacroCount);
    metrics.set_edit_macro_count(mAutomationMetrics.editMacroCount);

    MetricsReporter::get().report(
            [metrics](android_studio::AndroidStudioEvent* event) {
                auto automation =
                        event->mutable_emulator_details()->mutable_automation();
                automation->CopyFrom(metrics);
            });
}

void RecordMacroPage::on_recButton_clicked() {
    if (!mRecording) {
        if (mVideoPlayer && mVideoPlayer->isRunning()) {
            mVideoPlayer->stop();
        }
        setMacroUiState(MacroUiState::PreRecording);
    } else {
        stopRecording();
    }
}

void RecordMacroPage::startRecording() {
    disableMacroItems();
    mRecording = true;
    emit setRecordingStateSignal(mRecording);

    const std::string macrosLocation = getCustomMacrosDirectory();
    const std::string placeholderPath =
            PathUtils::join(macrosLocation, "placeholder_name");

    auto result = sAutomationAgent->startRecording(placeholderPath);
    if (result.err()) {
        std::ostringstream errString;
        errString << result.unwrapErr();
        displayErrorBox(errString.str());
        return;
    }

    setMacroUiState(MacroUiState::Recording);
}

void RecordMacroPage::stopRecording() {
    mTimer.stop();
    enableMacroItems();
    mRecording = false;
    emit setRecordingStateSignal(mRecording);

    auto result = sAutomationAgent->stopRecording();
    if (result.err()) {
        std::ostringstream errString;
        errString << result.unwrapErr();
        displayErrorBox(errString.str());
        return;
    }

    std::string newName = displayNameMacroBox();

    const std::string macrosLocation = getCustomMacrosDirectory();
    const std::string oldPath =
            PathUtils::join(macrosLocation, "placeholder_name");
    if (!newName.empty()) {
        // Rename file.
        sAutomationAgent->setMacroName(newName, oldPath);
        newName = QDateTime::currentDateTime()
                          .toString("yyyyMMdd-hhmmss-")
                          .toUtf8()
                          .constData() +
                  newName;
        newName.append(".emu-macro");
        const std::string newPath = PathUtils::join(macrosLocation, newName);
        if (std::rename(oldPath.c_str(), newPath.c_str()) != 0) {
            LOG(ERROR) << "Renaming file failed.";
            std::remove(oldPath.c_str());
        } else {
            createMacroItem(newName, false);
            reportMacroRecorded();
        }
    } else {
        // Delete file.
        std::remove(oldPath.c_str());
    }

    setMacroUiState(MacroUiState::Waiting);
}

void RecordMacroPage::setRecordState() {
    if (!mRecording) {
        mUi->recButton->setText(tr("RECORD NEW "));
        mUi->recButton->setIcon(getIconForCurrentTheme("recordCircle"));
        mUi->recButton->setProperty("themeIconName", "recordCircle");
        mUi->recButton->setIconSize(QSize(26, 18));
    }
    mUi->stackedWidget->setCurrentIndex(0);

    switch (mState) {
        case MacroUiState::Waiting: {
            if (!mMacroPlaying) {
                mUi->recButton->setEnabled(true);
            }
            break;
        }
        case MacroUiState::Selected: {
            mUi->recButton->setEnabled(true);
            break;
        }
        case MacroUiState::PreviewFinished: {
            mUi->recButton->setEnabled(true);
            break;
        }
        case MacroUiState::Playing: {
            mUi->recButton->setEnabled(false);
            break;
        }
        case MacroUiState::PreRecording: {
            mUi->stackedWidget->setCurrentIndex(1);
            mUi->recButton->setEnabled(false);
            mUi->recordingLabel->setText(tr("Recording will start in"));
            mUi->cancelButton->show();
            // Update every second
            mUi->countLabel->show();
            mUi->countLabel->setText(tr("3"));
            mSec = 0;
            mTimer.start(1000);
            break;
        }
        case MacroUiState::Recording: {
            mUi->stackedWidget->setCurrentIndex(1);
            mUi->countLabel->hide();
            mUi->cancelButton->hide();
            mUi->recButton->setEnabled(true);
            mUi->recButton->setIcon(getIconForCurrentTheme("stop_red"));
            mUi->recButton->setProperty("themeIconName", "stop_red");
            mUi->recButton->setText(tr("STOP RECORDING "));
            // Update every second
            mUi->recordingLabel->setText(tr(SECONDS_RECORDING).arg(0));
            mSec = 0;
            mTimer.start(1000);
            break;
        }
    }
}

std::string RecordMacroPage::getCustomMacrosDirectory() {
    const std::string appDataLocation =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                    .toUtf8()
                    .constData();
    const std::string macrosLocation =
            PathUtils::join(appDataLocation, "macros");
    QDir dir(QString::fromStdString(macrosLocation));
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return macrosLocation;
}

void RecordMacroPage::displayErrorBox(const std::string& errorStr) {
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText(tr("An error ocurred."));
    msgBox.setInformativeText(errorStr.c_str());
    msgBox.setDefaultButton(QMessageBox::Save);
    msgBox.exec();
}

std::string RecordMacroPage::displayNameMacroBox() {
    QWidget* placeholderWidget = new QWidget;
    QVBoxLayout* dialogLayout = new QVBoxLayout(placeholderWidget);

    dialogLayout->addWidget(new QLabel(tr("Macro name")));
    QLineEdit* name = new QLineEdit(this);
    name->setMaxLength(32);
    name->setText(tr("My Macro"));
    name->selectAll();
    dialogLayout->addWidget(name);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
            QDialogButtonBox::Save | QDialogButtonBox::Cancel, Qt::Horizontal);
    dialogLayout->addWidget(buttonBox);

    QDialog nameDialog(this);

    connect(buttonBox, SIGNAL(rejected()), &nameDialog, SLOT(reject()));
    connect(buttonBox, SIGNAL(accepted()), &nameDialog, SLOT(accept()));

    nameDialog.setWindowTitle(tr("Enter a name for your recording"));
    nameDialog.setLayout(dialogLayout);
    nameDialog.resize(300, nameDialog.height());

    int selection;
    QString newName;
    QRegExp re("\\w+");
    // Check that name is [a-zA-Z0-9_] after trimming.
    do {
        selection = nameDialog.exec();
        newName = name->text();
        newName.replace("_", " ");
        newName = newName.simplified();
        newName.replace(" ", "_");
    } while (selection == QDialog::Accepted && !re.exactMatch(newName));

    if (selection == QDialog::Rejected) {
        return "";
    }
    return newName.toUtf8().constData();
}

void RecordMacroPage::loadCustomMacros() {
    const std::string macrosPath = getCustomMacrosDirectory();
    std::vector<std::string> macroFileNames =
            System::get()->scanDirEntries(macrosPath);

    for (auto& macroName : macroFileNames) {
        // Extension check.
        QFileInfo fi(tr(macroName.c_str()));
        if (fi.suffix() == "emu-macro") {
            createMacroItem(macroName, false);
        }
    }
}

void RecordMacroPage::createMacroItem(std::string& macroName, bool isPreset) {
    QListWidgetItem* listItem = new QListWidgetItem(mUi->macroList);
    QVariant macroNameData(QString::fromStdString(macroName));
    listItem->setData(Qt::UserRole, macroNameData);

    RecordMacroSavedItem* macroSavedItem = new RecordMacroSavedItem();
    if (isPreset) {
        macroSavedItem->setEditButtonEnabled(false);
        macroSavedItem->setIsPreset(true);
        mDescriptions[macroName] = tr("Preset macro");
        macroSavedItem->setDisplayInfo(mDescriptions[macroName]);
        macroSavedItem->setDisplayTime(mLengths[macroName]);
    } else {
        const std::string macrosLocation = getCustomMacrosDirectory();
        const std::string filePath = PathUtils::join(macrosLocation, macroName);

        std::pair<uint64_t, uint64_t> metadata =
                sAutomationAgent->getMetadata(filePath);
        // Duration
        uint64_t durationNs = metadata.first;
        int durationS = durationNs / 1000000000;
        QString qs = tr("%1:%2%3")
                             .arg(durationS / 60)
                             .arg(durationS % 60 > 9 ? "" : "0")
                             .arg(durationS % 60);
        mLengths[macroName] = qs;

        // Description
        QDateTime dt;
        dt.setMSecsSinceEpoch(metadata.second);
        mDescriptions[macroName] = dt.toString("MM-dd-yyyy hh:mm:ss");

        macroSavedItem->setDisplayTime(mLengths[macroName]);
        macroSavedItem->setDisplayInfo(mDescriptions[macroName]);
        macroName = sAutomationAgent->getMacroName(filePath);
        connect(macroSavedItem,
                SIGNAL(editButtonClickedSignal(CCListItem*)), this,
                SLOT(editButtonClicked(CCListItem*)));
    }

    listItem->setSizeHint(QSize(0, 50));
    mUi->macroList->addItem(listItem);
    mUi->macroList->setItemWidget(listItem, macroSavedItem);

    // Set name after item widget is placed for correct elided text.
    std::replace(macroName.begin(), macroName.end(), '_', ' ');
    macroSavedItem->setName(macroName.c_str());
}

bool RecordMacroPage::isPreviewAvailable(const std::string& macroName) {
    // Only check if it is custom for the moment.
    return macroName.find(".emu-macro") == std::string::npos;
}

void RecordMacroPage::editButtonClicked(CCListItem* item) {
    auto* macroItem = reinterpret_cast<RecordMacroSavedItem*>(item);
    RecordMacroEditDialog editDialog(this);
    editDialog.setCurrentName(QString::fromStdString(macroItem->getName()));

    int selection = editDialog.exec();

    if (selection == ButtonClicked::Delete) {
        deleteMacroItem(macroItem);
    } else if (selection == QDialog::Accepted) {
        // Update macro item.
        QString newName = editDialog.getNewName();
        macroItem->setName(newName);

        // Update file header.
        const std::string macroName = newName.toUtf8().constData();
        QListWidgetItem* listItem = findListItemFromWidget(macroItem);
        if (!listItem) {
            displayErrorBox("Edit failed.");
            return;
        }
        const std::string macrosLocation = getCustomMacrosDirectory();
        const std::string path =
                PathUtils::join(macrosLocation, getMacroNameFromItem(listItem));
        sAutomationAgent->setMacroName(macroName, path);
        reportMacroEdited();
    }
}

QListWidgetItem* RecordMacroPage::findListItemFromWidget(
        RecordMacroSavedItem* macroItem) {
    for (int i = 0; i < mUi->macroList->count(); ++i) {
        QListWidgetItem* item = mUi->macroList->item(i);
        if (macroItem == getItemWidget(item)) {
            return item;
        }
    }
    return NULL;
}

void RecordMacroPage::deleteMacroItem(RecordMacroSavedItem* macroItem) {
    QString macroName = QString::fromStdString(macroItem->getName());
    QMessageBox msgBox(QMessageBox::Question, tr("Delete macro"),
                       tr("Do you want to permanently delete<br>macro \"%1\"?")
                               .arg(macroName),
                       QMessageBox::Cancel, this);
    QPushButton* deleteButton = msgBox.addButton(QMessageBox::Ok);
    deleteButton->setText(tr("Delete"));

    int selection = msgBox.exec();
    if (selection == QMessageBox::Ok) {
        QListWidgetItem* listItem = findListItemFromWidget(macroItem);
        const std::string name = getMacroNameFromItem(listItem);
        mUi->macroList->model()->removeRow(mUi->macroList->row(listItem));
        const std::string macrosLocation = getCustomMacrosDirectory();
        const std::string path = PathUtils::join(macrosLocation, name);
        if (std::remove(path.c_str()) != 0) {
            displayErrorBox("Deletion failed.");
        } else {
            reportMacroDeleted();
        }
    }
}

void RecordMacroPage::on_cancelButton_clicked() {
    mTimer.stop();
    enableMacroItems();
    setMacroUiState(MacroUiState::Waiting);
}

void RecordMacroPage::enablePresetMacros(bool enable) {
    // Reset all the possible states.
    mTimer.stop();
    mRecording = false;
    emit setRecordingStateSignal(mRecording);
    if (mState == MacroUiState::Playing) {
        playbackFinished();
    }
    sAutomationAgent->reset();
    if (mVideoPlayer && mVideoPlayer->isRunning()) {
        mVideoPlayer->stop();
    }

    if (enable) {
        loadUi();
    } else {
        // Iterate top to bottom to not skip items when deleting.
        for (int i = mUi->macroList->count() - 1; i >= 0; --i) {
            QListWidgetItem* item = mUi->macroList->item(i);
            RecordMacroSavedItem* macroItem = getItemWidget(item);
            if (macroItem->isPreset()) {
                mUi->macroList->takeItem(i);
            }
        }
    }
    enableMacroItems();
    setMacroUiState(MacroUiState::Waiting);
}

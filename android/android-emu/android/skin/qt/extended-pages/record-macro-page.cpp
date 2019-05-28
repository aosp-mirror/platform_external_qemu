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
#include "android/featurecontrol/FeatureControl.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/video-player/QtVideoPlayerNotifier.h"

#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegExp>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <iostream>
#include <sstream>

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

    // Descriptions as QStrings have to be initialized here to use tr().
    mDescriptions = {
            {"Reset_position", tr("Resets sensors to default.")},
            {"Track_horizontal_plane", tr("Circles around the rug.")},
            {"Track_vertical_plane", tr("Looks at the wall next to the tv.")},
            {"Walk_to_image_room", tr("Moves to the dining room.")}};

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

    QObject::connect(&mTimer, &QTimer::timeout, this,
                     &RecordMacroPage::updateElapsedTime);

    QObject::connect(this, &RecordMacroPage::playbackFinishedSignal, this,
                     &RecordMacroPage::playbackFinished);

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

void RecordMacroPage::on_macroList_currentItemChanged(
        QListWidgetItem* current,
        QListWidgetItem* previous) {
    if (current && previous) {
        RecordMacroSavedItem* macroSavedItem = getItemWidget(previous);
        macroSavedItem->macroSelected(false);
    }
    if (current) {
        RecordMacroSavedItem* macroSavedItem = getItemWidget(current);
        macroSavedItem->macroSelected(true);
    }
}

void RecordMacroPage::on_macroList_itemPressed(QListWidgetItem* listItem) {
    const std::string macroName = getMacroNameFromItem(listItem);

    if (mMacroPlaying && mCurrentMacroName == macroName) {
        setMacroUiState(MacroUiState::Playing);
        showPreviewFrame(macroName);
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
        case MacroUiState::Recording: {
            mUi->previewLabel->setText(tr("Macro recording ongoing"));
            mUi->previewLabel->show();
            mUi->previewOverlay->show();
            mUi->replayIcon->hide();
            mUi->playStopButton->setEnabled(false);
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
    if (macroSavedItem->getIsPreset()) {
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

    if (macroSavedItem->getIsPreset()) {
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
    setMacroUiState(MacroUiState::PreviewFinished);

    QListWidgetItem* listItem = mUi->macroList->selectedItems().first();
    const std::string macroName = getMacroNameFromItem(listItem);
    showPreviewFrame(macroName);
}

void RecordMacroPage::mousePressEvent(QMouseEvent* event) {
    if (mState == MacroUiState::PreviewFinished) {
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
        macroItem->editEnabled(false);
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
        if (!macroItem->getIsPreset()) {
            macroItem->editEnabled(true);
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
    mMacroItemPlaying->setDisplayTime(getTimerString(mSec));
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

    MetricsReporter::get().report(
            [metrics](android_studio::AndroidStudioEvent* event) {
                auto automation =
                        event->mutable_emulator_details()->mutable_automation();
                automation->CopyFrom(metrics);
            });
}

void RecordMacroPage::on_recButton_clicked() {
    if (!mRecording) {
        startRecording();
    } else {
        stopRecording();
    }
}

void RecordMacroPage::startRecording() {
    if (mVideoPlayer && mVideoPlayer->isRunning()) {
        mVideoPlayer->stop();
    }
    disableMacroItems();
    mRecording = true;

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
    enableMacroItems();
    mRecording = false;

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
    }

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
        case MacroUiState::Recording: {
            mUi->recButton->setIcon(getIconForCurrentTheme("stop_red"));
            mUi->recButton->setProperty("themeIconName", "stop_red");
            mUi->recButton->setText(tr("STOP RECORDING "));
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
        macroSavedItem->editEnabled(false);
        macroSavedItem->setIsPreset(true);
        macroSavedItem->setDisplayInfo(mDescriptions[macroName]);
        macroSavedItem->setDisplayTime(mLengths[macroName]);
        macroName.append(" (Preset macro)");
        std::replace(macroName.begin(), macroName.end(), '_', ' ');
        macroSavedItem->setName(macroName.c_str());
    } else {
        const std::string macrosLocation = getCustomMacrosDirectory();
        const std::string filePath = PathUtils::join(macrosLocation, macroName);
        uint64_t durationNs = sAutomationAgent->getDurationNs(filePath);
        int durationS = durationNs / 1000000000;
        QString qs = tr("%1:%2%3")
                             .arg(durationS / 60)
                             .arg(durationS % 60 > 9 ? "" : "0")
                             .arg(durationS % 60);
        mLengths[macroName] = qs;

        macroSavedItem->setDisplayTime(mLengths[macroName]);
        macroSavedItem->setName(
                sAutomationAgent->getMacroName(filePath).str().c_str());
        connect(macroSavedItem,
                SIGNAL(editButtonClickedSignal(RecordMacroSavedItem*)), this,
                SLOT(editButtonClicked(RecordMacroSavedItem*)));
    }

    listItem->setSizeHint(QSize(mUi->macroList->width(), 50));

    mUi->macroList->addItem(listItem);
    mUi->macroList->setItemWidget(listItem, macroSavedItem);
}

bool RecordMacroPage::isPreviewAvailable(const std::string& macroName) {
    // Only check if it is custom for the moment.
    return macroName.find(".emu-macro") == std::string::npos;
}

void RecordMacroPage::editButtonClicked(RecordMacroSavedItem* macroItem) {
    // Edit options will pop up for the specific list item.
    deleteMacroItem(macroItem);
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
        }
    }
}

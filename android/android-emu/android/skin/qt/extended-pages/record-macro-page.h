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

#pragma once

#include "ui_record-macro-page.h"

#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/recording/video/player/VideoPlayer.h"
#include "android/skin/qt/extended-pages/record-macro-saved-item.h"
#include "android/skin/qt/video-player/VideoInfo.h"

#include <QTime>
#include <QTimer>
#include <QWidget>
#include <memory>
#include <unordered_map>

struct QAndroidAutomationAgent;

class RecordMacroPage : public QWidget {
    Q_OBJECT

public:
    enum class MacroUiState { Waiting, Selected, PreviewFinished, Playing, Recording };

    explicit RecordMacroPage(QWidget* parent = 0);
    ~RecordMacroPage();

    static void setAutomationAgent(const QAndroidAutomationAgent* agent);
    static void stopCurrentMacro();

    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void on_playStopButton_clicked();
    void on_macroList_currentItemChanged(QListWidgetItem* current,
                                         QListWidgetItem* previous);
    void on_macroList_itemPressed(QListWidgetItem* listItem);
    void on_macroList_itemSelectionChanged();
    void on_recButton_clicked();
    void updatePreviewVideoView();
    void previewVideoPlayingFinished();
    void updateElapsedTime();
    void editButtonClicked(RecordMacroSavedItem* macroItem);

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void playbackFinishedSignal();

private:
    void loadUi();
    std::string getMacrosDirectory();
    std::string getMacroPreviewsDirectory();
    void setMacroUiState(MacroUiState state);
    void playButtonClicked(QListWidgetItem* listItem);
    void stopButtonClicked(QListWidgetItem* listItem);
    void showPreview(const std::string& macroName);
    RecordMacroSavedItem* getItemWidget(QListWidgetItem* listItem);
    void disableMacroItemsExcept(QListWidgetItem* listItem);
    void disableMacroItems();
    void enableMacroItems();
    void showPreviewFrame(const std::string& macroName);
    std::string getMacroNameFromItem(QListWidgetItem* listItem);
    QString getTimerString(int seconds);
    void reportMacroPlayed();
    void reportPreviewPlayedAgain();
    void reportTotalDuration();
    void reportPresetMacroPlayed(const std::string& macroName);
    void reportAllMetrics();
    void startRecording();
    void stopRecording();
    std::string getCustomMacrosDirectory();
    void displayErrorBox(const std::string& errorStr);
    std::string displayNameMacroBox();
    void loadCustomMacros();
    void createMacroItem(std::string& macroName, bool isPreset);
    bool isPreviewAvailable(const std::string& macroName);
    QListWidgetItem* findListItemFromWidget(RecordMacroSavedItem* macroItem);
    void deleteMacroItem(RecordMacroSavedItem* macroItem);

    // Behind feature flag.
    void setRecordState();
    bool mRecordEnabled = false;

    bool mRecording = false;
    bool mMacroPlaying = false;
    std::string mCurrentMacroName;
    std::unique_ptr<android::videoplayer::VideoPlayer> mVideoPlayer;
    std::unique_ptr<android::videoplayer::VideoInfo> mVideoInfo;
    std::unique_ptr<Ui::RecordMacroPage> mUi;
    RecordMacroSavedItem* mMacroItemPlaying;
    QListWidgetItem* mListItemPlaying;
    std::unordered_map<std::string, QString> mLengths;
    std::unordered_map<std::string, QString> mDescriptions;
    MacroUiState mState = MacroUiState::Waiting;
    int mSec = 0;
    QTimer mTimer;

    void emitPlaybackFinished();
    void playbackFinished();

    static RecordMacroPage* sInstance;
    static const QAndroidAutomationAgent* sAutomationAgent;

    // Aggregate metrics to determine how the macro page is used.
    // Metrics are collected while the tab is visible, and reported when the
    // session ends.
    uint64_t mReportWindowStartUs = 0;
    uint32_t mReportWindowCount = 0;
    QTime mElapsedTimeTimer;

    struct AutomationMetrics {
        uint64_t totalDurationMs = 0;
        uint64_t playbackCount = 0;
        uint64_t previewReplayCount = 0;
        std::vector<android_studio::EmulatorAutomation::
                            EmulatorAutomationPresetMacro>
                presetsPlayed = {};
    };

    AutomationMetrics mAutomationMetrics;
};

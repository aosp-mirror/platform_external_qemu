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

#include "android/recording/video/player/VideoPlayer.h"
#include "android/skin/qt/extended-pages/record-macro-saved-item.h"
#include "android/skin/qt/video-player/VideoInfo.h"

#include <QWidget>
#include <memory>
#include <unordered_map>

struct QAndroidAutomationAgent;

class RecordMacroPage : public QWidget {
    Q_OBJECT

public:
    enum class MacroUiState { Waiting, Selected, PreviewFinished, Playing };

    explicit RecordMacroPage(QWidget* parent = 0);

    static void setAutomationAgent(const QAndroidAutomationAgent* agent);

private slots:
    void on_playStopButton_clicked();
    void on_macroList_currentItemChanged(QListWidgetItem* current,
                                         QListWidgetItem* previous);
    void on_macroList_itemPressed(QListWidgetItem* listItem);
    void on_macroList_itemSelectionChanged();
    void updatePreviewVideoView();
    void previewVideoPlayingFinished();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void loadUi();
    std::string getMacrosDirectory();
    std::string getMacroPreviewsDirectory();
    void setMacroUiState(MacroUiState state);
    void playButtonClicked(QListWidgetItem* listItem);
    void stopButtonClicked(QListWidgetItem* listItem);
    void showPreview(const std::string& previewName);
    RecordMacroSavedItem* getItemWidget(QListWidgetItem* listItem);
    void disableMacroItemsExcept(QListWidgetItem* listItem);
    void enableMacroItems();
    void showPreviewFrame(const std::string& previewName);
    std::string getMacroNameFromItem(QListWidgetItem* listItem);

    bool mMacroPlaying = false;
    std::string mCurrentMacroName;
    std::unique_ptr<android::videoplayer::VideoPlayer> mVideoPlayer;
    std::unique_ptr<android::videoplayer::VideoInfo> mVideoInfo;
    std::unique_ptr<Ui::RecordMacroPage> mUi;
    std::unordered_map<std::string, QString> mDescriptions;
    MacroUiState mState = MacroUiState::Waiting;

    static const QAndroidAutomationAgent* sAutomationAgent;
};

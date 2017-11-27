// Copyright (C) 2017 The Android Open Source Project
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

#include "ui_record-screen-page.h"

#include "android/screen-recorder.h"
#include "android/skin/qt/video-player/VideoPlayer.h"
#include "android/skin/qt/video-player/VideoPlayerNotifier.h"
#include "android/skin/qt/video-player/VideoPlayerWidget.h"

#include <QTimer>
#include <QWidget>
#include <memory>

struct QAndroidRecordScreenAgent;

Q_DECLARE_METATYPE(RecordStopStatus);

class RecordScreenPage : public QWidget {
    Q_OBJECT
public:
    enum class RecordState { Ready, Recording, Stopping, Stopped, Converting, Playing };

    explicit RecordScreenPage(QWidget* parent = 0);
    ~RecordScreenPage();

    void setRecordScreenAgent(const QAndroidRecordScreenAgent* agent);
    void updateTheme();
    void emitRecordingStopped(RecordStopStatus status);
    static bool removeFileIfExists(const QString& file);

signals:
    void recordingStopped(RecordStopStatus status);

private slots:
    void on_rec_playStopButton_clicked();
    void on_rec_recordButton_clicked();
    void on_rec_saveButton_clicked();
    void updateElapsedTime();
    void slot_recordingStopped(RecordStopStatus status);
    void convertingStarted();
    void convertingFinished(bool success);
    void updateVideoView();
    void videoPlayingFinished();

public slots:

public:
    void setRecordState(RecordState r);

private:
    static const char kTmpMediaName[]; // tmp name for unsaved media file
    std::string mTmpFilePath;
    std::unique_ptr<Ui::RecordScreenPage> mUi;
    std::unique_ptr<android::videoplayer::VideoPlayerWidget> mVideoWidget;
    std::unique_ptr<android::videoplayer::VideoPlayerNotifier> mVideoPlayerNotifier;
    std::unique_ptr<android::videoplayer::VideoPlayer> mVideoPlayer;
    const QAndroidRecordScreenAgent* mRecordScreenAgent;
    RecordState mState;
    QTimer mTimer;
    int mSec;  // number of elapsed seconds
};

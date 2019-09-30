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

#include <qmetatype.h>                          // for Q_DECLARE_METATYPE
#include <qobjectdefs.h>                        // for slots, Q_OBJECT, signals
#include <QString>                              // for QString
#include <QTimer>                               // for QTimer
#include <QWidget>                              // for QWidget
#include <memory>                               // for unique_ptr
#include <string>                               // for string

#include "android/recording/screen-recorder.h"  // for RecordingStatus

class QObject;
class QString;
class QWidget;
namespace Ui {
class RecordScreenPage;
}  // namespace Ui
namespace android {
namespace videoplayer {
class VideoInfo;
class VideoPlayer;
}  // namespace videoplayer
}  // namespace android
struct QAndroidRecordScreenAgent;

Q_DECLARE_METATYPE(RecordingStatus);

class RecordScreenPage : public QWidget {
    Q_OBJECT
public:
    enum class RecordUiState {
        Ready,
        Starting,
        Recording,
        Stopping,
        Stopped,
        Converting,
        Playing
    };

    explicit RecordScreenPage(QWidget* parent = 0);
    ~RecordScreenPage();

    void updateTheme();
    void emitRecordingStatusChange(RecordingStatus status);
    static void setRecordScreenAgent(const QAndroidRecordScreenAgent* agent);
    static bool removeFileIfExists(const QString& file);

signals:
    void recordingStatusChange(RecordingStatus status);

private slots:
    void on_rec_playStopButton_clicked();
    void on_rec_recordButton_clicked();
    void on_rec_recordAgainButton_clicked();
    void on_rec_saveButton_clicked();
    void updateElapsedTime();
    void slot_recordingStatusChange(RecordingStatus status);
    void convertingStarted();
    void convertingFinished(bool success);
    void updateVideoView();
    void videoPlayingFinished();

public slots:

public:
    void setRecordUiState(RecordUiState r);

private:
    static const char kTmpMediaName[]; // tmp name for unsaved media file
    static const QAndroidRecordScreenAgent* sRecordScreenAgent;

    std::string mTmpFilePath;
    std::unique_ptr<Ui::RecordScreenPage> mUi;
    std::unique_ptr<android::videoplayer::VideoPlayer> mVideoPlayer;
    std::unique_ptr<android::videoplayer::VideoInfo> mVideoInfo;
    RecordUiState mState;
    QTimer mTimer;
    int mSec;  // number of elapsed seconds
};

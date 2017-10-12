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
#include <QTimer>
#include <QWidget>
#include <memory>

struct QAndroidRecordScreenAgent;
class RecordScreenPage : public QWidget {
    Q_OBJECT
public:
    enum class RecordState { Ready, Recording, Stopping, Stopped, Converting };

    explicit RecordScreenPage(QWidget* parent = 0);
    ~RecordScreenPage();

    void setRecordScreenAgent(const QAndroidRecordScreenAgent* agent);
    void updateTheme();

signals:

private slots:
    void on_rec_playStopButton_clicked();
    void on_rec_recordButton_clicked();
    void on_rec_saveButton_clicked();
    void updateElapsedTime();
    void stopRecordingStarted();
    void stopRecordingFinished(bool success);
    void convertingStarted();
    void convertingFinished(bool success);

public slots:

public:
    void setRecordState(RecordState r);

private:
    static const char kTmpMediaName[]; // tmp name for unsaved media file
    std::string mTmpFilePath;
    std::unique_ptr<Ui::RecordScreenPage> mUi;
    const QAndroidRecordScreenAgent* mRecordScreenAgent;
    RecordState mState;
    QTimer mTimer;
    int mSec;  // number of elapsed seconds
};

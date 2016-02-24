// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/skin/qt/ui-event-recorder.h"

#include <memory>
#include <vector>

#include <QString>
#include <QTimer>
#include <QWidget>

class UIEventRecordPlayer : public UIEventRecorder<std::vector> {
    Q_OBJECT

public:
    explicit UIEventRecordPlayer(EventCapturer* ecap);
    ~UIEventRecordPlayer();

    bool init(const char* recordPath,
              const char* replayPath,
              const char* delay);
    bool setRecordPath(const char* recordPath);
    bool setReplayPath(const char* replayPath);
    bool setReplayDelay(const char* delay);

    void save();
    void load();

    void follow(QWidget* wid);
    void unfollow(QWidget* wid);

    void start();

public slots:
    void stop();
    void replayEvent();

signals:
    void replayComplete();

private:
    bool inRecordMode();
    bool inReplayMode();
    void startReplaying();

    std::map<std::string, QWidget*> mFollowed;
    QString mRecordPath;
    QString mReplayPath;
    QTimer mTimer;
    unsigned int mHead;
    int mReplayDelay;

    DISALLOW_COPY_AND_ASSIGN(UIEventRecordPlayer);
};

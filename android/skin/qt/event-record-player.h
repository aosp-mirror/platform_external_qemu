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

#include <ios>
#include <fstream>
#include <string>
#include <vector>

#include <QTimer>
#include <QWidget>

/******************************************************************************/

class EventRecordPlayer {
public:
    EventRecordPlayer();
    virtual ~EventRecordPlayer() = default;

    virtual bool setRecordsFile(const char* filePath) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void print() const = 0;

    void follow(QWidget* const wid);
    void unfollow(const QWidget* const wid);

protected:
    bool openAndSetRecordsFile(const char* filePath,
                               std::ios_base::openmode mode);

    std::map<std::string, QWidget*> mFollowed;
    std::fstream mFileStream;

    DISALLOW_COPY_AND_ASSIGN(EventRecordPlayer);
};

/******************************************************************************/

class EventPlayer : public QObject, public EventRecordPlayer {
    Q_OBJECT

public:
    EventPlayer();
    virtual ~EventPlayer() override = default;

    bool setRecordsFile(const char* filePath) override;
    bool setReplayStartDelayMilliSec(const char* msec);
    void start() override;
    void print() const override;

public slots:
    void stop() override;
    void replayEvent();

signals:
    void replayComplete();

private:
    std::vector<EventRecord> mContainer;
    QTimer mTimer;
    unsigned int mReplayStartDelayMilliSec;
    unsigned int mHead;

    DISALLOW_COPY_AND_ASSIGN(EventPlayer);
};

/******************************************************************************/

class EventRecorder : public UIEventRecorder<std::vector>,
                      public EventRecordPlayer {
    Q_OBJECT

public:
    explicit EventRecorder(EventCapturer* ecap);
    virtual ~EventRecorder() override = default;

    void print() const override;

    bool setRecordsFile(const char* filePath) override;
    void start() override;
    void stop() override;

    DISALLOW_COPY_AND_ASSIGN(EventRecorder);
};

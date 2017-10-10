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

#include <QObject>

struct QAndroidRecordScreenAgent;

class StopRecordingTask : public QObject {
    Q_OBJECT
public:
    StopRecordingTask(const QAndroidRecordScreenAgent* agent);

public slots:
    void run();

signals:
    void started();
    void finished(bool success);

private:
    const QAndroidRecordScreenAgent* mRecordScreenAgent;
};

class ConvertingTask : public QObject {
    Q_OBJECT
public:
    ConvertingTask(const std::string& startFilename,
                   const std::string& endFilename);

public slots:
    void run();

signals:
    void started();
    void finished(bool success);

private:
    std::string mStartFilename;
    std::string mEndFilename;
};

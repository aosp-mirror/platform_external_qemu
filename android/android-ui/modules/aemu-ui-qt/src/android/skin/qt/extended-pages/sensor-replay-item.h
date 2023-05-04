// Copyright (C) 2020 The Android Open Source Project
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

#include <qobjectdefs.h>  // for Q_OBJECT
#include <QListWidget>    // for QListWidget
#include <QString>        // for QString
#include <memory>         // for unique_ptr
#include <string>         // for string

#include "android/sensor_replay/sensor_session_playback.h"
#include "android/skin/qt/common-controls/cc-list-item.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/stylesheet.h"

class SensorReplayItem : public CCListItem {
    Q_OBJECT
public:
    explicit SensorReplayItem(
            QListWidget* const listWidget,
            const QString& name,
            const QString& path,
            android::sensorsessionplayback::SensorSessionPlayback*
                    sensorSessionPlayback,
            QWidget* parent = nullptr);
    ~SensorReplayItem();

    // Return full file path
    QString getPath() const;

    // Remove the sensor replay item from the history list
    void removeFromListWidget();

    android::sensorsessionplayback::SensorSessionPlayback*
            getSensorSessionPlayback();

private:
    QListWidgetItem* mListItem;
    QListWidget* const mListWidget;
    QString mPath;
    android::sensorsessionplayback::SensorSessionPlayback*
            mSensorSessionPlayback = nullptr;
};

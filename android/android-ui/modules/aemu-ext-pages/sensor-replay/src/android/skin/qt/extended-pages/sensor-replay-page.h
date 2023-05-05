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

#include "ui_sensor-replay-page.h"

#include <qobjectdefs.h>  // for Q_OBJECT
#include <QLabel>
#include <QString>  // for QString
#include <QWidget>  // for QWidget
#include <memory>   // for unique_ptr

#include "android/sensor_replay/sensor_session_playback.h"
#include "android/skin/qt/common-controls/cc-list-item.h"

class QEvent;
class QObject;
class QMessageBox;
class QWidget;
class QLabel;
class QObject;
class SensorReplayItem;
struct QAndroidLocationAgent;
struct QCarDataAgent;
struct QAndroidSensorsAgent;

namespace emulator {
class EmulatorMessage;
}

class SensorReplayPage : public QWidget {
    Q_OBJECT

public:
    typedef int64_t DurationNs;

    explicit SensorReplayPage(QWidget* parent = nullptr);

    static void setAgent(const QCarDataAgent* carDataAgent,
                         const QAndroidLocationAgent* locationAgent,
                         const QAndroidSensorsAgent* sensorAgent);

private slots:
    void on_sensor_loadSensorButton_clicked();
    void on_sensor_playStopButton_clicked();
    void on_sensor_savedSensorRecordList_currentItemChanged(
            QListWidgetItem* current,
            QListWidgetItem* previous);
    void adjustPlaybackSlider();
    void editButtonClicked(CCListItem* item);

private:
    bool eventFilter(QObject* obj, QEvent* ev) override;

    void sensorReplayStart();
    void sensorReplayStop();
    void registerSensorSessionPlaybackCallback(
            android::sensorsessionplayback::SensorSessionPlayback*
                    sensorSessionPlayback);
    float getPlaybackSpeed();

    QString getPreviewText(emulator::SensorSession::SessionMetadata metadata,
                           std::string connector);
    QString getAppVersion(emulator::SensorSession::SessionMetadata metadata);
    QString toTimeString(DurationNs time);
    QString getConnectedTimeString(DurationNs timeline1,
                                   DurationNs timeline2,
                                   const QString& connector);
    QString formatTimeRange(DurationNs from, DurationNs to);
    QString formatTimeProgress(DurationNs position, DurationNs total);
    void addTimeLineTableRow(QTableWidget* table,
                             const QString& timeline,
                             const QString& eventCount,
                             int tableRow);
    void showSensorRecordDetail(bool show);
    void updateTimeLine(DurationNs timestamp);
    void updatePanel(android::sensorsessionplayback::SensorSessionPlayback*
                             sensorSessionPlayback,
                     QString fullPath);

    void addSensorRecord(QString fullPath,
                         QString fileName,
                         android::sensorsessionplayback::SensorSessionPlayback*
                                 sensorSessionPlayback);
    void deleteRecord(SensorReplayItem* item);
    bool findSensorRecord(QString itemPath);

    SensorReplayItem* getCurrentSensorRecordItem();

    static const QCarDataAgent* sCarDataAgent;
    static const QAndroidLocationAgent* sLocationAgent;
    static const QAndroidSensorsAgent* sSensorAgent;

    static void sendCarEmulatorMessageLogged(
            const emulator::EmulatorMessage& msg,
            const std::string& log);
    static emulator::EmulatorMessage makePropMsg(
            int propId,
            emulator::SensorSession::SensorRecord::CarPropertyValue val);
    static int convertSensorTypeToSensorId(int sensorType);

    bool isPlaying;
    std::unique_ptr<Ui::SensorReplayPage> mUi;
    QMessageBox* mSensorInfoDialog;
    DurationNs totalTime;
};

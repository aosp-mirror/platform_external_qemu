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

#include <qobjectdefs.h>       // for Q_OBJECT
#include <QString>           // for QString
#include <QWidget>                                          // for QWidget
#include <QLabel>
#include <memory>                                           // for unique_ptr

#include "android/car_sensor_replay/sensor_session_playback.h"
#include "android/skin/qt/common-controls/cc-list-item.h"

#include "ui_sensor-replay-page.h"  // for SensorReplayPage

class QEvent;
class QObject;
class QMessageBox;
class QWidget;
class QLabel;
class QObject;
class SensorReplayItem;
struct QCarDataAgent;


namespace emulator {
class EmulatorMessage;
}

class SensorReplayPage : public QWidget {
    Q_OBJECT

public:
    explicit SensorReplayPage(QWidget* parent = nullptr);

    static void setCarDataAgent(const QCarDataAgent* agent);

private slots:
    void on_sensor_LoadSensorButton_clicked();
    void on_sensor_playStopButton_clicked();
    void editButtonClicked(CCListItem* item);
    void on_savedSensorRecordList_currentItemChanged(QListWidgetItem* current,
                                                    QListWidgetItem* previous);

private:

    void sensorReplayStart();
    void sensorReplayStop();
    float getPlaybackSpeed();
    void addSensorRecord(QString fullPath, QString fileName);
    QString getPreviewText(const std::vector<std::string> sensorList, const std::vector<int> carPropertyIdList);
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void showSensorRecordDetail(bool show);
    void createSensorRecordItem(QString itemName, QString itemPath);
    void deleteRecord(SensorReplayItem* item);
    QString toTimeLineSingle(SensorSessionPlayback::DurationNs time);
    QString toConnectTimeLine(SensorSessionPlayback::DurationNs timeline1, SensorSessionPlayback::DurationNs timeline2, const QString& connector);
    void addTimeLineTableRow(QTableWidget* table, const QString& timeline, const QString& eventCount, int tableRow);
    void updateTimeLine(SensorSessionPlayback::DurationNs timestamp);
    void updatePanel(SensorSessionPlayback* sensorSessionPlayback, QString fullPath);
    SensorReplayItem* getCurrentSensorRecordItem();


    static const QCarDataAgent* sCarDataAgent;
    static void sendCarEmulatorMessageLogged(const emulator::EmulatorMessage& msg,
                                      const std::string& log);
    static emulator::EmulatorMessage makePropMsg(int propId, emulator::SensorSession::SensorRecord::CarPropertyValue val);

    bool isPlaying;
    std::unique_ptr<Ui::SensorReplayPage> mUi;
    // std::unique_ptr<SensorSessionPlayback> mCurrentSensorSessionPlayback;
    QMessageBox* mSensorInfoDialog;
    SensorReplayItem* mSensorReplayItemSelected;
    SensorSessionPlayback::DurationNs totalTime;


};

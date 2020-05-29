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

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION

#include <qobjectdefs.h>  // for Q_OBJECT, slots

#include <QString>     // for QString
#include <QThread>     // for QThread
#include <QTimer>      // for QTimer
#include <QWidget>     // for QWidget
#include <functional>  // for function
#include <memory>      // for unique_ptr
#include <string>      // for string

#include "VehicleHalProto.pb.h"  // for EmulatorMessage
#include "ui_car-sensor-data.h"                          // for CarSensorData

class QObject;
class QWidget;

namespace emulator {

class TimedEmulatorMessages {
public:
    enum PlayStatus { STOP, START, PAUSE };

    std::vector<emulator::EmulatorMessage> getEvents(int64_t interval);
    void addEvents(int64_t timestamp, emulator::EmulatorMessage& msg);
    void clear();
    PlayStatus getStatus();
    void setStatus(PlayStatus status);
    int getCurrentIndex();

private:
    std::vector<emulator::EmulatorMessage> mEmulatorMessages;
    std::vector<int64_t> mTimestampes;
    int mCurrIndex = 0;
    int64_t mBaseTimeStamp = -1;
    PlayStatus mStatus;
};

class VhalEventLoaderThread : public QThread {
    Q_OBJECT
public:
    // Loads Vhal from a json file specified
    // by file_name into the EmulatorMessage array
    void loadVhalEventFromFile(const QString& file_name,
                               emulator::TimedEmulatorMessages* events);

    static VhalEventLoaderThread* newInstance();

signals:
    void loadingFinished(QString file_name, bool ok, QString error);

protected:
    // Reimplemented to load the file into the given fixes array.
    void run() override;

private:
    VhalEventLoaderThread() = default;
    QString mFileName;
    emulator::TimedEmulatorMessages* mTimedEmulatorMessages = nullptr;
    bool parseJsonFile(const char* filePath,
                       emulator::TimedEmulatorMessages* emulatorMessages);
    QString readJsonStringFromFile(const char* filePath);
    bool loadEmulatorEvents(const QJsonDocument& eventDoc,
                            emulator::TimedEmulatorMessages* emulatorMessages);
};

class EmulatorMessage;
}
class CarSensorData : public QWidget {
    Q_OBJECT
public:
    explicit CarSensorData(QWidget* parent = nullptr);
    using EmulatorMsgCallback =
            std::function<void(const emulator::EmulatorMessage& msg,
                               const std::string& log)>;
    void setSendEmulatorMsgCallback(EmulatorMsgCallback&&);
    void processMsg(emulator::EmulatorMessage emulatorMsg);

private slots:
    void on_car_speedSlider_valueChanged(int value);
    void on_comboBox_gear_currentIndexChanged(int index);
    void on_comboBox_ignition_currentIndexChanged(int index);
    void on_checkBox_night_toggled();
    void on_checkBox_park_toggled();
    void on_checkBox_fuel_low_toggled();
    void on_button_loadrecord_clicked();
    void on_button_playrecord_clicked();
    void on_button_voice_assistant_clicked();
    void vhalEventThreadStarted();
    void startupVhalEventThreadFinished(QString file_name,
                                        bool ok,
                                        QString error);

private:
    std::unique_ptr<Ui::CarSensorData> mUi;
    EmulatorMsgCallback mSendEmulatorMsg;
    const float MILES_PER_HOUR_TO_METERS_PER_SEC = 1.60934f*1000.0f/(60.0f*60.0f);
    const float KILOMETERS_PER_HOUR_TO_METERS_PER_SEC = 1000.0f/(60.0f*60.0f);
    enum SpeedUnitSelection {
        MILES_PER_HOUR = 0,
        KILOMETERS_PER_HOUR = 1
    };
    void sendGearChangeMsg(const int gear, const std::string& gearName);
    void sendIgnitionChangeMsg(const int ignition,
                               const std::string& ignitionName);
    void sendInputEvent(int32_t keyCode);
    float getSpeedMetersPerSecond(int speed, int unitIndex);
    int getIndexFromVehicleGear(int gear);
    void parseEventsFromJsonFile(QString jsonPath);

    // Vhal replay
    std::unique_ptr<emulator::VhalEventLoaderThread> mVhalEventLoader;
    emulator::TimedEmulatorMessages mTimedEmulatorMessages;
    bool mNowLoadingVhalEvent = false;
    QTimer mTimer;

    void VhalTimeout();
    void vhalEventThreadFinished(QString file_name,
                                 bool ok,
                                 QString error_message);
    void updateControlsAfterLoading();
    void finishVhalEventLoading(const QString& file_name,
                                bool ok,
                                const QString& error_message,
                                bool ignore_error);
    void prepareVhalLoader();
};

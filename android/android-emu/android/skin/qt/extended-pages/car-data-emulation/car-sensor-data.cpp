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
#include "android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h"

#include <stdint.h>  // for int32_t

#include <QByteArray>     // for QByteArray
#include <QCheckBox>      // for QCheckBox
#include <QComboBox>      // for QComboBox
#include <QFileDialog>    // for QFileDialog
#include <QJsonArray>     // for QJsonArray
#include <QJsonDocument>  // for QJsonDocument
#include <QJsonObject>    // for QJsonObject
#include <QJsonValue>     // for QJsonValue
#include <QLabel>         // for QLabel
#include <QSlider>        // for QSlider
#include <QTextStream>    // for QSlider
#include <utility>        // for move

#include "android/base/Log.h"
#include "android/featurecontrol/feature_control.h"
#include "android/hw-events.h"             // for EV_KEY, EV_SW
#include "android/skin/qt/error-dialog.h"  // for showErrorDialog
#include "android/utils/debug.h"           // for VERBOSE_PRINT
#include "ui_car-sensor-data.h"            // for CarSensorData
#include "vehicle_constants_generated.h"   // for VehicleIgnit...

class QWidget;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using std::string;

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::TimedEmulatorMessages;
using emulator::VehicleArea;
using emulator::VehicleGear;
using emulator::VehicleIgnitionState;
using emulator::VehicleProperty;
using emulator::VehiclePropertyGroup;
using emulator::VehiclePropertyType;
using emulator::VehicleDisplay;
using emulator::VehiclePropValue;
using emulator::VhalEventLoaderThread;

// See frameworks/base/core/java/android/view/KeyEvent.java
static constexpr int32_t KEYCODE_VOICE_ASSIST = 231;
// See android/hardware/interfaces/automotive/vehicle/2.0/default/impl/vhal_v2_0/DefaultConfig.h
static constexpr int32_t kGenerateFakeDataControllingProperty =
    0x0666 | (int)VehiclePropertyGroup::VENDOR | (int)VehicleArea::GLOBAL | (int)VehiclePropertyType::MIXED;
// See android/hardware/interfaces/automotive/vehicle/2.0/default/impl/vhal_v2_0/DefaultConfig.h
static constexpr int32_t kKeyPressCommand = 100;
static constexpr int64_t VHAL_REPLAY_INTERVAL = 1000;

CarSensorData::CarSensorData(QWidget* parent)
    : QWidget(parent), mUi(new Ui::CarSensorData) {
    mUi->setupUi(this);

    if (!feature_is_enabled(kFeature_CarVhalReplay)) {
        mUi->button_loadrecord->setVisible(false);
        mUi->button_playrecord->setVisible(false);
        QObject::connect(&mTimer, &QTimer::timeout, this,
                     &CarSensorData::VhalTimeout);
        prepareVhalLoader();
    }

    if (!feature_is_enabled(kFeature_CarAssistButton)) {
        mUi->button_voice_assistant->setVisible(false);
    }
}

static const enum VehicleGear sComboBoxGearValues[] = {
        VehicleGear::GEAR_NEUTRAL, VehicleGear::GEAR_REVERSE,
        VehicleGear::GEAR_PARK,    VehicleGear::GEAR_DRIVE};

static const enum VehicleIgnitionState sComboBoxIgnitionStates[] = {
        VehicleIgnitionState::UNDEFINED, VehicleIgnitionState::LOCK,
        VehicleIgnitionState::OFF,       VehicleIgnitionState::ACC,
        VehicleIgnitionState::ON,        VehicleIgnitionState::START};

static EmulatorMessage makeSetPropMsg() {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::SET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    return emulatorMsg;
}

void CarSensorData::VhalTimeout() {
    if (mTimedEmulatorMessages.getStatus() != TimedEmulatorMessages::START) {
        mTimer.stop();
        return;
    }
    std::vector<emulator::EmulatorMessage> events =
            mTimedEmulatorMessages.getEvents(VHAL_REPLAY_INTERVAL);
    int realIndex = mTimedEmulatorMessages.getCurrentIndex() - events.size();
    for (auto event : events) {
        string log = "Send event from json index: " + std::to_string(realIndex);
        realIndex++;
        mSendEmulatorMsg(event, log);
    }
}

void CarSensorData::sendGearChangeMsg(const int gear, const string& gearName) {
    // TODO: Grey out the buttons when callback is not set or vehicle hal is
    // not connected.
    if (mSendEmulatorMsg == nullptr) {
        return;
    }

    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::GEAR_SELECTION));
    value->add_int32_values(gear);
    string log = "Gear changed to " + gearName;
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::sendIgnitionChangeMsg(const int ignition,
                                          const string& ignitionName) {
    if (mSendEmulatorMsg == nullptr) {
        return;
    }

    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::IGNITION_STATE));
    value->add_int32_values(ignition);
    string log = "Ignition state: " + ignitionName;
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::on_car_speedSlider_valueChanged(int speed) {
    mUi->car_speedLabel->setText(QString::number(speed));
    float speedMetersPerSecond = (float)speed * 
        ((mUi->comboBox_speedUnit->currentIndex() == MILES_PER_HOUR) 
            ? MILES_PER_HOUR_TO_METERS_PER_SEC
            : KILOMETERS_PER_HOUR_TO_METERS_PER_SEC);
    
    if (mSendEmulatorMsg != nullptr) {
        EmulatorMessage emulatorMsg = makeSetPropMsg();
        VehiclePropValue* value = emulatorMsg.add_value();
        value->set_prop(
                static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED));
        value->add_float_values(speedMetersPerSecond);
        string log = "Speed changed to " + std::to_string(speedMetersPerSecond);
        mSendEmulatorMsg(emulatorMsg, log);
    }
}

void CarSensorData::setSendEmulatorMsgCallback(EmulatorMsgCallback&& func) {
    mSendEmulatorMsg = std::move(func);
}

void CarSensorData::on_checkBox_night_toggled() {
    bool night = mUi->checkBox_night->isChecked();
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::NIGHT_MODE));
    value->add_int32_values(night ? 1 : 0);
    string log = "Night mode: " + std::to_string(night);
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::on_checkBox_park_toggled() {
    bool parkBrakeOn = mUi->checkBox_park->isChecked();
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::PARKING_BRAKE_ON));
    value->add_int32_values(parkBrakeOn ? 1 : 0);
    string log = "Park brake: " + std::to_string(parkBrakeOn);
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::on_checkBox_fuel_low_toggled() {
    bool fuelLow = mUi->checkBox_fuel_low->isChecked();
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(static_cast<int32_t>(VehicleProperty::FUEL_LEVEL_LOW));
    value->add_int32_values(fuelLow ? 1 : 0);
    string log = "Fuel low: " + std::to_string(fuelLow);
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::on_comboBox_ignition_currentIndexChanged(int index) {
    sendIgnitionChangeMsg(static_cast<int32_t>(sComboBoxIgnitionStates[index]),
                          mUi->comboBox_ignition->currentText().toStdString());
}

void CarSensorData::on_comboBox_gear_currentIndexChanged(int index) {
    sendGearChangeMsg(static_cast<int32_t>(sComboBoxGearValues[index]),
                      mUi->comboBox_gear->currentText().toStdString());
}

void CarSensorData::on_button_loadrecord_clicked() {
    prepareVhalLoader();

    QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Json File"), ".", tr("Json files (*.json)"));

    if (fileName.isNull())
        return;
    parseEventsFromJsonFile(fileName);
}

void CarSensorData::prepareVhalLoader() {
    mVhalEventLoader.reset(VhalEventLoaderThread::newInstance());

    connect(mVhalEventLoader.get(), &VhalEventLoaderThread::started, this,
            &CarSensorData::vhalEventThreadStarted);

    connect(mVhalEventLoader.get(), &VhalEventLoaderThread::loadingFinished,
            this, &CarSensorData::startupVhalEventThreadFinished);

    // Make sure new_instance gets cleaned up after the thread exits.
    connect(mVhalEventLoader.get(), &VhalEventLoaderThread::finished,
            mVhalEventLoader.get(), &QObject::deleteLater);
}

void CarSensorData::on_button_playrecord_clicked() {
    mTimedEmulatorMessages.setStatus(TimedEmulatorMessages::START);
    mTimer.start(VHAL_REPLAY_INTERVAL);
}

void CarSensorData::on_button_voice_assistant_clicked() {
    sendInputEvent(KEYCODE_VOICE_ASSIST);
}

void CarSensorData::sendInputEvent(int32_t keyCode) {
    EmulatorMessage emulatorMsg = makeSetPropMsg();
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop(kGenerateFakeDataControllingProperty);
    value->add_int32_values(kKeyPressCommand);
    value->add_int32_values(static_cast<int32_t>(VehicleProperty::HW_KEY_INPUT));
    value->add_int32_values(keyCode);
    value->add_int32_values(static_cast<int32_t>(VehicleDisplay::MAIN));
    string log = "Key event: " + std::to_string(keyCode);
    mSendEmulatorMsg(emulatorMsg, log);
}

void CarSensorData::parseEventsFromJsonFile(QString jsonPath) {
    mVhalEventLoader->loadVhalEventFromFile(jsonPath, &mTimedEmulatorMessages);
}

void CarSensorData::processMsg(emulator::EmulatorMessage emulatorMsg) {
    if (emulatorMsg.prop_size() == 0 && emulatorMsg.value_size() == 0) {
        return;
    }
    for (int valIndex = 0; valIndex < emulatorMsg.value_size(); valIndex++) {
        VehiclePropValue val = emulatorMsg.value(valIndex);

        switch (val.prop()) {
            case static_cast<int32_t>(VehicleProperty::GEAR_SELECTION): {
                int gear_vhal = getIndexFromVehicleGear(val.int32_values(0));
                int gear_state = mUi->comboBox_gear->currentIndex();
                if (gear_state != gear_vhal) {
                    mUi->comboBox_gear->setCurrentIndex(gear_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::FUEL_LEVEL_LOW): {
                bool fuelLow_vhal = val.int32_values(0) == 1;
                bool fuelLow = mUi->checkBox_fuel_low->isChecked();
                if (fuelLow != fuelLow_vhal) {
                    mUi->checkBox_fuel_low->setChecked(fuelLow_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::PARKING_BRAKE_ON): {
                bool parkingBreak_vhal = val.int32_values(0) == 1;
                bool parkingBreak = mUi->checkBox_park->isChecked();
                if (parkingBreak != parkingBreak_vhal) {
                    mUi->checkBox_park->setChecked(parkingBreak_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::NIGHT_MODE): {
                bool night_vhal = val.int32_values(0) == 1;
                bool night = mUi->checkBox_night->isChecked();
                if (night != night_vhal) {
                    mUi->checkBox_night->setChecked(night_vhal);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::PERF_VEHICLE_SPEED): {
                float speedMeterPerSecond = val.float_values(0);
                int speed = static_cast<int32_t>(
                        speedMeterPerSecond /
                        ((mUi->comboBox_speedUnit->currentIndex() ==
                          MILES_PER_HOUR)
                                 ? MILES_PER_HOUR_TO_METERS_PER_SEC
                                 : KILOMETERS_PER_HOUR_TO_METERS_PER_SEC));
                if (speed != mUi->car_speedSlider->value()) {
                    mUi->car_speedSlider->setValue(speed);
                }
                break;
            }
            case static_cast<int32_t>(VehicleProperty::IGNITION_STATE): {
                int ignition_state_vhal = val.int32_values(0);
                int ignition_state = mUi->comboBox_ignition->currentIndex();
                if (ignition_state != ignition_state_vhal) {
                    mUi->comboBox_ignition->setCurrentIndex(
                            ignition_state_vhal);
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}

int CarSensorData::getIndexFromVehicleGear(int gear) {
    int len = sizeof(sComboBoxGearValues) / sizeof(sComboBoxGearValues[0]);
    for (int index = 0; index < len; index++) {
        if (static_cast<int>(sComboBoxGearValues[index]) == gear) {
            return index;
        }
    }
    return len - 1;
}

void CarSensorData::vhalEventThreadStarted() {
    // Prevent the user from initiating a load json while another load is
    // already in progress
    mUi->button_loadrecord->setEnabled(false);
    mUi->button_playrecord->setEnabled(false);

    mNowLoadingVhalEvent = true;
}

void CarSensorData::startupVhalEventThreadFinished(QString file_name,
                                                   bool ok,
                                                   QString error_message) {
    // on startup, we silently ignore the previously remebered event data file
    // being missing or malformed.
    finishVhalEventLoading(file_name, ok, error_message, true);
}

void CarSensorData::vhalEventThreadFinished(QString file_name,
                                            bool ok,
                                            QString error_message) {
    // on startup, we silently ignore the previously remebered event data file
    // being missing or malformed.
    finishVhalEventLoading(file_name, ok, error_message, false);
}

void CarSensorData::finishVhalEventLoading(const QString& file_name,
                                           bool ok,
                                           const QString& error_message,
                                           bool ignore_error) {
    mVhalEventLoader.reset();
    updateControlsAfterLoading();

    if (!ok) {
        if (!ignore_error) {
            showErrorDialog(error_message, tr("Vhal EVENT Parser"));
        }
        return;
    }
}

void CarSensorData::updateControlsAfterLoading() {
    mUi->button_loadrecord->setEnabled(true);
    mUi->button_playrecord->setEnabled(true);

    mNowLoadingVhalEvent = false;
}

void VhalEventLoaderThread::loadVhalEventFromFile(
        const QString& file_name,
        TimedEmulatorMessages* events) {
    mFileName = file_name;
    mTimedEmulatorMessages = events;
    start();
}

void VhalEventLoaderThread::run() {
    if (mFileName.isEmpty() || mTimedEmulatorMessages == nullptr) {
        emit(loadingFinished(mFileName, false, tr("No file to load")));
        return;
    }
    bool ok = false;
    std::string err_str;

    QFileInfo file_info(mFileName);
    mTimedEmulatorMessages->clear();
    auto suffix = file_info.suffix().toLower();
    if (suffix == "json") {
        ok = parseJsonFile(mFileName.toStdString().c_str(),
                           mTimedEmulatorMessages);
    } else {
        err_str = tr("Unknown file type").toStdString();
    }

    auto err_qstring = QString::fromStdString(err_str);
    emit(loadingFinished(mFileName, ok, err_qstring));
}

bool VhalEventLoaderThread::parseJsonFile(
        const char* filePath,
        TimedEmulatorMessages* timedEmulatorMessages) {
    QString jsonString;
    if (filePath) {
        jsonString = readJsonStringFromFile(filePath);
    }

    QJsonDocument eventDoc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (eventDoc.isNull()) {
        return false;
    } else {
        return loadEmulatorEvents(eventDoc, timedEmulatorMessages);
    }
    return false;
}

bool VhalEventLoaderThread::loadEmulatorEvents(
        const QJsonDocument& eventDoc,
        TimedEmulatorMessages* timedEmulatorMessages) {
    if (eventDoc.isNull()) {
        return false;
    }
    QJsonObject eventsJson = eventDoc.object();
    QJsonArray eventArray = eventsJson.value("events").toArray();

    for (int eventIdx = 0; eventIdx < eventArray.size(); eventIdx++) {
        QJsonObject eventObject = eventArray.at(eventIdx).toObject();
        QJsonDocument Doc(eventObject);
        QByteArray ba = Doc.toJson();
        QJsonObject sensorrecord =
                eventObject.value("sensor_records").toObject();
        QJsonObject carPropertyValues =
                sensorrecord.value("car_property_values").toObject();
        int propID = carPropertyValues.value("key").toInt();

        EmulatorMessage emulatorMsg = makeSetPropMsg();
        VehiclePropValue* value = emulatorMsg.add_value();
        value->set_prop(static_cast<int32_t>(propID));

        int prop = carPropertyValues.value("key").toInt();
        int areaId = carPropertyValues.value("value")
                             .toObject()
                             .value("area_id")
                             .toInt();
        QJsonObject propertyValue = carPropertyValues.value("value").toObject();
        int type = 0;
        if (propertyValue.contains("int32_values")) {
            value->add_int32_values(
                    propertyValue.value("int32_values").toInt());
        } else if (propertyValue.contains("float_values")) {
            value->add_float_values(
                    (float)propertyValue.value("float_values").toDouble());
        }

        char* error = nullptr;
        timedEmulatorMessages->addEvents(
                strtol(sensorrecord.value("timestamp_ns")
                               .toString()
                               .toStdString()
                               .c_str(),
                       &error, 0),
                emulatorMsg);
    }

    return true;
}

QString VhalEventLoaderThread::readJsonStringFromFile(const char* filePath) {
    QString fullContents;
    if (filePath) {
        QFile jsonFile(filePath);
        if (jsonFile.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream jsonStream(&jsonFile);
            fullContents = jsonStream.readAll();
            jsonFile.close();
        }
    }
    return fullContents;
}

VhalEventLoaderThread* VhalEventLoaderThread::newInstance() {
    VhalEventLoaderThread* new_instance = new VhalEventLoaderThread();
    return new_instance;
}

std::vector<emulator::EmulatorMessage> TimedEmulatorMessages::getEvents(
        int64_t interval) {
    std::vector<emulator::EmulatorMessage> res;

    if (mTimestampes.size() != mEmulatorMessages.size()) {
        clear();
        return res;
    }
    if (mCurrIndex == mEmulatorMessages.size()) {
        mCurrIndex = 0;
        mBaseTimeStamp = -1;
        mStatus = STOP;
        return res;
    }

    int64_t nextTimeStamp = mBaseTimeStamp + (int64_t)interval;

    for (; mCurrIndex < mEmulatorMessages.size(); mCurrIndex++) {
        int64_t timestamp = mTimestampes.at(mCurrIndex);
        if (timestamp >= mBaseTimeStamp && timestamp < nextTimeStamp) {
            res.push_back(mEmulatorMessages.at(mCurrIndex));
        } else {
            break;
        }
    }
    mBaseTimeStamp = nextTimeStamp;
    return res;
}

void TimedEmulatorMessages::addEvents(int64_t timestamp,
                                      emulator::EmulatorMessage& msg) {
    if (mBaseTimeStamp == -1) {
        mBaseTimeStamp = timestamp;
    }
    if (mTimestampes.size() != mEmulatorMessages.size()) {
        return;
    }
    mEmulatorMessages.push_back(msg);
    mTimestampes.push_back(timestamp);
}

void TimedEmulatorMessages::clear() {
    mEmulatorMessages.clear();
    mTimestampes.clear();
    mCurrIndex = 0;
    mBaseTimeStamp = -1;
}

TimedEmulatorMessages::PlayStatus TimedEmulatorMessages::getStatus() {
    return mStatus;
}

void TimedEmulatorMessages::setStatus(PlayStatus status) {
    mStatus = status;
}

int TimedEmulatorMessages::getCurrentIndex() {
    return mCurrIndex;
}

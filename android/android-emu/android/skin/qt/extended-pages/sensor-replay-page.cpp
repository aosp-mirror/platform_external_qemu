// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h"
#include "android/skin/qt/extended-pages/sensor-replay-page.h"

#include <QFileDialog>    // for QFileDialog

#include "android/emulation/control/car_data_agent.h"
#include "android/utils/debug.h"
#include "ui_sensor-replay-page.h"  // for CarSensorReplayPage


class QWidget;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::SensorSession;
using emulator::Status;
using emulator::VehiclePropValue;
using std::string;

static constexpr float PLAYBACK_SPEED[5]  = {1, 2, 5, 10, 100};

const QCarDataAgent* SensorReplayPage::sCarDataAgent = nullptr;


SensorReplayPage::SensorReplayPage(QWidget* parent)
    :QWidget(parent), mUi(new Ui::SensorReplayPage) {
    mUi->setupUi(this);
    mUi->noFileMask->setVisible(false);

    isPlaying = true;
}

void SensorReplayPage::on_sensor_LoadSensorButton_clicked() {
    D("import button clicked");

    QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Sensor Record File"), ".");

    if (fileName.isNull())
        return;

    ssp.reset(new SensorSessionPlayback());
    // parseEventsFromJsonFile(fileName);

    //sensor replay v2
    SensorSessionPlayback::SensorSessionStatus status = ssp->LoadFrom(fileName.toStdString());

    if (status == SensorSessionPlayback::OK) {
        D("filename parse OK for sensorsessionplayback");
    } else if (status == SensorSessionPlayback::PROTO_PARSE_FAIL) {
        D("proto parse fail");
    }

}

void SensorReplayPage::on_sensor_playStopButton_clicked() {
    D("play button clicked");

    if(isPlaying) {
        sensorReplayStart();
    } else {
        sensorReplayStop();
    }
}

void SensorReplayPage::sensorReplayStart() {
    D ("start play button clicked in car data");
    ssp->RegisterCallback([this](emulator::SensorSession::SensorRecord record) {

        if (record.car_property_values_size() > 0) {
            std::map<int32_t, emulator::SensorSession::SensorRecord::CarPropertyValue> vhal_property_map(record.car_property_values().begin(),
                                        record.car_property_values().end());
            std::map<int32_t, emulator::SensorSession::SensorRecord::CarPropertyValue>::iterator it;

            for ( it = vhal_property_map.begin(); it != vhal_property_map.end(); it++ )
            {
                EmulatorMessage emulatorMsg = carpropertyutils::makePropMsg((int) it->first, it->second);
                sendCarEmulatorMessageLogged(emulatorMsg, "Replay record is sent");

            }
        }
    });

    ssp->StartReplay(getPlaybackSpeed());
    isPlaying = true;
    // Change the icon on the play/stop button.
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("pause"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "pause");
}

float SensorReplayPage::getPlaybackSpeed() {
    return PLAYBACK_SPEED[mUi->playbackSpeed->currentIndex()];
}

void SensorReplayPage::sensorReplayStop() {
    ssp->StopReplay();

    isPlaying = false;
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "play_arrow");
}

//static
void SensorReplayPage::setCarDataAgent(const QCarDataAgent* agent) {
    if (agent == nullptr) D("data agent null");
    sCarDataAgent = agent;
}

void SensorReplayPage::sendCarEmulatorMessageLogged(const EmulatorMessage& msg,
                                               const string& log) {
    if (sCarDataAgent == nullptr) {
        return;
    }
    D("Car sensor replay %s", log.c_str());
    string msgString;
    if (msg.SerializeToString(&msgString)) {
        sCarDataAgent->sendCarData(msgString.c_str(), msgString.length());
    } else {
        D("Failed to send emulator message.");
    }
}

EmulatorMessage SensorReplayPage::makePropMsg(int propId, SensorSession::SensorRecord::CarPropertyValue val) {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::SET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop((int) propId);

    // If area id is empty, return empty EmulatorMessage
    if (val.has_area_id()) {
        value->set_area_id((int) val.area_id());
    }


    // Unused field in emulatorMessage optional int32 status = 2;
    // optional bool bool_value = 3;
    if (val.has_bool_value()) {
        value->add_int32_values(val.bool_value()? 1 : 0);
    }

    // repeated sint32 int32_values = 4;
    for (auto int32_value : val.int32_values()) {
        value->add_int32_values(int32_value);
    }

    // repeated sint64 int64_values = 5;
    for (auto int64_value : val.int64_values()) {
        value->add_int64_values(int64_value);
    }
    // repeated float float_values = 6;
    for (auto float_value : val.float_values()) {
        value->add_float_values(float_value);
    }
    // optional string string_value = 7;
    if (val.has_string_value()) {
        value->set_string_value(val.string_value());
    }
    // optional bytes bytes_value = 8;
    if (val.has_bytes_value()) {
        value->set_bytes_value(val.bytes_value());
    }

    return emulatorMsg;

}

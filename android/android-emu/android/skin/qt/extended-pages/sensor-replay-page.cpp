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

#include "android/skin/qt/extended-pages/sensor-replay-page.h"

#include <qmessagebox.h>  // for QMessageBox::C...f
#include <QFileDialog>    // for QFileDialog
#include <QMenu>
#include <ctime>

// TODO: (b/120444474) rename ERROR_INVALID_OPERATION & remove this undef
#undef ERROR_INVALID_OPERATION
#include "VehicleHalProto.pb.h"
#include "android/base/Log.h"  // for DCHECK
#include "android/base/StringFormat.h"  // for StringFormat
#include "android/emulation/control/car_data_agent.h"
#include "android/emulation/control/location_agent.h"
#include "android/emulation/control/sensors_agent.h"
#include "android/hw-sensors.h"  // for ANDROID_SENSOR_ACCEL...
#include "android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h"
#include "android/skin/qt/extended-pages/common.h"   // for setButtonEnabled
#include "android/skin/qt/extended-pages/location-page.h"
#include "android/skin/qt/extended-pages/sensor-replay-item.h"
#include "android/utils/debug.h"
#include "sensor_session.pb.h"
#include "ui_sensor-replay-page.h"  // for CarSensorReplayPage

class QWidget;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using android::base::StringFormat;
using android::sensorsessionplayback::SensorSessionPlayback;
using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::SensorSession;
using emulator::Status;
using emulator::VehiclePropValue;

static constexpr float PLAYBACK_SPEED[8] = {1, 2, 5, 10, 100, 0.5, 0.25, 0.1};
static constexpr double E7 = 1e7;
static constexpr double MPS_TO_KNOTS = 1.94384;
static constexpr SensorReplayPage::DurationNs TIMELINE_INTERVAL =
        1000 * 1000 * 1000;

// https://developer.android.com/reference/android/hardware/Sensor#TYPE_GYROSCOPE
static constexpr int ANDROID_SENSOR_TYPE_GYROSCOPE = 4;
// https://developer.android.com/reference/android/hardware/Sensor#TYPE_GYROSCOPE_UNCALIBRATED
static constexpr int ANDROID_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED = 16;
// https://developer.android.com/reference/android/hardware/Sensor#TYPE_ACCELEROMETER
static constexpr int ANDROID_SENSOR_TYPE_ACCELEROMETER = 1;
// https://developer.android.com/reference/android/hardware/Sensor#TYPE_MAGNETIC_FIELD
static constexpr int ANDROID_SENSOR_TYPE_MAGNETIC_FIELD = 2;
// https://developer.android.com/reference/android/hardware/Sensor#TYPE_PRESSURE
static constexpr int ANDROID_SENSOR_TYPE_PRESSURE = 6;

const QCarDataAgent* SensorReplayPage::sCarDataAgent = nullptr;
const QAndroidLocationAgent* SensorReplayPage::sLocationAgent = nullptr;
const QAndroidSensorsAgent* SensorReplayPage::sSensorAgent = nullptr;

SensorReplayPage::SensorReplayPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::SensorReplayPage) {
    mUi->setupUi(this);

    showSensorRecordDetail(false);

    isPlaying = false;

    mSensorInfoDialog = new QMessageBox(
            QMessageBox::NoIcon,
            QString::fromStdString(StringFormat("Sensors Detail")), "",
            QMessageBox::Close, this);
    mSensorInfoDialog->setModal(false);
    mUi->sensor_previewValue_expand->installEventFilter(this);

    connect(mUi->sensor_playbackSlider, SIGNAL(sliderReleased()), this,
            SLOT(adjustPlaybackSlider()));
}

void SensorReplayPage::showSensorRecordDetail(bool show) {
    SettingsTheme theme = getSelectedTheme();
    setButtonEnabled(mUi->sensor_playStopButton, theme, show);

    mUi->sensor_noFileMask->setVisible(!show);

    mUi->sensor_fileNameTitle->setVisible(show);
    mUi->sensor_filenameValue->setVisible(show);
    mUi->sensor_appVersionTitle->setVisible(show);
    mUi->sensor_appVersionTitleValue->setVisible(show);
    mUi->sensor_totalSensorEventsCountTitle->setVisible(show);
    mUi->sensor_totalSensorEventsCountValue->setVisible(show);
    mUi->sensor_previewTitle->setVisible(show);
    mUi->sensor_previewValue->setVisible(show);
    mUi->sensor_previewValue_expand->setVisible(show);
    mUi->sensor_eventTimelineTitle->setVisible(show);
    mUi->sensor_eventTimelineTable->setVisible(show);
    if (!show) {
        mUi->sensor_playbackTime->setText(formatTimeProgress(0, 0));
    }
}

bool SensorReplayPage::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mSensorInfoDialog->show();
    return true;
}

// This functions is triggerred when user manually adjust the playback slider
// When there is a sensor replay item available, use seekToTime function to
// adjust the start point of playback session. When there is no sensor replay
// item, pop up an warning
void SensorReplayPage::adjustPlaybackSlider() {
    if (mUi->sensor_savedSensorRecordList->count() == 0) {
        QMessageBox::warning(this, tr("Warning"),
                            tr("Please import a sensor file first"),
                             QMessageBox::Cancel);
        mUi->sensor_playbackSlider->setValue(0);
        return;
    } else {
        if (isPlaying) {
            QMessageBox::warning(
                    this, tr("Warning"),
                    tr("Please don't move the slider while playing"),
                    QMessageBox::Cancel);
            return;
        }

        DurationNs time =
                totalTime * (mUi->sensor_playbackSlider->value()) / 100;

        getCurrentSensorRecordItem()->getSensorSessionPlayback()->seekToTime(
                time);
        updateTimeLine(time);
    }
}

void SensorReplayPage::on_sensor_loadSensorButton_clicked() {
    QString filePath = QFileDialog::getOpenFileName(
            this, tr("Open Sensor Record File"), ".");

    if (filePath.isNull())
        return;

    // Check if this record already exists in the list, if yes show an error
    // then return
    if (findSensorRecord(filePath)) {
        QMessageBox::warning(this, tr("Warning"),
                             filePath + tr(" already exists in the list"),
                             QMessageBox::Cancel);
        return;
    }

    // Load the sensor record file to SensorSessionPlayback
    SensorSessionPlayback* currentSensorSessionPlayback;
    currentSensorSessionPlayback = new SensorSessionPlayback();

    if (currentSensorSessionPlayback->loadFrom(filePath.toStdString())) {
        auto parts = filePath.split(QLatin1Char('/'));

        // Add a sensor record to the history list
        addSensorRecord(parts.last(), filePath, currentSensorSessionPlayback);

        updatePanel(currentSensorSessionPlayback, filePath);

        registerSensorSessionPlaybackCallback(currentSensorSessionPlayback);

    } else {
        D("proto parseing failed");
    }
}

void SensorReplayPage::registerSensorSessionPlaybackCallback(
        SensorSessionPlayback* sensorSessionPlayback) {
    google::protobuf::Map<std::string, emulator::SensorSession::SessionMetadata::Sensor>
        subscribedSensors = sensorSessionPlayback->metadata().subscribed_sensors();

    sensorSessionPlayback->registerCallback(
            [this, subscribedSensors](emulator::SensorSession::SensorRecord record) {

                if (sCarDataAgent != nullptr && record.car_property_values_size() > 0) {
                    std::map<int32_t, emulator::SensorSession::SensorRecord::
                                              CarPropertyValue>
                            vhal_property_map(
                                    record.car_property_values().begin(),
                                    record.car_property_values().end());
                    std::map<int32_t, emulator::SensorSession::SensorRecord::
                                              CarPropertyValue>::iterator it;

                    for (it = vhal_property_map.begin();
                         it != vhal_property_map.end(); it++) {
                        EmulatorMessage emulatorMsg =
                                makePropMsg((int)it->first, it->second);
                        sendCarEmulatorMessageLogged(emulatorMsg,
                                                     "Replay record is sent");
                    }
                }

                if (sLocationAgent != nullptr && record.locations_size() > 0) {
                    const auto& locations = record.locations();
                    auto locationIterator =
                        std::find_if(locations.begin(), locations.end(),
                                     [](const SensorSession::SensorRecord::PositionProto& location)
                                     {return location.has_provider() && (location.provider() ==
                                       SensorSession::SensorRecord::PositionProto::GPS);});

                    if (locationIterator != locations.end()) {
                        double latitude =
                                ((int)locationIterator->point().lat_e7()) / E7;
                        double longitude =
                                ((int)locationIterator->point().lng_e7()) / E7;
                        double altitude = locationIterator->altitude_m();
                        double velocity = (double)locationIterator->speed_mps() *
                                          MPS_TO_KNOTS;
                        double heading = (double)locationIterator->bearing_deg_full_accuracy() -
                                         180.0;

                        timeval timeVal = {};
                        gettimeofday(&timeVal, nullptr);
                        sLocationAgent->gpsSendLoc(latitude, longitude, altitude,
                                                   velocity, heading, 4, &timeVal);

                        // To sync with LocationPage, update location to settings
                        LocationPage::writeDeviceLocationToSettings(
                                latitude, longitude, altitude, velocity, heading);
                    }
                }

                if (sSensorAgent != nullptr && record.sensor_events_size() > 0) {
                    std::map<std::string, emulator::SensorSession::SensorRecord::
                                              SensorEvent>
                            sensor_map(
                                    record.sensor_events().begin(),
                                    record.sensor_events().end());
                    std::map<std::string, emulator::SensorSession::SensorRecord::
                                              SensorEvent>::iterator it;

                    for (it = sensor_map.begin();
                         it != sensor_map.end(); it++) {
                        std::string sensorKey = it->first;
                        emulator::SensorSession::SessionMetadata::Sensor subscribedSensor =
                            subscribedSensors.at(it->first);
                        int sensorId = convertSensorTypeToSensorId(subscribedSensor.type());
                        if(sensorId >= 0 && !subscribedSensor.wake_up_sensor()) {
                            size_t sensorVectorSize;
                            sSensorAgent->getSensorSize(sensorId, &sensorVectorSize);
                            DCHECK(sensorVectorSize == it->second.values_size());
                            sSensorAgent->setSensorOverride(sensorId, it->second.values().data(),
                                                            it->second.values_size());
                        }
                    }
                }
                // update slider and time ticker,
                // reset playbacksession if it's the end
                this->updateTimeLine(record.timestamp_ns());
            });
}

void SensorReplayPage::updatePanel(SensorSessionPlayback* sensorSessionPlayback,
                                   QString filePath) {
    if (sensorSessionPlayback == nullptr) {
        return;
    }

    // Update file name
    auto parts = filePath.split(QLatin1Char('/'));
    mUi->sensor_filenameValue->setText(parts.last());

    // Update app version
    mUi->sensor_appVersionTitleValue->setText(
            getAppVersion(sensorSessionPlayback->metadata()));

    // Update Sensor Events count
    mUi->sensor_totalSensorEventsCountValue->setText(
            QString::number(sensorSessionPlayback->eventCount()));

    // Set sensor preview

    QString PreviewText =
            getPreviewText(sensorSessionPlayback->metadata(), ";\n");
    QFontMetrics metrics(mUi->sensor_previewValue_expand->font());
    QString elidedText = metrics.elidedText(
            getPreviewText(sensorSessionPlayback->metadata(), "; "),
            Qt::ElideRight, mUi->sensor_previewValue->width());
    mUi->sensor_previewValue->setText(elidedText);

    // Set sensor preview dialog
    mSensorInfoDialog->setInformativeText(PreviewText);

    mUi->sensor_previewValue_expand->setText("more");

    // Set playback time label
    std::vector<int> recordCounts =
            sensorSessionPlayback->getSensorRecordTimeLine(TIMELINE_INTERVAL);

    totalTime = sensorSessionPlayback->sessionDuration();
    mUi->sensor_playbackTime->setText(formatTimeProgress(0, totalTime));

    // Update the time line table
    mUi->sensor_eventTimelineTable->clear();
    mUi->sensor_eventTimelineTable->setRowCount(0);
    mUi->sensor_eventTimelineTable->setHorizontalHeaderItem(
            0, new QTableWidgetItem(tr("Time Range")));
    mUi->sensor_eventTimelineTable->setHorizontalHeaderItem(
            1, new QTableWidgetItem(tr("Event Count")));

    DurationNs time = 0;
    for (int row = 0; row < recordCounts.size(); row++) {
        addTimeLineTableRow(mUi->sensor_eventTimelineTable,
                            formatTimeRange(time, time + TIMELINE_INTERVAL),
                            QString::number(recordCounts[row]), row);
        time += TIMELINE_INTERVAL;
    }

    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "play_arrow");

    updateTimeLine(0);

    showSensorRecordDetail(true);
}

QString SensorReplayPage::getAppVersion(
        emulator::SensorSession::SessionMetadata metadata) {
    if (metadata.has_version()) {
        emulator::SensorSession::SessionMetadata::Version version =
                metadata.version();
        std::string version_string =
                version.has_major() ? std::to_string(version.major()) : "0";
        version_string.append(".").append(
                version.has_minor() ? std::to_string(version.minor()) : "0");
        version_string.append(".").append(
                version.has_patch() ? std::to_string(version.patch()) : "0");
        return QString::fromStdString(version_string);
    }

    return QString::fromStdString("N/A");
}

QString SensorReplayPage::getPreviewText(
        emulator::SensorSession::SessionMetadata metadata,
        std::string connector) {
    QString result;
    auto car_property_configs = metadata.car_property_configs();
    for (auto it = car_property_configs.begin();
         it != car_property_configs.end(); it++) {
        int carPropertyId = it->first;
        QString property =
                carpropertyutils::propMap.count(carPropertyId)
                        ? carpropertyutils::propMap[carPropertyId].label
                        : QObject::tr(std::to_string(carPropertyId).c_str());
        result.append(property);
        result.append(connector.c_str());
    }

    auto subscribed_sensors = metadata.subscribed_sensors();
    for (auto it = subscribed_sensors.begin(); it != subscribed_sensors.end();
         it++) {
        result.append(it->first.c_str());
        result.append(connector.c_str());
    }

    return result;
}

void SensorReplayPage::addTimeLineTableRow(QTableWidget* table,
                                           const QString& timeline,
                                           const QString& eventCount,
                                           int tableRow) {
    table->insertRow(tableRow);
    QTableWidgetItem* timeLineItem = new QTableWidgetItem(timeline);
    QTableWidgetItem* eventCountItem = new QTableWidgetItem(eventCount);

    timeLineItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    eventCountItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    table->setItem(tableRow, 0, timeLineItem);
    table->setItem(tableRow, 1, eventCountItem);
}

SensorReplayItem* SensorReplayPage::getCurrentSensorRecordItem() {
    QListWidgetItem* currentListWidgetItem =
            mUi->sensor_savedSensorRecordList->currentItem();
    SensorReplayItem* currentSensorReplayItem = qobject_cast<SensorReplayItem*>(
            mUi->sensor_savedSensorRecordList->itemWidget(
                    currentListWidgetItem));
    return currentSensorReplayItem;
}

// Let's assume that the total duration of the sensor record is less than 1 day
QString SensorReplayPage::toTimeString(DurationNs time) {
    time_t secs = (int)(time / 1000000000);

    tm* gtm = gmtime(&secs);
    char buffer[20];
    if (gtm->tm_hour > 0) {
        strftime(buffer, 20, "%H:%M:%S", gtm);
    } else {
        strftime(buffer, 20, "%M:%S", gtm);
    }
    return QString(buffer);
}

QString SensorReplayPage::getConnectedTimeString(DurationNs current,
                                                 DurationNs total,
                                                 const QString& connector) {
    QString result =
            toTimeString(current).append(connector).append(toTimeString(total));
    return result;
}

QString SensorReplayPage::formatTimeRange(DurationNs from, DurationNs to) {
    return getConnectedTimeString(from, to, tr(" - "));
}

QString SensorReplayPage::formatTimeProgress(DurationNs position,
                                             DurationNs total) {
    return getConnectedTimeString(position, total, tr(" / "));
}

// Playback control start
void SensorReplayPage::on_sensor_playStopButton_clicked() {
    if (!isPlaying) {
        sensorReplayStart();
    } else {
        sensorReplayStop();
    }
}

void SensorReplayPage::sensorReplayStart() {
    getCurrentSensorRecordItem()->getSensorSessionPlayback()->startReplay(
            getPlaybackSpeed());

    // Adjust UI and flags
    isPlaying = true;
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("stop"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "stop");
    // Disable import button and history list until finish current playback
    // session
    mUi->sensor_loadSensorButton->setEnabled(!isPlaying);
    mUi->sensor_savedSensorRecordList->setEnabled(false);
}

void SensorReplayPage::sensorReplayStop() {
    getCurrentSensorRecordItem()->getSensorSessionPlayback()->stopReplay();
    getCurrentSensorRecordItem()->getSensorSessionPlayback()->seekToTime(0);
    updateTimeLine(0);
    // Adjust UI and flags
    isPlaying = false;
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "play_arrow");
    mUi->sensor_loadSensorButton->setEnabled(!isPlaying);
    mUi->sensor_savedSensorRecordList->setEnabled(true);
}

float SensorReplayPage::getPlaybackSpeed() {
    return PLAYBACK_SPEED[mUi->sensor_playbackSpeed->currentIndex()];
}

void SensorReplayPage::updateTimeLine(DurationNs timestamp) {
    if (timestamp < 0) {
        timestamp = 0;
    }

    mUi->sensor_playbackSlider->setValue(timestamp * 100 / totalTime);
    mUi->sensor_playbackTime->setText(formatTimeProgress(timestamp, totalTime));

    if (timestamp == totalTime) {
        sensorReplayStop();
    }
}

void SensorReplayPage::sendCarEmulatorMessageLogged(const EmulatorMessage& msg,
                                                    const std::string& log) {
    if (sCarDataAgent == nullptr) {
        return;
    }
    std::string msgString;
    if (msg.SerializeToString(&msgString)) {
        sCarDataAgent->sendCarData(msgString.c_str(), msgString.length());
    } else {
        D("Failed to send emulator message.");
    }
}

// Convert SensorRecord to car EmulatorMessage
EmulatorMessage SensorReplayPage::makePropMsg(
        int propId,
        SensorSession::SensorRecord::CarPropertyValue val) {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::SET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    VehiclePropValue* value = emulatorMsg.add_value();
    value->set_prop((int)propId);

    // If area id is empty, return empty EmulatorMessage
    if (val.has_area_id()) {
        value->set_area_id((int)val.area_id());
    }

    // Unused field in emulatorMessage optional int32 status = 2;
    // optional bool bool_value = 3;
    if (val.has_bool_value()) {
        value->add_int32_values(val.bool_value() ? 1 : 0);
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

int SensorReplayPage::convertSensorTypeToSensorId(int sensorType) {
    if(sensorType == ANDROID_SENSOR_TYPE_GYROSCOPE) {
        return ANDROID_SENSOR_GYROSCOPE;
    } else if(sensorType == ANDROID_SENSOR_TYPE_GYROSCOPE_UNCALIBRATED) {
        return ANDROID_SENSOR_GYROSCOPE_UNCALIBRATED;
    } else if(sensorType == ANDROID_SENSOR_TYPE_ACCELEROMETER) {
        return ANDROID_SENSOR_ACCELERATION;
    } else if(sensorType == ANDROID_SENSOR_TYPE_MAGNETIC_FIELD) {
        return ANDROID_SENSOR_MAGNETIC_FIELD;
    } else if(sensorType == ANDROID_SENSOR_TYPE_PRESSURE) {
        return ANDROID_SENSOR_PRESSURE;
    } else {
        return -1;
    }
}

// Add SensorRecord to history list
void SensorReplayPage::addSensorRecord(
        QString itemName,
        QString itemPath,
        android::sensorsessionplayback::SensorSessionPlayback*
                sensorSessionPlayback) {
    SensorReplayItem* sensorReplayItem =
            new SensorReplayItem(mUi->sensor_savedSensorRecordList, itemName,
                                 itemPath, sensorSessionPlayback);

    connect(sensorReplayItem, SIGNAL(editButtonClickedSignal(CCListItem*)),
            this, SLOT(editButtonClicked(CCListItem*)));
}

bool SensorReplayPage::findSensorRecord(QString itemPath) {
    for (int i = 0; i < mUi->sensor_savedSensorRecordList->count(); ++i) {
        QListWidgetItem* item = mUi->sensor_savedSensorRecordList->item(i);
        SensorReplayItem* sensorReplayItem = qobject_cast<SensorReplayItem*>(
                mUi->sensor_savedSensorRecordList->itemWidget(item));

        if (QString::compare(sensorReplayItem->getPath(), itemPath,
                             Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

void SensorReplayPage::editButtonClicked(CCListItem* item) {
    mUi->sensor_savedSensorRecordList->blockSignals(true);

    auto* sensorReplayItem = reinterpret_cast<SensorReplayItem*>(item);
    QMenu* popMenu = new QMenu(this);
    popMenu->setMinimumWidth(100);
    popMenu->setStyleSheet(
            "QMenu::item {padding: 4px;}"
            "QMenu::item:selected {background-color: #4285f4;}");
    QAction* deleteAction = popMenu->addAction(tr("Delete"));

    QAction* theAction = popMenu->exec(QCursor::pos());

    if (theAction == deleteAction) {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        if (isPlaying) {
            QMessageBox msgBox(
                    QMessageBox::Warning, tr("Warning"),
                    tr("Please don't delete this record while playing "),
                    QMessageBox::Cancel, this);
            msgBox.exec();

        } else {
            QMessageBox msgBox(QMessageBox::Warning, tr("Delete record"),
                               tr("Do you want to this record?"),
                               QMessageBox::Cancel, this);
            QPushButton* deleteButton = msgBox.addButton(QMessageBox::Apply);
            deleteButton->setText(tr("Delete"));

            int selection = msgBox.exec();

            if (selection == QMessageBox::Apply) {
                deleteRecord(sensorReplayItem);
            }
        }

        QApplication::restoreOverrideCursor();
    }
    mUi->sensor_savedSensorRecordList->blockSignals(false);
}

void SensorReplayPage::deleteRecord(SensorReplayItem* item) {
    item->removeFromListWidget();

    showSensorRecordDetail(mUi->sensor_savedSensorRecordList->count() != 0);
    if (mUi->sensor_savedSensorRecordList->count() > 0) {
        mUi->sensor_savedSensorRecordList->setCurrentRow(0);
        getCurrentSensorRecordItem()->setEditButtonEnabled(true);
        updatePanel(getCurrentSensorRecordItem()->getSensorSessionPlayback(),
                    getCurrentSensorRecordItem()->getPath());
    }
}

void SensorReplayPage::on_sensor_savedSensorRecordList_currentItemChanged(
        QListWidgetItem* current,
        QListWidgetItem* previous) {
    SensorReplayItem* currentSensorReplayItem = qobject_cast<SensorReplayItem*>(
            mUi->sensor_savedSensorRecordList->itemWidget(current));

    currentSensorReplayItem->setEditButtonEnabled(true);
    currentSensorReplayItem->setSelected(true);

    updatePanel(currentSensorReplayItem->getSensorSessionPlayback(),
                currentSensorReplayItem->getPath());

    if (previous != nullptr) {
        SensorReplayItem* previousSensorReplayItem =
                qobject_cast<SensorReplayItem*>(
                        mUi->sensor_savedSensorRecordList->itemWidget(
                                previous));
        previousSensorReplayItem->setEditButtonEnabled(false);
        previousSensorReplayItem->setSelected(false);
    }
}

// static
void SensorReplayPage::setAgent(const QCarDataAgent* carDataAgent,
                                const QAndroidLocationAgent* locationAgent,
                                const QAndroidSensorsAgent* sensorAgent) {
    if (carDataAgent == nullptr) {
        D("car data agent null");
    }
    sCarDataAgent = carDataAgent;

    if (locationAgent == nullptr) {
        D("location agent null");
    }
    sLocationAgent = locationAgent;

    if (sensorAgent == nullptr) {
        D("sensor agent null");
    }
    sSensorAgent = sensorAgent;
}

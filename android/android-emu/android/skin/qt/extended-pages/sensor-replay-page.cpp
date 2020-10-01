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

#include "VehicleHalProto.pb.h"
#include "android/base/StringFormat.h"  // for StringFormat
#include "android/emulation/control/car_data_agent.h"
#include "android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h"
#include "android/skin/qt/extended-pages/common.h"   // for setButtonEnabled
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

static constexpr float PLAYBACK_SPEED[5] = {1, 2, 5, 10, 100};
static constexpr SensorReplayPage::DurationNs TIMELINE_INTERVAL =
        1000 * 1000 * 1000;

const QCarDataAgent* SensorReplayPage::sCarDataAgent = nullptr;

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
        mUi->sensor_playbackTime->setText(toPlaybackTimeString(0, 0, tr("/")));
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
// When there is a sensor replay item available, use SeekToTime function to
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
        DurationNs time =
                totalTime * (mUi->sensor_playbackSlider->value()) / 100;
        SensorSessionPlayback::SensorSessionStatus status =
            getCurrentSensorRecordItem()->getSensorSessionPlayback()->SeekToTime(time);
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

    SensorSessionPlayback::SensorSessionStatus status =
            currentSensorSessionPlayback->LoadFrom(filePath.toStdString());

    if (status == SensorSessionPlayback::OK) {
        auto parts = filePath.split(QLatin1Char('/'));

        // Add a sensor record to the history list
        addSensorRecord(parts.last(), filePath, currentSensorSessionPlayback);

        updatePanel(currentSensorSessionPlayback, filePath);

        registerSensorSessionPlaybackCallback(currentSensorSessionPlayback);

    } else if (status == SensorSessionPlayback::PROTO_PARSE_FAIL) {
        D("proto parseing failed");
    }
}

void SensorReplayPage::registerSensorSessionPlaybackCallback(
        SensorSessionPlayback* sensorSessionPlayback) {
    sensorSessionPlayback->RegisterCallback(
            [this](emulator::SensorSession::SensorRecord record) {
                // update slider and time ticker
                this->updateTimeLine(record.timestamp_ns());

                if (record.car_property_values_size() > 0) {
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
            QString::fromStdString(sensorSessionPlayback->app_version()));

    // Update Sensor Events count
    mUi->sensor_totalSensorEventsCountValue->setText(
            QString::number(sensorSessionPlayback->event_count()));

    // Set sensor preview
    const std::vector<std::string> sensorList =
            sensorSessionPlayback->sensor_list();
    const std::vector<int> carPropertyIdList =
            sensorSessionPlayback->car_property_id_list();
    QString PreviewText = getPreviewText(sensorList, carPropertyIdList, ";\n");
    QFontMetrics metrics(mUi->sensor_previewValue_expand->font());
    QString elidedText = metrics.elidedText(getPreviewText(sensorList, carPropertyIdList, "; "), Qt::ElideRight,
                                            mUi->sensor_previewValue->width());
    mUi->sensor_previewValue->setText(elidedText);

    // Set sensor preview dialog
    mSensorInfoDialog->setInformativeText(PreviewText);
    QString expand =
            tr(std::to_string(sensorList.size() + carPropertyIdList.size())
                       .c_str())
                    .append(" in total");
    mUi->sensor_previewValue_expand->setText(expand);

    // Set playback time label
    std::vector<int> recordCounts =
            sensorSessionPlayback->getSensorRecordTimeLine(TIMELINE_INTERVAL);

    totalTime = sensorSessionPlayback->session_duration();
    mUi->sensor_playbackTime->setText(
            toPlaybackTimeString(0, totalTime, tr("/")));

    // Update the time line table
    mUi->sensor_eventTimelineTable->clear();
    mUi->sensor_eventTimelineTable->setRowCount(0);
    mUi->sensor_eventTimelineTable->setHorizontalHeaderItem(
            0, new QTableWidgetItem(tr("Time Range")));
    mUi->sensor_eventTimelineTable->setHorizontalHeaderItem(
            1, new QTableWidgetItem(tr("Event Count")));

    DurationNs time = 0;
    for (int row = 0; row < recordCounts.size(); row++) {
        addTimeLineTableRow(
                mUi->sensor_eventTimelineTable,
                toPlaybackTimeString(time, time + TIMELINE_INTERVAL, "~"),
                QString::number(recordCounts[row]), row);
        time += TIMELINE_INTERVAL;
    }

    showSensorRecordDetail(true);
}

QString SensorReplayPage::getPreviewText(
        const std::vector<std::string> sensorList,
        const std::vector<int> carPropertyIdList,
        std::string connector) {
    QString result;

    for (int carPropertyId : carPropertyIdList) {
        QString property =
                carpropertyutils::propMap.count(carPropertyId)
                        ? carpropertyutils::propMap[carPropertyId].label
                        : QObject::tr(std::to_string(carPropertyId).c_str());
        result.append(property);
        result.append(connector.c_str());
    }

    for (std::string sensor : sensorList) {
        result.append(sensor.c_str());
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

// Let's assume that the total duration of the sensor record is less than 1 hr
QString SensorReplayPage::toTimeString(DurationNs time) {
    int secs = time / 1000000000;
    int mins = secs / 60;
    secs = secs % 60;
    QString result = QString::number(mins).append(":");
    if (secs < 10) {
        result.append("0");
    }
    result.append(QString::number(secs));
    return result;
}

QString SensorReplayPage::toPlaybackTimeString(DurationNs current,
                                               DurationNs total,
                                               const QString& connector) {
    QString result =
            toTimeString(current).append(connector).append(toTimeString(total));
    return result;
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
    getCurrentSensorRecordItem()->getSensorSessionPlayback()->StartReplay(
            getPlaybackSpeed());

    // Adjust UI and flags
    isPlaying = true;
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("stop"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "stop");
    mUi->sensor_loadSensorButton->setEnabled(!isPlaying);
    mUi->sensor_savedSensorRecordList->setEnabled(false);
}

void SensorReplayPage::sensorReplayStop() {
    getCurrentSensorRecordItem()->getSensorSessionPlayback()->StopReplay();

    // Adjust UI and flags
    isPlaying = false;
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "play_arrow");
    mUi->sensor_playbackTime->setText(
            toPlaybackTimeString(0, totalTime, tr("/")));
    mUi->sensor_playbackSlider->setValue(0);
    mUi->sensor_loadSensorButton->setEnabled(!isPlaying);
    mUi->sensor_savedSensorRecordList->setEnabled(true);
}

float SensorReplayPage::getPlaybackSpeed() {
    return PLAYBACK_SPEED[mUi->sensor_playbackSpeed->currentIndex()];
}

void SensorReplayPage::updateTimeLine(DurationNs timestamp) {
    mUi->sensor_playbackSlider->setValue(timestamp * 100 / totalTime);
    mUi->sensor_playbackTime->setText(
            toPlaybackTimeString(timestamp, totalTime, tr("/")));

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
void SensorReplayPage::setCarDataAgent(const QCarDataAgent* agent) {
    if (agent == nullptr) {
        D("data agent null");
    }
    sCarDataAgent = agent;
}

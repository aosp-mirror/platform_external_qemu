// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h"
#include "android/skin/qt/extended-pages/sensor-replay-page.h"
#include "android/skin/qt/extended-pages/sensor-replay-item.h"
#include "sensor_session.pb.h"
#include "VehicleHalProto.pb.h"

#include <QFileDialog>    // for QFileDialog
#include <qmessagebox.h>                               // for QMessageBox::C...f
#include <QMenu>


#include "android/base/StringFormat.h"                 // for StringFormat
#include "android/emulation/control/car_data_agent.h"
#include "android/utils/debug.h"
#include "ui_sensor-replay-page.h"  // for CarSensorReplayPage


class QWidget;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)

using android::base::StringFormat;
using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::SensorSession;
using emulator::Status;
using emulator::VehiclePropValue;
using std::string;

static constexpr float PLAYBACK_SPEED[5]  = {1, 2, 5, 10, 100};
static constexpr SensorSessionPlayback::DurationNs TIMELINE_INTERVAL = 1000*1000*1000;

const QCarDataAgent* SensorReplayPage::sCarDataAgent = nullptr;


SensorReplayPage::SensorReplayPage(QWidget* parent)
    :QWidget(parent), mUi(new Ui::SensorReplayPage) {
    mUi->setupUi(this);

    showSensorRecordDetail(false);

    isPlaying = false;

    mSensorInfoDialog = new QMessageBox(
            QMessageBox::NoIcon,
            QString::fromStdString(StringFormat("Sensors Detail")),
            "", QMessageBox::Close, this);
    mSensorInfoDialog->setModal(false);
    mUi->sensorPreviewValue_expand->installEventFilter(this);

}

void SensorReplayPage::showSensorRecordDetail(bool show) {
    mUi->noFileMask->setVisible(!show);
    mUi->fileNameTitle->setVisible(show);
    mUi->filenameValue->setVisible(show);
    mUi->appVersionTitle->setVisible(show);
    mUi->appVersionTitleValue->setVisible(show);
    mUi->totalSensorEventsCountTitle->setVisible(show);
    mUi->totalSensorEventsCountValue->setVisible(show);
    mUi->sensorPreviewTitle->setVisible(show);
    mUi->sensorPreviewValue->setVisible(show);
    mUi->sensorPreviewValue_expand->setVisible(show);
    mUi->eventTimelineTitle->setVisible(show);
    mUi->eventTimelineTable->setVisible(show);
}

void SensorReplayPage::on_sensor_LoadSensorButton_clicked() {
    D("import button clicked");

    QString fullPath = QFileDialog::getOpenFileName(
            this, tr("Open Sensor Record File"), ".");

    if (fullPath.isNull())
        return;


    SensorSessionPlayback* currentSensorSessionPlayback;
    currentSensorSessionPlayback = new SensorSessionPlayback();

    //sensor replay v2
    SensorSessionPlayback::SensorSessionStatus status = currentSensorSessionPlayback->LoadFrom(fullPath.toStdString());

    if (status == SensorSessionPlayback::OK) {
        D("filename parse OK for sensorsessionplayback, show status");
        // Add a sensor record to the history list
        auto parts = fullPath.split(QLatin1Char('/'));
        mUi->filenameValue->setText(parts.last());

        addSensorRecord(fullPath,parts.last());

        D("after add a record update teh panel");

        updatePanel(currentSensorSessionPlayback, fullPath);
        D("set playback");

        getCurrentSensorRecordItem()->setSensorSessionPlayback(currentSensorSessionPlayback);

    } else if (status == SensorSessionPlayback::PROTO_PARSE_FAIL) {
        D("proto parse fail");
    }

}

void SensorReplayPage::updatePanel(SensorSessionPlayback* sensorSessionPlayback, QString fullPath) {
    if (sensorSessionPlayback == nullptr)
    {
        return ;
    }

    // Update app version
    mUi->appVersionTitleValue->setText(QString::fromStdString(sensorSessionPlayback->app_version()));

    // Update Sensor Events count
    mUi->totalSensorEventsCountValue->setText(QString::number(sensorSessionPlayback->event_count()));

    //Set sensor preview
    D("Set sensor preview");
    const std::vector<std::string> sensorList = sensorSessionPlayback->sensor_list();
    const std::vector<int> carPropertyIdList = sensorSessionPlayback->car_property_id_list();
    QString PreviewText = getPreviewText(sensorList, carPropertyIdList);
    QFontMetrics metrics(mUi->sensorPreviewValue_expand->font());
    QString elidedText = metrics.elidedText(
        PreviewText, Qt::ElideRight,
        mUi->sensorPreviewValue->width());
    mUi->sensorPreviewValue->setText(elidedText);

    mSensorInfoDialog->setInformativeText(PreviewText);
    QString expand = tr(std::to_string(sensorList.size() + carPropertyIdList.size()).c_str()).append(" in total");
    mUi->sensorPreviewValue_expand->setText(expand);

    // set playback time label
    D("set playback time label");

    SensorSessionPlayback::DurationNs total;
    std::vector<int> recordCounts;

    sensorSessionPlayback->getSensorRecordTimeLine(&total, &recordCounts, TIMELINE_INTERVAL);

    totalTime = total;
    mUi->playback_time->setText(toConnectTimeLine(0,total,tr("/")));

    // Update the time line table
    D("Update the time line table");

    mUi->eventTimelineTable->clear();
    mUi->eventTimelineTable->setRowCount(0);
    mUi->eventTimelineTable->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Time Range")));
    mUi->eventTimelineTable->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Event Count")));

    SensorSessionPlayback::DurationNs time = 0;
    for (int row = 0; row < recordCounts.size(); row++) {
        D ("record counts %d ", recordCounts[row]);
        addTimeLineTableRow(mUi->eventTimelineTable, toConnectTimeLine(time, time + TIMELINE_INTERVAL, "~"), QString::number(recordCounts[row]), row);
        time += TIMELINE_INTERVAL;
        D("connect itemline %s", toConnectTimeLine(time, time + TIMELINE_INTERVAL, "~").toStdString().c_str());
    }

    // Add the sensorSessionPlayback to the sensor replay item
    showSensorRecordDetail(true);
}

SensorReplayItem* SensorReplayPage::getCurrentSensorRecordItem() {
    QListWidgetItem* currentListWidgetItem = mUi->savedSensorRecordList->currentItem();
    SensorReplayItem* currentSensorReplayItem = qobject_cast<SensorReplayItem*>(mUi->savedSensorRecordList->itemWidget(currentListWidgetItem));
    return currentSensorReplayItem;
}

void SensorReplayPage::addTimeLineTableRow(QTableWidget* table, const QString& timeline, const QString& eventCount, int tableRow) {
    table->insertRow(tableRow);
    table->setItem(tableRow, 0, new QTableWidgetItem(timeline));
    table->setItem(tableRow, 1, new QTableWidgetItem(eventCount));
    D("timeline %s, %s", timeline.toStdString().c_str(), eventCount.toStdString().c_str());

}

// Let's assume that the total duration of the sensor record is less than 1 hr first
QString SensorReplayPage::toTimeLineSingle(SensorSessionPlayback::DurationNs time) {
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


QString SensorReplayPage::toConnectTimeLine(SensorSessionPlayback::DurationNs current, SensorSessionPlayback::DurationNs total, const QString& connector) {
    QString result = toTimeLineSingle(current).append(connector).append(toTimeLineSingle(total));
    return result;
}


void SensorReplayPage::on_sensor_playStopButton_clicked() {
    D("play button clicked");

    if(!isPlaying) {
        D("start click");
        sensorReplayStart();
    } else {
        D("stop click");
        sensorReplayStop();
    }
}

void SensorReplayPage::sensorReplayStart() {
    D ("start play button clicked in car data");
    getCurrentSensorRecordItem()->getSensorSessionPlayback()->RegisterCallback([this](emulator::SensorSession::SensorRecord record) {

        // update slider and time ticker
        this->updateTimeLine(record.timestamp_ns());

        if (record.car_property_values_size() > 0) {
            std::map<int32_t, emulator::SensorSession::SensorRecord::CarPropertyValue> vhal_property_map(record.car_property_values().begin(),
                                        record.car_property_values().end());
            std::map<int32_t, emulator::SensorSession::SensorRecord::CarPropertyValue>::iterator it;

            for ( it = vhal_property_map.begin(); it != vhal_property_map.end(); it++ )
            {
                EmulatorMessage emulatorMsg = makePropMsg((int) it->first, it->second);
                sendCarEmulatorMessageLogged(emulatorMsg, "Replay record is sent");
            }
        }
    });

    getCurrentSensorRecordItem()->getSensorSessionPlayback()->StartReplay(getPlaybackSpeed());
    isPlaying = true;
    // Change the icon on the play/stop button.
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("pause"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "pause");
}

void SensorReplayPage::updateTimeLine(SensorSessionPlayback::DurationNs timestamp) {
    D("update timestamp %" PRId64, timestamp);

    mUi->playback_slider->setValue(timestamp*100/totalTime);
    mUi->playback_time->setText(toConnectTimeLine(timestamp,totalTime,tr("/")));

    if (timestamp == totalTime) {
        sensorReplayStop();
    }
}

float SensorReplayPage::getPlaybackSpeed() {
    return PLAYBACK_SPEED[mUi->playbackSpeed->currentIndex()];
}

void SensorReplayPage::sensorReplayStop() {
    D("stop  button clicked");
    getCurrentSensorRecordItem()->getSensorSessionPlayback()->StopReplay();

    isPlaying = false;
    mUi->sensor_playStopButton->setIcon(getIconForCurrentTheme("play_arrow"));
    mUi->sensor_playStopButton->setProperty("themeIconName", "play_arrow");
    mUi->playback_time->setText(toConnectTimeLine(0,totalTime,tr("/")));
    mUi->playback_slider->setValue(0);

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

void SensorReplayPage::addSensorRecord(QString fullPath, QString fileName) {
    D("Add a record %s, %s", fullPath.toStdString().c_str(), fileName.toStdString().c_str());
    createSensorRecordItem(fileName, fullPath);
}

QString SensorReplayPage::getPreviewText(const std::vector<std::string> sensorList, const std::vector<int> carPropertyIdList) {
    QString result;

    for (int carPropertyId : carPropertyIdList) {
        QString property = carpropertyutils::propMap.count(carPropertyId)? carpropertyutils::propMap[carPropertyId].label : QObject::tr(std::to_string(carPropertyId).c_str());
        result.append(property);
        result.append("; ");
    }

    for (std::string sensor : sensorList)
    {
        result.append(sensor.c_str());
        result.append("; ");
    }

    return result;
}

bool SensorReplayPage::eventFilter(QObject* object, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return false;
    }
    mSensorInfoDialog->show();
    return true;
}

void SensorReplayPage::createSensorRecordItem(QString itemName, QString itemPath) {

    SensorReplayItem* sensorReplayItem = new SensorReplayItem(mUi->savedSensorRecordList,
    itemName, itemPath);
    D("Add an Item to the list");

    connect(sensorReplayItem,
                SIGNAL(editButtonClickedSignal(CCListItem*)), this,
                SLOT(editButtonClicked(CCListItem*)));

}

void SensorReplayPage::editButtonClicked(CCListItem* item) {
    mUi->savedSensorRecordList->blockSignals(true);

    auto* sensorReplayItem = reinterpret_cast<SensorReplayItem*>(item);
    QMenu* popMenu = new QMenu(this);
    popMenu->setMinimumWidth(100);
    popMenu->setStyleSheet("QMenu::item {padding: 4px;}"
        "QMenu::item:selected {background-color: #4285f4;}");
    QAction* deleteAction = popMenu->addAction(tr("Delete"));

    QAction* theAction = popMenu->exec(QCursor::pos());

    if (theAction == deleteAction) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QMessageBox msgBox(QMessageBox::Warning,
                           tr("Delete record"),
                           tr("Do you want to this record?"),
                           QMessageBox::Cancel,
                           this);
        QPushButton *deleteButton = msgBox.addButton(QMessageBox::Apply);
        deleteButton->setText(tr("Delete"));

        int selection = msgBox.exec();

        if (selection == QMessageBox::Apply) {
            deleteRecord(sensorReplayItem);
        }
        QApplication::restoreOverrideCursor();
    }
    mUi->savedSensorRecordList->blockSignals(false);

}

void SensorReplayPage::deleteRecord(SensorReplayItem* item) {
    item->removeFromListWidget();

    showSensorRecordDetail(mUi->savedSensorRecordList->count() != 0);
    if(mUi->savedSensorRecordList->count() > 0) {
        mUi->savedSensorRecordList->setCurrentRow(0);
        getCurrentSensorRecordItem()->setEditButtonEnabled(true);
        updatePanel(getCurrentSensorRecordItem()->getSensorSessionPlayback(), getCurrentSensorRecordItem()->getPath());
    }
}

void SensorReplayPage::on_savedSensorRecordList_currentItemChanged(QListWidgetItem* current,
                                                    QListWidgetItem* previous) {
    D("oncurrent item change ");

    SensorReplayItem* currentSensorReplayItem = qobject_cast<SensorReplayItem*>(mUi->savedSensorRecordList->itemWidget(current));
    D("set edit item");

    currentSensorReplayItem->setEditButtonEnabled(true);

    D("update panel");
    // SensorSessionPlayback::SensorSessionStatus status = mCurrentSensorSessionPlayback->LoadFrom(currentSensorReplayItem->getPath());
    updatePanel(currentSensorReplayItem->getSensorSessionPlayback(), currentSensorReplayItem->getPath());

    D("handel previous");
    if(previous != nullptr) {
        SensorReplayItem* previousSensorReplayItem = qobject_cast<SensorReplayItem*>(mUi->savedSensorRecordList->itemWidget(previous));
        previousSensorReplayItem->setEditButtonEnabled(false);
    }

}

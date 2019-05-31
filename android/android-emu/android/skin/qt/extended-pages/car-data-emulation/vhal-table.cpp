#include "vhal-table.h"

#include "android/skin/qt/extended-pages/car-data-emulation/car-property-table.h"
#include "android/skin/qt/extended-pages/car-data-emulation/checkbox-dialog.h"
#include "android/skin/qt/extended-pages/car-data-emulation/vehicle_constants_generated.h"
#include "ui_vhal-table.h"

using std::map;
using std::string;
using std::vector;

using emulator::EmulatorMessage;
using emulator::MsgType;
using emulator::Status;
using emulator::VehiclePropertyType;
using emulator::VehiclePropGet;
using emulator::VehiclePropValue;

using carpropertyutils::PropertyDescription;
using carpropertyutils::propMap;

// This map is to store the property get back from VHAL
// Key is property name + areaID (idea comes from original property table)
// Value is PropertyDescription
static map<QString, VehiclePropValue> vHalpropertyMap;
static QString selectedKey;

static constexpr int REFRESH_START = 1;
static constexpr int REFRESH_STOP = 2;
static constexpr int REFRESH_PUASE = 3;

static constexpr int64_t REFRESH_INTERVEL = 1000000LL;

VhalTable::VhalTable(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::VhalTable),
      mVhalPropertyTableRefreshThread([this] {
          int msg;
          while (true) {
              mRefreshMsg.tryReceive(&msg);
              if (msg == REFRESH_STOP) {
                  break;
              }
              android::base::AutoLock lock(mVhalPropertyTableRefreshLock);
              switch (msg) {
                  case REFRESH_START:
                      if (mSendEmulatorMsg != nullptr) {
                          sendGetAllPropertiesRequest();
                      }
                      mVhalPropertyTableRefreshCV.timedWait(
                              &mVhalPropertyTableRefreshLock,
                              nextRefreshAbsolute());
                      break;
                  case REFRESH_PUASE:
                      mVhalPropertyTableRefreshCV.wait(
                              &mVhalPropertyTableRefreshLock);
                      break;
              }
          }
      }) {
    mUi->setupUi(this);

    mUi->doubleSpinBox_value->hide();
    mUi->spinBox_value->hide();
    mUi->comboBox_value->hide();

    connect(this, SIGNAL(updateData(QString, QString, QString, QString)), this,
            SLOT(updateTable(QString, QString, QString, QString)),
            Qt::QueuedConnection);
    connect(mUi->property_search, SIGNAL(textEdited(QString)), this,
            SLOT(refresh_filter(QString)));

    // start refresh thread
    mRefreshMsg.trySend(REFRESH_START);
    mVhalPropertyTableRefreshThread.start();
}

VhalTable::~VhalTable() {
    stopVhalPropertyTableRefreshThread();
}

// Check data for selected item
void VhalTable::on_property_list_itemClicked(QListWidgetItem* item) {
    int prop = item->data(PROP_ROLE).toInt();
    int areaId = item->data(AREA_ROLE).toInt();
    QString key = item->data(KEY_ROLE).toString();

    selectedKey = key;

    EmulatorMessage getMsg = makeGetPropMsg(prop, areaId);
    string getLog =
            "Sending get request for " + QString::number(prop).toStdString();
    mSendEmulatorMsg(getMsg, getLog);
}

void VhalTable::on_editButton_clicked() {
    showEditableArea(vHalpropertyMap[selectedKey]);
}

void VhalTable::sendGetAllPropertiesRequest() {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_ALL_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    string log = "Requesting all properties";
    mSendEmulatorMsg(emulatorMsg, log);
}

EmulatorMessage VhalTable::makeGetPropMsg(int32_t prop, int areaId) {
    EmulatorMessage emulatorMsg;
    emulatorMsg.set_msg_type(MsgType::GET_PROPERTY_CMD);
    emulatorMsg.set_status(Status::RESULT_OK);
    VehiclePropGet* getMsg = emulatorMsg.add_prop();
    getMsg->set_prop(prop);
    getMsg->set_area_id(areaId);
    return emulatorMsg;
}

void VhalTable::setSendEmulatorMsgCallback(
        CarSensorData::EmulatorMsgCallback&& func) {
    mSendEmulatorMsg = std::move(func);
}

void VhalTable::showEvent(QShowEvent* event) {
    // clear old property list and map
    // ask for new data.
    mUi->property_list->clear();
    vHalpropertyMap.clear();
    sendGetAllPropertiesRequest();
    setVhalPropertyTableRefreshThread();
}

void VhalTable::hideEvent(QHideEvent*) {
    // stop asking data
    pauseVhalPropertyTableRefreshThread();
}

void VhalTable::updateTable(QString label,
                            QString propertyId,
                            QString areaId,
                            QString key) {
    QListWidgetItem* item = new QListWidgetItem();
    mUi->property_list->addItem(item);
    VhalItem* ci = new VhalItem(nullptr, label,
                                QString::fromStdString("ID : ") + propertyId);
    item->setSizeHint(QSize(80, 56));
    item->setData(PROP_ROLE, propertyId);
    item->setData(AREA_ROLE, areaId);
    item->setData(KEY_ROLE, key);
    mUi->property_list->setItemWidget(item, ci);
}

void VhalTable::processMsg(EmulatorMessage emulatorMsg) {
    if (emulatorMsg.prop_size() > 0 || emulatorMsg.value_size() > 0) {
        QStringList sl;
        for (int valIndex = 0; valIndex < emulatorMsg.value_size();
             valIndex++) {
            VehiclePropValue val = emulatorMsg.value(valIndex);
            if (!propMap.count(val.prop())) {
                // Some received constants are vendor id and aren't on the list.
                // so if constants is vendor id, transfer raw value to hex and
                // build as vender label like Vendor(id: XXX) where XXX is hex
                // representation of the property. If not, show raw value.
                if (carpropertyutils::isVendor(val.prop())) {
                    propMap[val.prop()] = {
                            carpropertyutils::vendorIdToString(val.prop())};
                } else {
                    propMap[val.prop()] = {QString::number(val.prop())};
                }
            }

            PropertyDescription propDesc = propMap[val.prop()];

            QString label = propDesc.label;
            QString area = carpropertyutils::getAreaString(val);
            QString key = label + area;

            // if the return value contains new property
            // like new sensors start during runtime
            if (!vHalpropertyMap.count(key)) {
                sl << key;
            }
            vHalpropertyMap[key] = val;
            // if the return val is the selected property
            // update the description board
            if (QString::compare(key, selectedKey, Qt::CaseSensitive) == 0) {
                showPropertyDescription(val);
            }
        }

        // Sort the keys and emit the output based on keys
        // only delta property will be rendered here
        sl.sort();
        for (int i = 0; i < sl.size(); i++) {
            QString key = sl.at(i);
            VehiclePropValue currVal = vHalpropertyMap[key];
            PropertyDescription currPropDesc = propMap[currVal.prop()];
            QString label = currPropDesc.label;
            QString id = QString::number(currVal.prop());
            QString areaId = QString::number(currVal.area_id());

            emit updateData(label, id, areaId, key);
        }

        // set selectedKey to the first key if selectedKey is empty
        // This should only happen at the first time table is opened
        if (selectedKey.isEmpty() && sl.size() > 0) {
            selectedKey = sl.at(0);
            printf("init selectedKey %s \n", selectedKey.toStdString().c_str());
            showPropertyDescription(vHalpropertyMap[selectedKey]);
        }
    }
}

void VhalTable::showPropertyDescription(VehiclePropValue val) {
    PropertyDescription propDesc = propMap[val.prop()];
    QString label = propDesc.label;
    QString area = carpropertyutils::getAreaString(val);
    QString id = QString::number(val.prop());
    QString value = carpropertyutils::getValueString(val);
    int type = val.value_type();

    mUi->property_name_value->setText(label);
    mUi->area_value->setText(area);
    mUi->property_id_value->setText(id);
    mUi->editable_value->setText(propDesc.writable
                                         ? QString::fromStdString("Yes")
                                         : QString::fromStdString("No"));

    mUi->value_value->setText(value);
    if (propDesc.writable) {
        mUi->editButton->setEnabled(true);
    } else {
        mUi->editButton->setEnabled(false);
    }
}

void VhalTable::showEditableArea(VehiclePropValue val) {
    int type = val.value_type();
    EmulatorMessage writeMsg;
    int areaId = val.area_id();
    string writeLog;
    switch (type) {
        case (int32_t)VehiclePropertyType::BOOLEAN:
            // TODO: show bool input
            printf("show bool dialog\n");
            break;

        case (int32_t)VehiclePropertyType::INT32:
            // TODO: show int input
            printf("show int dialog\n");
            break;

        case (int32_t)VehiclePropertyType::FLOAT:
            // TODO: show float input
            printf("show float dialog\n");
            break;
        case (int32_t)VehiclePropertyType::INT32_VEC:
            // TODO: show mulit selection input
            printf("show mulit selection dialog\n");
            break;
    }
}

void VhalTable::refresh_filter(QString patern) {
    hide_all();
    for (int row(0); row < mUi->property_list->count(); row++) {
        QListWidgetItem* item = mUi->property_list->item(row);
        QString key = item->data(KEY_ROLE).toString();
        if (key.contains(patern, Qt::CaseInsensitive)) {
            item->setHidden(false);
        }
    }
}

void VhalTable::hide_all() {
    for (int row(0); row < mUi->property_list->count(); row++)
        mUi->property_list->item(row)->setHidden(true);
}

// Ask property update every 1s
android::base::System::Duration VhalTable::nextRefreshAbsolute() {
    return android::base::System::get()->getUnixTimeUs() + REFRESH_INTERVEL;
}

void VhalTable::setVhalPropertyTableRefreshThread() {
    mRefreshMsg.trySend(REFRESH_START);
    mVhalPropertyTableRefreshCV.signal();
}

void VhalTable::stopVhalPropertyTableRefreshThread() {
    mRefreshMsg.trySend(REFRESH_STOP);
    mVhalPropertyTableRefreshCV.signal();
    mVhalPropertyTableRefreshThread.wait();
}

void VhalTable::pauseVhalPropertyTableRefreshThread() {
    mRefreshMsg.trySend(REFRESH_PUASE);
}
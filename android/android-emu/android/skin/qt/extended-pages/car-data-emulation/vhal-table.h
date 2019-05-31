#ifndef VHALTABLE_H
#define VHALTABLE_H

#include <QListWidgetItem>
#include <QStringList>
#include <QWidget>

#include "android/base/synchronization/ConditionVariable.h"
#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/system/System.h"
#include "android/base/threads/FunctorThread.h"
#include "android/emulation/proto/VehicleHalProto.pb.h"
#include "android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h"
#include "android/skin/qt/extended-pages/car-data-emulation/car-property-table.h"

#include "vhal-item.h"

#ifdef _MSC_VER
#include "msvc-posix.h"
#else
#include <sys/time.h>
#endif

using android::base::FunctorThread;
using android::base::System;

namespace Ui {
class VhalTable;
}

class VhalTable : public QWidget {
    Q_OBJECT

public:
    explicit VhalTable(QWidget* parent = nullptr);
    ~VhalTable();
    void processMsg(emulator::EmulatorMessage emulatorMsg);
    void setSendEmulatorMsgCallback(CarSensorData::EmulatorMsgCallback&&);

signals:
    void updateData(QString label,
                    QString propertyId,
                    QString areaId,
                    QString key);

private slots:
    void on_property_list_itemClicked(QListWidgetItem* item);
    void on_property_list_currentItemChanged(QListWidgetItem* current,
                                             QListWidgetItem* previous);
    void on_editButton_clicked();
    void updateTable(QString label,
                     QString propertyId,
                     QString areaId,
                     QString key);
    void refresh_filter(QString patern);

private:
    std::unique_ptr<Ui::VhalTable> mUi;

    VhalItem* getItemWidget(QListWidgetItem* listItem);
    void setPropertyText(QLabel* label, QString text);

    void sendGetAllPropertiesRequest();
    CarSensorData::EmulatorMsgCallback mSendEmulatorMsg;
    emulator::EmulatorMessage makeGetPropMsg(int32_t prop, int areaId);

    void showEvent(QShowEvent* event);
    void hideEvent(QHideEvent*);
    void showPropertyDescription(emulator::VehiclePropValue val);
    void showEditableArea(emulator::VehiclePropValue val);
    void hide_all();

    // 0x0100 is the first role that can be used for application-specific
    // purposes. Same as Qt::UserRole
    const int PROP_ROLE = 0x0100;
    const int AREA_ROLE = 0x0101;
    const int KEY_ROLE = 0x0102;

    // table refresh funcitons
    FunctorThread mVhalPropertyTableRefreshThread;
    android::base::MessageChannel<int, 2> mRefreshMsg;
    android::base::ConditionVariable mVhalPropertyTableRefreshCV;
    android::base::Lock mVhalPropertyTableRefreshLock;
    android::base::System::Duration nextRefreshAbsolute();
    void setVhalPropertyTableRefreshThread();
    void stopVhalPropertyTableRefreshThread();
    void pauseVhalPropertyTableRefreshThread();



    emulator::EmulatorMessage makeSetPropMsgInt32(int32_t propId, int val, int areaId);
    emulator::EmulatorMessage makeSetPropMsgFloat(int32_t propId, float val, int areaId);
    emulator::EmulatorMessage makeSetPropMsgInt32Vec(
            int32_t propId,
            const std::vector<int32_t>* val,
            int areaId);
    emulator::EmulatorMessage makeSetPropMsg(int propId, emulator::VehiclePropValue** valueRef,
                                                  int areaId);
    int32_t getUserBoolValue(carpropertyutils::PropertyDescription propDesc,
                              QString oldValueString, bool* pressedOk);
    float getUserFloatValue(carpropertyutils::PropertyDescription propDesc,
                             QString oldValueString, bool* pressedOk);
    int32_t getUserInt32Value(carpropertyutils::PropertyDescription propDesc,
                               QString oldValueString, bool* pressedOk);
    const std::vector<int32_t>* getUserInt32VecValue(
            carpropertyutils::PropertyDescription propDesc,
            QString oldValueString,
            bool* pressedOk);
};

#endif  // VHALTABLE_H

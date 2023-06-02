// Copyright (C) 2015 The Android Open Source Project
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

#include <qobjectdefs.h>     // for Q_OBJECT, slots
#include <QString>           // for QString
#include <QWidget>           // for QWidget
#include <initializer_list>  // for initializer_list
#include <memory>            // for unique_ptr
#include <utility>           // for pair

#include "android/emulation/control/utils/SimpleEmulatorControlClient.h"
#include "ui_battery-page-grpc.h"  // for BatteryPageGrpc

namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::emulation::control::BatteryState;
using android::emulation::control::SimpleEmulatorControlClient;
using android::metrics::UiEventTracker;

namespace android {
namespace emulation {
namespace grpc {
namespace ui {

class BatteryPageGrpc : public QWidget {
    Q_OBJECT

public:
    explicit BatteryPageGrpc(QWidget* parent = 0);

private slots:
    void on_bat_chargerBox_activated(int value);
    void on_bat_healthBox_activated(int index);
    void on_bat_levelSlider_valueChanged(int value);
    void on_bat_statusBox_activated(int index);


private:
    void populateListBox(
            QComboBox* list,
            std::initializer_list<std::pair<int, const char*>> associations);

private:
    std::unique_ptr<Ui::BatteryPageGrpc> mUi;
    SimpleEmulatorControlClient mEmulatorControl;
    BatteryState mState;
    std::shared_ptr<UiEventTracker> mDropDownTracker;
};
}  // namespace ui
}  // namespace grpc
}  // namespace emulation
}  // namespace android
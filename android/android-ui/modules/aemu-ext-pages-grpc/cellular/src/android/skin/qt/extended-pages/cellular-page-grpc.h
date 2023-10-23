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

#include <QObject>
#include <QString>
#include <QWidget>
#include <memory>

#include "android/emulation/control/utils/ModemClient.h"
#include "ui_cellular-page-grpc.h"

struct QAndroidCellularAgent;

namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::emulation::control::ModemClient;
using android::metrics::UiEventTracker;
using android::emulation::control::incubating::CellInfo;

class CellularPageGrpc : public QWidget {
    Q_OBJECT

public:
    explicit CellularPageGrpc(QWidget* parent = nullptr);
    static bool
    simIsPresent();  // Returns true if the user wants a SIM present,
                     // considering both command line and UI.

private slots:
    void on_cell_dataStatusBox_currentIndexChanged(int index);
    void on_cell_standardBox_currentIndexChanged(int index);
    void on_cell_voiceStatusBox_currentIndexChanged(int index);
    void on_cell_signalStatusBox_currentIndexChanged(int index);
    void on_cell_meterStatusBox_currentIndexChanged(int index);

    // TODO: Implement Network delay setting
    // http://developer.android.com/tools/devices/emulator.html#netdelay
private:
    void loadCellState();
    void updateCellState();

    std::unique_ptr<Ui::CellularPageGrpc> mUi;
    std::shared_ptr<UiEventTracker> mDropDownTracker;
    ModemClient mModemClient;
    CellInfo mCurrentState;
};

/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#ifndef BATTERY_WINDOW_H
#define BATTERY_WINDOW_H

#include <QFrame>
#include <QCheckBox>
#include <QAbstractSlider>

#include "android/battery-if.h"

class EmulatorWindow;

namespace Ui {
    class BatteryWindow;
}

class BatteryWindow : public QFrame
{
    Q_OBJECT

public:
    explicit BatteryWindow(EmulatorWindow *eW,
                           void*& persistentPtr,
                           const BatteryIf *batteryIf);

private:

    class BatteryState {
    public:
        bool   isCharging;
        int    chargeLevel; // Percent

        BatteryState() {
            isCharging  = true;
            chargeLevel = 50;
        }
    };

    ~BatteryWindow();
    void closeEvent(QCloseEvent *ce);

    EmulatorWindow  *parentWindow;
    const BatteryIf *batteryIf;
    QCheckBox       *chargingCkBox;
    BatteryState    *batteryState;
    QAbstractSlider *batteryLevelSlider;

private slots:
    void slot_chargingCk(bool checked);
    void slot_batteryLevelSlider(int value);

};

#endif // BATTERY_WINDOW_H

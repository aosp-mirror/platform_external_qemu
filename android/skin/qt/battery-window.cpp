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

#include <QtWidgets>
#include <QGridLayout>

#include "android/battery-if.h"
#include "android/skin/qt/emulator-window.h"
#include "android/skin/qt/battery-window.h"

BatteryWindow::BatteryWindow(EmulatorWindow *eW,
                             void*& persistentPtr,
                             const BatteryIf *bIf) :
    QFrame(eW),
    parentWindow(eW),
    batteryIf(bIf),
    chargingCkBox(NULL),
    batteryState(NULL),
    batteryLevelSlider(NULL)
{
    Q_INIT_RESOURCE(resources);

    if (persistentPtr == NULL) {
        // We have no battery state yet.
        // Create it.
        batteryState = new BatteryState();
        // Our parent will maintain this structure
        // even if we exit.
        persistentPtr = batteryState;
    } else {
        // Use the existing state
        batteryState = (BatteryState *)persistentPtr;
    }

    setWindowFlags(Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    QGridLayout *layout = new QGridLayout(this);
    setLayout(layout);
    setStyleSheet(QString("QWidget { background: #2c3239 }"));

    // Charging?
    chargingCkBox = new QCheckBox;
    chargingCkBox->setText(tr("Charging"));
    chargingCkBox->setChecked(batteryState->isCharging);
    layout->addWidget(chargingCkBox);
    QObject::connect(chargingCkBox, &QCheckBox::toggled,
                     this, &BatteryWindow::slot_chargingCk);
    
    // Charge level
    batteryLevelSlider = new QAbstractSlider;
    batteryLevelSlider->setOrientation(Qt::Horizontal);
    batteryLevelSlider->setFocusPolicy(Qt::StrongFocus);
    batteryLevelSlider->setTracking(true);
    batteryLevelSlider->setMinimum(  0);
    batteryLevelSlider->setMaximum(100);

    batteryLevelSlider->setSliderPosition(batteryState->chargeLevel);
    
    layout->addWidget(batteryLevelSlider);
    QObject::connect(batteryLevelSlider, &QAbstractSlider::valueChanged,
                     this, &BatteryWindow::slot_batteryLevelSlider);

    // Battery health
    // (Radio buttons?)
    
    // Battery status
    // (What widget?)

    move(parentWindow->geometry().right(),
         (( parentWindow->geometry().top() + parentWindow->geometry().bottom() ) * 3) / 4 );

    setWindowTitle(tr("Power / Battery"));
    setMinimumSize(200, 200);
}

BatteryWindow::~BatteryWindow()
{
    // Do not delete 'batteryState'--we'll want it if
    // this window gets re-created.
}

void BatteryWindow::closeEvent(QCloseEvent *ce) {
    parentWindow->BatteryWindowIsClosing();
}

void BatteryWindow::slot_chargingCk(bool checked) {
    if (batteryState && batteryIf) {
        batteryState->isCharging = checked;
        batteryIf->setIsCharging(checked);
    }
}

void BatteryWindow::slot_batteryLevelSlider(int value)
{
    if (batteryState && batteryIf) {
        batteryState->chargeLevel = value;
        batteryIf->setChargeLevel(value);
    }
}

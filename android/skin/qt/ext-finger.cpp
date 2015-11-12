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

#include "android/emulation/control/finger_agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "ui_extended.h"

#include <QtWidgets>

void ExtendedWindow::initFinger()
{
    // Initialize 10 "fingers", each with a fixed
    // random fingerprint ID value
    mExtendedUi->finger_pickBox->addItem("Finger 1",   45146572);
    mExtendedUi->finger_pickBox->addItem("Finger 2",  192618075);
    mExtendedUi->finger_pickBox->addItem("Finger 3",   84807873);
    mExtendedUi->finger_pickBox->addItem("Finger 4",  189675793);
    mExtendedUi->finger_pickBox->addItem("Finger 5",  132710472);
    mExtendedUi->finger_pickBox->addItem("Finger 6",   36321043);
    mExtendedUi->finger_pickBox->addItem("Finger 7",  139425534);
    mExtendedUi->finger_pickBox->addItem("Finger 8",   15301340);
    mExtendedUi->finger_pickBox->addItem("Finger 9",  105702233);
    mExtendedUi->finger_pickBox->addItem("Finger 10",  87754286);
}

void ExtendedWindow::on_finger_touchButton_pressed()
{
    // Send the ID associated with the selected fingerprint
    int fingerID = mExtendedUi->finger_pickBox->currentData().toInt();

    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(true, fingerID);
    }
}

void ExtendedWindow::on_finger_touchButton_released()
{
    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(false, 0);
    }
}

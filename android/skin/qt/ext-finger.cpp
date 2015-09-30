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
{ }

void ExtendedWindow::on_finger_touchButton_pressed()
{
    bool OK = false;
    int  id = mExtendedUi->finger_IdBox->toPlainText().toInt(&OK);

    if ( !OK || id <= 0 ) {
        mToolWindow->showErrorDialog(tr("The \"Fingerprint ID\" number is invalid."),
                                     tr("Finger ID"));
        return;
    }

    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(true, id);
    }
}

void ExtendedWindow::on_finger_touchButton_released()
{
    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(false, 0);
    }
}

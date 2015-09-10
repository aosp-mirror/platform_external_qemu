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

#include "android/finger-agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "ui_extended.h"

#include <QtWidgets>

void ExtendedWindow::initFinger()
{ }

void ExtendedWindow::on_finger_touchCkBox_toggled(bool checked)
{
    bool OK = false;
    int  id = mExtendedUi->finger_IdBox->toPlainText().toInt(&OK);

    if (checked && !OK) {
        mToolWindow->showErrorDialog(tr("The \"Fingerprint ID\" number is invalid."),
                                     tr("Finger ID"));
        mExtendedUi->finger_IdBox->setEnabled(true);
        mExtendedUi->finger_touchCkBox->setChecked(false);
        return;
    }

    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(checked, id);
    }

    // Don't allow changing the ID while a touch is active
    mExtendedUi->finger_IdBox->setEnabled( !checked );
}

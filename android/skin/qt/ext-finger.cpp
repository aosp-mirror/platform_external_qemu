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
    // The ID that this code associates with each "finger"
    // is a fixed value (that is essentially random).
    static const int fingerID[10] =
            { 45146572, 192618075, 84807873, 189675793, 132710472,
              36321043, 139425534, 15301340, 105702233,  87754286  };

    int idx = mExtendedUi->finger_pickBox->currentIndex();

    if (idx < 0  ||  idx > 10) {
        mToolWindow->showErrorDialog(tr("The finger index is invalid"),
                                     tr("Fingerprint"));
        return;
    }

    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(true, fingerID[idx]);
    }
}

void ExtendedWindow::on_finger_touchButton_released()
{
    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(false, 0);
    }
}

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

#include "android/skin/qt/extended-pages/finger-page.h"
#include "android/emulation/control/finger_agent.h"

#include "android/globals.h"

FingerPage::FingerPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::FingerPage())
{
    mUi->setupUi(this);

    int apiLevel = avdInfo_getApiLevel(android_avdInfo);

    if (apiLevel >= 23) {
        // This API level supports fingerprint recognition.
        // Initialize 10 "fingers", each with a fixed
        // random fingerprint ID value
        mUi->finger_pickBox->addItem("Finger 1",   45146572);
        mUi->finger_pickBox->addItem("Finger 2",  192618075);
        mUi->finger_pickBox->addItem("Finger 3",   84807873);
        mUi->finger_pickBox->addItem("Finger 4",  189675793);
        mUi->finger_pickBox->addItem("Finger 5",  132710472);
        mUi->finger_pickBox->addItem("Finger 6",   36321043);
        mUi->finger_pickBox->addItem("Finger 7",  139425534);
        mUi->finger_pickBox->addItem("Finger 8",   15301340);
        mUi->finger_pickBox->addItem("Finger 9",  105702233);
        mUi->finger_pickBox->addItem("Finger 10",  87754286);

        // Don't show the overlay that obscures the controls.
        mUi->finger_noFinger_mask->hide();
    } else {
        // This API level does not support fingerprint
        // recognition.
        // Overlay a mask and text saying that this is disabled.
        QString badApi = tr("This emulated device is using API level ");
        badApi += QString::number(apiLevel);
        QString dessertName = apiDessert(apiLevel);
        if ( !dessertName.isEmpty() ) {
            badApi += " (" + dessertName + ")";
        }
        badApi += tr(".<br>Fingerprint recognition is only available "
                     "for API level 23 (Marshmallow) and above.");
        mUi->finger_noFinger_mask->setTextFormat(Qt::RichText);
        mUi->finger_noFinger_mask->setText(badApi);
        mUi->finger_noFinger_mask->raise();
    }
}

void FingerPage::on_finger_touchButton_pressed()
{
    // Send the ID associated with the selected fingerprint
    int fingerID = mUi->finger_pickBox->currentData().toInt();

    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(true, fingerID);
    }
}

void FingerPage::on_finger_touchButton_released()
{
    if (mFingerAgent && mFingerAgent->setTouch) {
        mFingerAgent->setTouch(false, 0);
    }
}

void FingerPage::setFingerAgent(const QAndroidFingerAgent* agent)
{
    mFingerAgent = agent;
}

QString FingerPage::apiDessert(int apiLevel) {
    // This information was taken from the SDK Manager:
    // Appearances & Behavior > System Settings > Android SDK > SDK Platforms
    if (apiLevel == 10) return "Gingerbread";        // 10
    if (apiLevel <= 13) return "";                   // No names for 0..9, 11..13
    if (apiLevel <= 15) return "Ice Cream Sandwich"; // 14..15
    if (apiLevel <= 18) return "Jelly Bean";         // 16..18
    if (apiLevel <= 20) return "KitKat";             // 19..20
    if (apiLevel <= 22) return "Lollipop";           // 21..22
    if (apiLevel <= 23) return "Marshmallow";        // 23

    return "";
}

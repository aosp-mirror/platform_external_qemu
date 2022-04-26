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

#include <QComboBox>                                 // for QComboBox
#include <QLabel>                                    // for QLabel
#include <QVariant>                                  // for QVariant
#include <Qt>                                        // for RichText

#include "android/avd/info.h"                        // for avdInfo_getApiDe...
#include "android/emulation/VmLock.h"                // for RecursiveScopedV...
#include "android/emulation/control/finger_agent.h"  // for QAndroidFingerAgent
#include "android/globals.h"                         // for getConsoleAgents()->settings->avdInfo()
#include "android/metrics/UiEventTracker.h"          // for UiEventTracker
#include "studio_stats.pb.h"                         // for EmulatorUiEvent

static const QAndroidFingerAgent* sFingerAgent = nullptr;

FingerPage::FingerPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::FingerPage()),
    mFingerTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::BUTTON_PRESS,
              android_studio::EmulatorUiEvent::EXTENDED_FINGER_TAB))
{
    mUi->setupUi(this);

    int apiLevel = avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo());

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
        QString dessertName = avdInfo_getApiDessertName(apiLevel);
        if ( !dessertName.isEmpty() ) {
            dessertName = " (" + dessertName + ")";
        }

        QString badApi = tr("This emulated device is using API level %1%2.<br>"
                            "Fingerprint recognition is available with API level "
                            "23 (Marshmallow) and higher only.")
                         .arg(apiLevel).arg(dessertName);

        mUi->finger_noFinger_mask->setTextFormat(Qt::RichText);
        mUi->finger_noFinger_mask->setText(badApi);
        mUi->finger_noFinger_mask->raise();
    }
}

void FingerPage::on_finger_touchButton_pressed()
{
    mFingerTracker->increment("FINGER");
    // Send the ID associated with the selected fingerprint
    int fingerID = mUi->finger_pickBox->currentData().toInt();

    android::RecursiveScopedVmLock vmlock;
    if (sFingerAgent && sFingerAgent->setTouch) {
        sFingerAgent->setTouch(true, fingerID);
    }
}

void FingerPage::on_finger_touchButton_released()
{
    android::RecursiveScopedVmLock vmlock;
    if (sFingerAgent && sFingerAgent->setTouch) {
        sFingerAgent->setTouch(false, 0);
    }
}

// static
void FingerPage::setFingerAgent(const QAndroidFingerAgent* agent)
{
    android::RecursiveScopedVmLock vmlock;
    sFingerAgent = agent;
}

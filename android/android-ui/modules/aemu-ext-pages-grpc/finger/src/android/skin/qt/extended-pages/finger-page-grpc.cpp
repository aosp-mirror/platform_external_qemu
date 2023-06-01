// Copyright 2023 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "android/skin/qt/extended-pages/finger-page-grpc.h"

#include <QComboBox>
#include <QLabel>
#include <QVariant>
#include <Qt>

#include "android/avd/info.h"
#include "android/console.h"
#include "android/metrics/UiEventTracker.h"
#include "studio_stats.pb.h"

using android::emulation::control::EmulatorGrpcClient;
using android::emulation::control::Fingerprint;

FingerPageGrpc::FingerPageGrpc(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::FingerPageGrpc()),
      mFingerTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::BUTTON_PRESS,
              android_studio::EmulatorUiEvent::EXTENDED_FINGER_TAB)),
      mEmulatorControl(EmulatorGrpcClient::me()) {
    mUi->setupUi(this);

    int apiLevel = avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo());

    if (apiLevel >= 23) {
        mUi->finger_pickBox->addItem("Finger 1", 45146572);
        mUi->finger_pickBox->addItem("Finger 2", 192618075);
        mUi->finger_pickBox->addItem("Finger 3", 84807873);
        mUi->finger_pickBox->addItem("Finger 4", 189675793);
        mUi->finger_pickBox->addItem("Finger 5", 132710472);
        mUi->finger_pickBox->addItem("Finger 6", 36321043);
        mUi->finger_pickBox->addItem("Finger 7", 139425534);
        mUi->finger_pickBox->addItem("Finger 8", 15301340);
        mUi->finger_pickBox->addItem("Finger 9", 105702233);
        mUi->finger_pickBox->addItem("Finger 10", 87754286);

        mUi->finger_noFinger_mask->hide();
    } else {
        QString dessertName = avdInfo_getApiDessertName(apiLevel);
        if (!dessertName.isEmpty()) {
            dessertName = " (" + dessertName + ")";
        }

        QString badApi =
                tr("This emulated device is using API level %1%2.<br>"
                   "Fingerprint recognition is available with API level "
                   "23 (Marshmallow) and higher only.")
                        .arg(apiLevel)
                        .arg(dessertName);

        mUi->finger_noFinger_mask->setTextFormat(Qt::RichText);
        mUi->finger_noFinger_mask->setText(badApi);
        mUi->finger_noFinger_mask->raise();
    }
}

void FingerPageGrpc::on_finger_touchButton_pressed() {
    mFingerTracker->increment("FINGER");

    int fingerID = mUi->finger_pickBox->currentData().toInt();

    Fingerprint fp;
    fp.set_istouching(true);
    fp.set_touchid(fingerID);
    mEmulatorControl.sendFingerprint(fp, [](auto _ignored) {});
}

void FingerPageGrpc::on_finger_touchButton_released() {
    Fingerprint fp;
    fp.set_istouching(false);
    fp.set_touchid(0);
    mEmulatorControl.sendFingerprint(fp, [](auto _ignored) {});
}

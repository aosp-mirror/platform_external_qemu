// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/microphone-page.h"

#include "android/emulation/control/vm_operations.h"
#include "android/hw-events.h"
#include "android/skin/event.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include <QSettings>

MicrophonePage::MicrophonePage(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::MicrophonePage()),
      mEmulatorWindow(nullptr) {
    mUi->setupUi(this);

    // The Hook button is not functional yet.
    // Hide it for now.
    mUi->mic_hookButton->hide();
}

void MicrophonePage::on_mic_hasMic_toggled(bool checked) {
    if (mUi->mic_inserted->isChecked()) {
        // The headset is inserted, give our new microphone
        // status to the device.
        forwardGenericEventToEmulator(EV_SW, SW_MICROPHONE_INSERT,
                                      checked ? 1 : 0);
        forwardGenericEventToEmulator(EV_SYN, 0, 0);
    }
}

void MicrophonePage::on_mic_hookButton_pressed() {
    forwardGenericEventToEmulator(EV_KEY, KEY_HEADSETHOOK, 1);
}

void MicrophonePage::on_mic_hookButton_released() {
    forwardGenericEventToEmulator(EV_KEY, KEY_HEADSETHOOK, 0);
}

void MicrophonePage::on_mic_inserted_toggled(bool checked) {
    // Enable or disable the subordinate controls
    mUi->mic_hasMic->setEnabled(checked);
    setButtonEnabled(mUi->mic_voiceAssistButton, getSelectedTheme(), checked);
    setButtonEnabled(mUi->mic_hookButton, getSelectedTheme(), checked);

    // Send the indication to the device
    int phonesInserted;
    int micInserted;

    if (checked) {
        // Headphones + optional microphone
        phonesInserted = 1;
        micInserted = mUi->mic_hasMic->isChecked() ? 1 : 0;
    } else {
        // No headphones, no microphone
        phonesInserted = 0;
        micInserted = 0;
    }

    forwardGenericEventToEmulator(EV_SW, SW_HEADPHONE_INSERT, phonesInserted);
    forwardGenericEventToEmulator(EV_SW, SW_MICROPHONE_INSERT, micInserted);
    forwardGenericEventToEmulator(EV_SYN, 0, 0);
}

void MicrophonePage::on_mic_allowRealAudio_toggled(bool checked) {
    gQAndroidVmOperations->allowRealAudio(checked);
}

void MicrophonePage::on_mic_voiceAssistButton_pressed() {
    forwardGenericEventToEmulator(EV_KEY, KEY_SEND, 1);
}

void MicrophonePage::on_mic_voiceAssistButton_released() {
    forwardGenericEventToEmulator(EV_KEY, KEY_SEND, 0);
}

void MicrophonePage::setEmulatorWindow(EmulatorQtWindow* eW) {
    mEmulatorWindow = eW;
}

void MicrophonePage::forwardGenericEventToEmulator(int type,
                                                   int code,
                                                   int value) {
    if (mEmulatorWindow) {
        SkinEvent* skin_event = new SkinEvent();
        skin_event->type = kEventGeneric;
        SkinEventGenericData& genericData = skin_event->u.generic_event;
        genericData.type = type;
        genericData.code = code;
        genericData.value = value;

        mEmulatorWindow->queueSkinEvent(skin_event);
    }
}

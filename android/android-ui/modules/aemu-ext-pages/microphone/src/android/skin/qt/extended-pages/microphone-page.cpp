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

#include <QCheckBox>                                  // for QCheckBox

#include "android/skin/backend-defs.h"
#include "android/avd/info.h"                         // for avdInfo_getAvdF...
#include "android/avd/util.h"                         // for AVD_ANDROID_AUTO
#include "android/console.h"                          // for getConsoleAgents
#include "host-common/vm_operations.h"  // for QAndroidVmOpera...
#include "android/emulation/control/adb/AdbInterface.h"
#include "android/hw-events.h"                        // for EV_KEY, EV_SW
#include "android/metrics/UiEventTracker.h"           // for UiEventTracker
#include "android/skin/keycode.h"                     // for kKeyCodeAssist
#include "android/skin/event.h"                       // for SkinEvent, Skin...
#include "android/skin/qt/emulator-qt-window.h"       // for EmulatorQtWindow
#include "android/skin/qt/extended-pages/common.h"    // for getSelectedTheme
#include "android/skin/qt/qt-settings.h"              // for PER_AVD_SETTIN...
#include "android/skin/qt/raised-material-button.h"   // for RaisedMaterialB...
#include "studio_stats.pb.h"                          // for EmulatorUiEvent

#include "host-common/FeatureControl.h"
#include "host-common/feature_control.h"

namespace {
QString getAvdSettingsFile() {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        return QString(avdPath) + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
    } else {
        return {};
    }
}

QVariant getSavedSetting(const char* avdSpecificKey, const char* globalKey,
                         const QVariant& defaultValue) {
    const QString avdSettingsFile = getAvdSettingsFile();
    if (avdSettingsFile.isEmpty()) {
        QSettings settings;
        return settings.value(globalKey, defaultValue);
    } else {
        QSettings settings(avdSettingsFile, QSettings::IniFormat);
        return settings.value(avdSpecificKey, defaultValue);
    }
}

void saveSetting(const char* avdSpecificKey, const char* globalKey,
                 const QVariant& value) {
    const QString avdSettingsFile = getAvdSettingsFile();
    if (avdSettingsFile.isEmpty()) {
        QSettings settings;
        settings.setValue(globalKey, value);
    } else {
        QSettings settings(avdSettingsFile, QSettings::IniFormat);
        settings.setValue(avdSpecificKey, value);
    }
}

bool getSavedMicInserted() {
    return getSavedSetting(Ui::Settings::PER_AVD_MIC_INSERTED,
                           Ui::Settings::MIC_INSERTED, false).toBool();
}

void saveHasMicInserted(const bool value) {
    saveSetting(Ui::Settings::PER_AVD_MIC_INSERTED,
                Ui::Settings::MIC_INSERTED, value);
}

bool getSavedHasMic() {
    return getSavedSetting(Ui::Settings::PER_AVD_MIC_AVAILABLE,
                           Ui::Settings::MIC_AVAILABLE, false).toBool();
}

void saveHasMic(const bool value) {
    saveSetting(Ui::Settings::PER_AVD_MIC_AVAILABLE,
                Ui::Settings::MIC_AVAILABLE, value);
}

bool getSavedMicAllowRealAudio() {
    return getSavedSetting(Ui::Settings::PER_AVD_MIC_ALLOW_READ_AUDIO,
                           Ui::Settings::MIC_ALLOW_READ_AUDIO, false).toBool();
}

void saveMicAllowRealAudio(const bool value) {
    saveSetting(Ui::Settings::PER_AVD_MIC_ALLOW_READ_AUDIO,
                Ui::Settings::MIC_ALLOW_READ_AUDIO, value);
}
}  // namespace

namespace fc = android::featurecontrol;

MicrophonePage::MicrophonePage(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::MicrophonePage()),
             mMicTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::BUTTON_PRESS,
              android_studio::EmulatorUiEvent::EXTENDED_MIC_TAB)),
      mEmulatorWindow(nullptr),
      mDeviceKeyboardHasAssistKey(fc::isEnabled(fc::DeviceKeyboardHasAssistKey)) {
    mUi->setupUi(this);

    mUi->mic_inserted->setCheckState(getSavedMicInserted() ? Qt::Checked : Qt::Unchecked);
    mUi->mic_hasMic->setCheckState(getSavedHasMic() ? Qt::Checked : Qt::Unchecked);
    mUi->mic_allowRealAudio->setCheckState(getSavedMicAllowRealAudio() ? Qt::Checked : Qt::Unchecked);

    // The Hook button is not functional yet.
    // Hide it for now.
    mUi->mic_hookButton->hide();

    if (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) == AVD_ANDROID_AUTO) {
        // Android Auto doesn't support the key event used in voice assist button
        mUi->mic_voiceAssistButton->setHidden(true);
    }

    if (avdInfo_getApiLevel(getConsoleAgents()->settings->avdInfo()) < 24) {
        // Older api levels do not support the key event used in
        // voice assist button
        mUi->mic_voiceAssistButton->setHidden(true);
    }
}

void MicrophonePage::on_mic_hasMic_toggled(bool checked) {
    saveHasMic(checked);

    if (mUi->mic_inserted->isChecked()) {
        // The headset is inserted, give our new microphone
        // status to the device.
        forwardGenericEventToEmulator(EV_SW, SW_MICROPHONE_INSERT,
                                      checked ? 1 : 0);
        forwardGenericEventToEmulator(EV_SYN, 0, 0);
    }
}

void MicrophonePage::on_mic_hookButton_pressed() {
    forwardKeyToEmulator(KEY_HEADSETHOOK, true);
}

void MicrophonePage::on_mic_hookButton_released() {
    forwardKeyToEmulator(KEY_HEADSETHOOK, false);
}

void MicrophonePage::on_mic_inserted_toggled(bool checked) {
    saveHasMicInserted(checked);

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
    saveMicAllowRealAudio(checked);
    getConsoleAgents()->vm->allowRealAudio(checked);
}

void MicrophonePage::on_mic_voiceAssistButton_pressed() {
    if (mDeviceKeyboardHasAssistKey) {
        forwardKeyToEmulator(kKeyCodeAssist, true);
    }
}

void MicrophonePage::on_mic_voiceAssistButton_released() {
    if (mDeviceKeyboardHasAssistKey) {
        forwardKeyToEmulator(kKeyCodeAssist, false);
    } else {
        // for devices without KEYCODE_ASSIST in the .kl file, send it via adb
        auto adbInterface = android::emulation::AdbInterface::getGlobal();
        if (adbInterface) {
            adbInterface->enqueueCommand({"shell", "input", "keyevent", "KEYCODE_ASSIST"});
        }
    }
}

void MicrophonePage::setEmulatorWindow(EmulatorQtWindow* eW) {
    mEmulatorWindow = eW;
}

void MicrophonePage::forwardGenericEventToEmulator(int type,
                                                   int code,
                                                   int value) {
    if (mEmulatorWindow) {
        SkinEvent skin_event;
        skin_event.type = kEventGeneric;
        SkinEventGenericData& genericData = skin_event.u.generic_event;
        genericData.type = type;
        genericData.code = code;
        genericData.value = value;

        mEmulatorWindow->queueSkinEvent(std::move(skin_event));
    }
}

void MicrophonePage::forwardKeyToEmulator(uint32_t keycode, bool down) {
    SkinEvent skin_event;
    skin_event.type = down ? kEventKeyDown : kEventKeyUp;
    skin_event.u.key.keycode = keycode;
    skin_event.u.key.mod = 0;
    mEmulatorWindow->queueSkinEvent(std::move(skin_event));
}

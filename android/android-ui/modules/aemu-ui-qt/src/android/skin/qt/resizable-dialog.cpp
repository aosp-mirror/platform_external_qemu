// Copyright (C) 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "android/skin/qt/resizable-dialog.h"

#include <qsettings.h>

#include "android/avd/info.h"             // for avdInfo_getApiL...
#include "android/avd/util.h"             // for path_getAvdCont...
#include "android/console.h"              // for android_hw, and...
#include "android/skin/qt/qt-settings.h"  // for PER_AVD_SETTING...
#include "ui_resizable-dialog.h"

ResizableDialog::ResizableDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResizableDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Popup);
    // Hide desktop mode. Wait for the new desktop AVD configuration
    // consolidated
    ui->size_desktop->hide();
}

void ResizableDialog::on_size_phone_clicked() {
    saveSize(PRESET_SIZE_PHONE);
    emit newResizableRequested(PRESET_SIZE_PHONE);
    accept();   // hides dialog
}

void ResizableDialog::on_size_unfolded_clicked() {
    saveSize(PRESET_SIZE_UNFOLDED);
    emit newResizableRequested(PRESET_SIZE_UNFOLDED);
    accept();   // hides dialog
}

void ResizableDialog::on_size_tablet_clicked() {
    saveSize(PRESET_SIZE_TABLET);
    emit newResizableRequested(PRESET_SIZE_TABLET);
    accept();   // hides dialog
}

void ResizableDialog::on_size_desktop_clicked() {
    saveSize(PRESET_SIZE_DESKTOP);
    emit newResizableRequested(PRESET_SIZE_DESKTOP);
    accept();   // hides dialog
}

void ResizableDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
}

ResizableDialog::~ResizableDialog()
{
    delete ui;
}

void ResizableDialog::saveSize(PresetEmulatorSizeType sizeType) {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        avdSpecificSettings.setValue(Ui::Settings::PER_AVD_RESIZABLE_SIZE,
                                     sizeType);
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        settings.setValue(Ui::Settings::RESIZABLE_SIZE, sizeType);
    }
}

PresetEmulatorSizeType ResizableDialog::getSize() {
    const char* avdPath = path_getAvdContentPath(getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
        return static_cast<PresetEmulatorSizeType>(avdSpecificSettings
                .value(Ui::Settings::PER_AVD_RESIZABLE_SIZE, PRESET_SIZE_UNFOLDED)
                .toInt());
    } else {
        // Use the global settings if no AVD.
        QSettings settings;
        return static_cast<PresetEmulatorSizeType>(settings
                .value(Ui::Settings::RESIZABLE_SIZE, PRESET_SIZE_UNFOLDED)
                .toInt());
    }
}

// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// This file contains the methods for the Proxy
// tab of the Settings page

#include "android/skin/qt/extended-pages/settings-page.h"

#include "android/emulator-window.h"
#include "android/skin/qt/qt-settings.h"

#include <QSettings>

void SettingsPage::initProxy() {

    // Recall the user's previous settings
    QSettings settings;

    bool useStudio = settings.value(Ui::Settings::HTTP_PROXY_USE_STUDIO, true).toBool();
    bool proxyAuth = settings.value(Ui::Settings::HTTP_PROXY_AUTHENTICATION, false).toBool();
    enum Ui::Settings::HTTP_PROXY_TYPE proxyType =
            (enum Ui::Settings::HTTP_PROXY_TYPE)settings.value(
                    Ui::Settings::HTTP_PROXY_TYPE,
                    Ui::Settings::HTTP_PROXY_TYPE_NONE).toInt();
    QString proxyHost = settings.value(Ui::Settings::HTTP_PROXY_HOST, "").toString();
    int proxyPort = settings.value(Ui::Settings::HTTP_PROXY_PORT, 80).toInt();
    QString proxyUsername = settings.value(Ui::Settings::HTTP_PROXY_USERNAME, "").toString();

    mUi->set_useStudio->setChecked(useStudio);
    mUi->set_noProxy->setChecked(proxyType == Ui::Settings::HTTP_PROXY_TYPE_NONE);
    mUi->set_manualConfig->setChecked(proxyType == Ui::Settings::HTTP_PROXY_TYPE_MANUAL);
    mUi->set_hostName->setText(proxyHost);
    mUi->set_portNumber->setValue(proxyPort);
    mUi->set_proxyAuth->setChecked(proxyAuth);
    mUi->set_loginName->setText(proxyUsername);
    mUi->set_loginPassword->setText(""); // Password is not saved

    grayOutProxy();
}

void SettingsPage::on_set_hostName_editingFinished() {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_HOST, mUi->set_hostName->text());
}

void SettingsPage::on_set_loginName_editingFinished() {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_USERNAME, mUi->set_loginName->text());
}

void SettingsPage::on_set_loginPassword_editingFinished() {
    // Password is not stored
}
void SettingsPage::on_set_manualConfig_toggled(bool checked) {
    if (checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::HTTP_PROXY_TYPE, Ui::Settings::HTTP_PROXY_TYPE_MANUAL);
        grayOutProxy();
    }
}
void SettingsPage::on_set_noProxy_toggled(bool checked) {
    if (checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::HTTP_PROXY_TYPE, Ui::Settings::HTTP_PROXY_TYPE_NONE);
        grayOutProxy();
    }
}
void SettingsPage::on_set_portNumber_valueChanged(int value) {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_PORT, value);
}
void SettingsPage::on_set_proxyAuth_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_AUTHENTICATION, checked);
    grayOutProxy();
}
void SettingsPage::on_set_useStudio_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_USE_STUDIO, checked);
    grayOutProxy();
}

// Conditionally gray out portions of the
// HTTP proxy page
void SettingsPage::grayOutProxy() {
    // Start off assuming all are grayed out
    bool radioEnabled = false;
    bool manualFieldsEnabled = false;
    bool loginEnabled = false;

    if (!mUi->set_useStudio->isChecked()) {
        // Some are not grayed out
        radioEnabled = true;
        if (mUi->set_manualConfig->isChecked()) {
            manualFieldsEnabled = true;
            if (mUi->set_proxyAuth->isChecked()) {
                loginEnabled = true;
            }
        }
    }

    mUi->set_noProxy->setEnabled(radioEnabled);
    mUi->set_manualConfig->setEnabled(radioEnabled);


    mUi->set_hostName->setEnabled(manualFieldsEnabled);
    mUi->set_portNumber->setEnabled(manualFieldsEnabled);
    mUi->set_proxyAuth->setEnabled(manualFieldsEnabled);

    mUi->set_loginName->setEnabled(loginEnabled);
    mUi->set_loginPassword->setEnabled(loginEnabled);
}

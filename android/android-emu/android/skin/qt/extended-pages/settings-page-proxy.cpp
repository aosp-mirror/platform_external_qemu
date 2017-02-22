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


void SettingsPage::on_set_useStudio_toggled(bool checked) {
    grayOutProxy();
}
void SettingsPage::on_set_noProxy_toggled(bool checked) {
    grayOutProxy();
}
void SettingsPage::on_set_autoDetect_toggled(bool checked) {
    grayOutProxy();
}
void SettingsPage::on_set_autoProxy_toggled(bool checked) {
    grayOutProxy();
}
void SettingsPage::on_set_manualConfig_toggled(bool checked) {
    grayOutProxy();
}
void SettingsPage::on_set_proxyAuth_toggled(bool checked) {
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

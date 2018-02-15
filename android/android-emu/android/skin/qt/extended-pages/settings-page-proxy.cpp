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

#include "android/emulation/control/http_proxy_agent.h"
#include "android/emulation/VmLock.h"
#include "android/emulator-window.h"
#include "android/proxy/proxy_common.h"
#include "android/proxy/proxy_errno.h"
#include "android/skin/qt/qt-settings.h"

#include <QFile>
#include <QSettings>

static QString sHttpProxyResults;
static QString sStudioProxyString;
static const QAndroidHttpProxyAgent* sHttpProxyAgent = nullptr;

static void getStudioProxyString();
static void sendProxySettingsToAgent(QString password);
static QString proxyStringFromParts(QString hostName,
                                    int     port,
                                    QString userName,
                                    QString password);

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
    mUi->set_proxyResults->setText(sHttpProxyResults);

    disableProxyApply();

    grayOutProxy();
}

void SettingsPage::proxyDtor() {
    // Clean up.
    // Try to delete the parameters file.
    // (If we were not given an agent, we didn't try
    // to read it and we didn't delete it.)

    EmulatorWindow* const ew = emulator_window_get();
    if (ew && ew->opts && ew->opts->studio_params) {
        QFile paramFile(ew->opts->studio_params);
        (void)paramFile.remove();
    }
}

// static
void SettingsPage::setHttpProxyAgent(const QAndroidHttpProxyAgent* agent) {
    {
        android::RecursiveScopedVmLock vmlock;
        sHttpProxyAgent = agent;
    }
    getStudioProxyString();
    sendProxySettingsToAgent("");
}

void SettingsPage::on_set_proxyApply_clicked() {
    disableProxyApply();
    sendProxySettingsToAgent(mUi->set_loginPassword->text());
    mUi->set_proxyResults->setText(sHttpProxyResults);
}

void SettingsPage::on_set_hostName_textChanged(QString /* unused */) {
    enableProxyApply();
}
void SettingsPage::on_set_hostName_editingFinished() {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_HOST, mUi->set_hostName->text());
}

void SettingsPage::on_set_loginName_textChanged(QString /* unused */) {
    enableProxyApply();
}
void SettingsPage::on_set_loginName_editingFinished() {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_USERNAME, mUi->set_loginName->text());
}

void SettingsPage::on_set_manualConfig_toggled(bool checked) {
    if (checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::HTTP_PROXY_TYPE, Ui::Settings::HTTP_PROXY_TYPE_MANUAL);
        grayOutProxy();
        enableProxyApply();
    }
}
void SettingsPage::on_set_noProxy_toggled(bool checked) {
    if (checked) {
        QSettings settings;
        settings.setValue(Ui::Settings::HTTP_PROXY_TYPE, Ui::Settings::HTTP_PROXY_TYPE_NONE);
        grayOutProxy();
        enableProxyApply();
    }
}
void SettingsPage::on_set_portNumber_valueChanged(int /* unused */) {
    enableProxyApply();
}
void SettingsPage::on_set_portNumber_editingFinished() {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_PORT,
                      mUi->set_portNumber->value());
}

void SettingsPage::on_set_proxyAuth_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_AUTHENTICATION, checked);
    grayOutProxy();
    enableProxyApply();
}
void SettingsPage::on_set_useStudio_toggled(bool checked) {
    QSettings settings;
    settings.setValue(Ui::Settings::HTTP_PROXY_USE_STUDIO, checked);
    grayOutProxy();
    enableProxyApply();
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

// Send the HTTP Proxy settings to the agent.
//
// Based on the command line, the UI, and Studio's settings,
// decide what to tell the agent.
//
// But only if there is an agent.
static void sendProxySettingsToAgent(QString password) {
    android::RecursiveScopedVmLock vmlock;
    if (sHttpProxyAgent == nullptr || sHttpProxyAgent->httpProxySet == nullptr) {
        return;
    }

    EmulatorWindow* const ew = emulator_window_get();
    if (ew && ew->opts && ew->opts->http_proxy) {
        // There is HTTP proxy information on the command line.
        // This takes precendence over the UI, and the back end
        // is already using these settings.
        sHttpProxyResults = SettingsPage::tr("From command line");
        return;
    }

    // There is no proxy info on the command line. Use the UI values.

    QString proxyString;

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

    if (useStudio) {
        // Use the settings from Android Studio
        proxyString = sStudioProxyString;
    } else if (proxyType == Ui::Settings::HTTP_PROXY_TYPE_NONE) {
        proxyString = "";
    } else if (proxyType == Ui::Settings::HTTP_PROXY_TYPE_MANUAL) {
        if (!proxyAuth) {
            // No authentication. Don't send user name or password.
            proxyUsername = "";
            password = "";
        }
        proxyString = proxyStringFromParts(proxyHost, proxyPort,
                                           proxyUsername, password);
    } else {
        // Should never get here. Default to "no proxy."
        proxyString = "";
    }
    // Send the proxy selection to the agent
    int proxyResultCode = sHttpProxyAgent->httpProxySet(proxyString.toStdString().c_str());

    // Make the results available to the GUI
    if (proxyString.isEmpty() && proxyResultCode == PROXY_ERR_OK) {
        // Don't say "Success" when we're not using a proxy
        sHttpProxyResults = SettingsPage::tr("No proxy");
    } else {
        sHttpProxyResults = SettingsPage::tr(proxy_error_string(proxyResultCode));
    }
}

void SettingsPage::enableProxyApply() {
    mUi->set_proxyApply->setEnabled(true);
}
void SettingsPage::disableProxyApply() {
    mUi->set_proxyApply->setEnabled(false);
}


// If Android Studio gave us a file with HTTP Proxy
// settings, read those into memory and delete the
// file.

static void getStudioProxyString() {
    EmulatorWindow* const ew = emulator_window_get();
    if (!ew || !ew->opts || !ew->opts->studio_params) {
        // No file to read
        return;
    }

    // Android Studio created a file for us
    QFile paramFile(ew->opts->studio_params);
    if (!paramFile.open(QFile::ReadOnly | QFile::Text)) {
        // Open failure
        // Try to clean up
        (void)paramFile.remove();
        ew->opts->studio_params = nullptr;
        return;
    }

    QString host;
    int     port = 0;
    QString username;
    QString password;

    while (!paramFile.atEnd()) {
        QByteArray line = paramFile.readLine().trimmed();
        int idx = line.indexOf("=");
        if (idx > 0) {
            QByteArray key = line.left(idx);
            QByteArray value = line.right(line.length() - 1 - idx);
            if (key == "http.proxyHost" ||
                key == "https.proxyHost") {
                host = value;
            } else if (key == "http.proxyPort" ||
                       key == "https.proxyPort") {
                port = value.toInt();
            } else if (key == "proxy.authentication.username") {
                username = value;
            } else if (key == "proxy.authentication.password") {
                password = value;
            }
        }
    }
    // Delete the file
    (void)paramFile.remove();
    ew->opts->studio_params = nullptr;

    sStudioProxyString = proxyStringFromParts(host, port, username, password);
}

// Construct a string like "myname:password@host.com:80"
static QString proxyStringFromParts(QString hostName,
                                    int     port,
                                    QString userName,
                                    QString password) {

    if (hostName.isEmpty()) {
        // Without a host name, we have nothing
        return "";
    }

    QString proxyString = "";
    if (!userName.isEmpty()) {
        proxyString += userName + ":" + password + "@";
    }

    proxyString += hostName + ":" + QString::number(port);
    return proxyString;
}

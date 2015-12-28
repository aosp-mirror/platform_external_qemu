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

#include "android/skin/qt/extended-pages/help-page.h"
#include "android/android.h"
#include "android/emulation/bufprint_config_dirs.h"
#include "android/globals.h"
#include "android/update-check/UpdateChecker.h"
#include "android/update-check/VersionExtractor.h"
#include <QDesktopServices>
#include <QThread>
#include <QUrl>

const QString DOCS_URL =
    "http://developer.android.com/tools/help/emulator.html";
const QString FILE_BUG_URL =
    "https://code.google.com/p/android/issues/entry?template=Android%20Emulator%20Bug";
const QString SEND_FEEDBACK_URL =
    "https://code.google.com/p/android/issues/entry?template=Emulator%20Feature%20Request";

static QString apiVersionString(int apiVersion)
{
    // This information was taken from the SDK Manager:
    // Appearances & Behavior > System Settings > Android SDK > SDK Platforms
    switch (apiVersion) {
        case 15: return "4.0.3 (Ice Cream Sandwich) - API 15 (Rev 5)";
        case 16: return "4.1 (Jelly Bean) - API 16 (Rev 5)";
        case 17: return "4.2 (Jelly Bean) - API 17 (Rev 3)";
        case 18: return "4.3 (Jelly Bean) - API 18 (Rev 3)";
        case 19: return "4.4 (KitKat) - API 19 (Rev 4)";
        case 20: return "4.4 (KitKat Wear) - API 20 (Rev 2)";
        case 21: return "5.0 (Lollipop) - API 21 (Rev 2)";
        case 22: return "5.1 (Lollipop) - API 22 (Rev 2)";
        case 23: return "6.0 (Marshmallow) - API 23 (Rev 1)";

        default:
            if (apiVersion < 0 || apiVersion > 99) {
                return qApp->tr("Unknown API version");
            } else {
                return "API " + QString::number(apiVersion);
            }
    }
}

HelpPage::HelpPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::HelpPage)
{
    mUi->setupUi(this);

    // Get the version of this code
    android::update_check::VersionExtractor vEx;

    android::base::Version curVersion = vEx.getCurrentVersion();
    auto verStr = curVersion.isValid()
            ? QString(curVersion.toString().c_str()) : "Unknown";

    mUi->help_versionBox->setPlainText(verStr);

    int apiLevel = avdInfo_getApiLevel(android_avdInfo);
    mUi->help_androidVersionBox->setPlainText(apiVersionString(apiLevel));

    // Show the ADB port number
    mUi->help_adbPortBox->setPlainText( QString::number(android_adb_port) );

    // launch the latest version loader in a separate thread
    auto latestVersionThread = new QThread();
    auto latestVersionTask = new LatestVersionLoadTask();
    latestVersionTask->moveToThread(latestVersionThread);
    connect(latestVersionThread, SIGNAL(started()), latestVersionTask, SLOT(run()));
    connect(latestVersionTask, SIGNAL(finished(QString)),
            mUi->help_latestVersionBox, SLOT(setPlainText(QString)));
    connect(latestVersionTask, SIGNAL(finished(QString)), latestVersionThread, SLOT(quit()));
    connect(latestVersionThread, SIGNAL(finished()), latestVersionTask, SLOT(deleteLater()));
    connect(latestVersionThread, SIGNAL(finished()), latestVersionThread, SLOT(deleteLater()));
    mUi->help_latestVersionBox->setPlainText(tr("Loading..."));
    latestVersionThread->start();
}

static void addShortcutsTableRow(QTableWidget* table_widget,
                                 const QString& key_sequence,
                                 const QString& description) {
    int table_row = table_widget->rowCount();
    table_widget->insertRow(table_row);
    table_widget->setItem(table_row, 0, new QTableWidgetItem(description));
    table_widget->setItem(table_row, 1, new QTableWidgetItem(key_sequence));
}

void HelpPage::initializeKeyboardShortcutList(const ShortcutKeyStore<QtUICommand>* key_store)
{
    QTableWidget* table_widget = mUi->shortcutsTableWidget;
    if (key_store) {
        for (auto key_sequence_and_command = key_store->begin();
             key_sequence_and_command != key_store->end();
             ++key_sequence_and_command) {
            addShortcutsTableRow(table_widget,
                                 key_sequence_and_command.key().toString(QKeySequence::NativeText),
                                 getQtUICommandDescription(key_sequence_and_command.value()));
        }
    }

    table_widget->sortItems(0);
}

void HelpPage::on_help_docs_clicked() {
    QDesktopServices::openUrl(QUrl(DOCS_URL));
}

void HelpPage::on_help_fileBug_clicked() {
    QDesktopServices::openUrl(QUrl(FILE_BUG_URL));
}

void HelpPage::on_help_sendFeedback_clicked() {
    QDesktopServices::openUrl(QUrl(SEND_FEEDBACK_URL));
}

void LatestVersionLoadTask::run() {
    // Get latest version that is available online
    char configPath[PATH_MAX];
    bufprint_config_path(configPath, configPath + sizeof(configPath));
    android::update_check::UpdateChecker upCheck(configPath);
    const auto latestVersion = upCheck.getLatestVersion();
    const auto latestVerStr = latestVersion.isValid()
            ? QString(latestVersion.toString().c_str()) : tr("Unavailable");
    emit finished(latestVerStr);
}

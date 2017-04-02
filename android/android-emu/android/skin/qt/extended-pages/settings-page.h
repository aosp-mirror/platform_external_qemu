// Copyright (C) 2015-2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/settings-agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "ui_settings-page.h"
#include <QString>
#include <QWidget>
#include <memory>

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = 0);

    void setAdbInterface(android::emulation::AdbInterface* adb);

signals:
    void frameAlwaysChanged(bool showFrame);
    void onForwardShortcutsToDeviceChanged(int index);
    void onTopChanged(bool isOnTop);
    void themeChanged(SettingsTheme new_theme);

private slots:
    void on_set_forwardShortcutsToDevice_currentIndexChanged(int index);
    void on_set_frameAlways_toggled(bool checked);
    void on_set_onTop_toggled(bool checked);
    void on_set_autoFindAdb_toggled(bool checked);
    void on_set_saveLocBox_textEdited(const QString&);
    void on_set_saveLocFolderButton_clicked();
    void on_set_adbPathBox_textEdited(const QString&);
    void on_set_adbPathButton_clicked();
    void on_set_themeBox_currentIndexChanged(int index);
    void on_set_crashReportPrefComboBox_currentIndexChanged(int index);
    void on_set_glesBackendPrefComboBox_currentIndexChanged(int index);
    void on_set_glesApiLevelPrefComboBox_currentIndexChanged(int index);

    void on_set_manualConfig_toggled(bool checked);
    void on_set_noProxy_toggled(bool checked);
    void on_set_proxyAuth_toggled(bool checked);
    void on_set_useStudio_toggled(bool checked);

private:
    bool eventFilter (QObject* object, QEvent* event) override;
    void grayOutProxy();

    android::emulation::AdbInterface* mAdb;
    std::unique_ptr<Ui::SettingsPage> mUi;
};

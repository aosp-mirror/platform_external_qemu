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

#include "ui_settings-page.h"

#include "android/settings-agent.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/perfstats-page.h"

#include <QString>
#include <QWidget>
#include <memory>

struct QAndroidHttpProxyAgent;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = 0);
    ~SettingsPage();

    void setAdbInterface(android::emulation::AdbInterface* adb);
    static void setHttpProxyAgent(const QAndroidHttpProxyAgent* agent);

public slots:
    void setHaveClipboardSharing(bool haveSharing);

signals:
    void frameAlwaysChanged(bool showFrame);
    void onForwardShortcutsToDeviceChanged(int index);
    void onTopChanged(bool isOnTop);
    void themeChanged(SettingsTheme new_theme);
    void enableClipboardSharingChanged(bool enabled);
    void disableMouseWheelChanged(bool disabled);

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
    void on_set_resetNotifications_pressed();
    void on_perfstatsButton_pressed();
#ifndef SNAPSHOT_CONTROLS  // TODO:jameskaye Remove when Snapshot controls are fully enabled
    void on_set_saveSnapNowButton_clicked();
    void on_set_loadSnapNowButton_clicked();
    void on_set_saveSnapshotOnExit_currentIndexChanged(int index);
#endif

    // HTTP Proxy
    void on_set_hostName_editingFinished();
    void on_set_hostName_textChanged(QString /* unused */);
    void on_set_loginName_editingFinished();
    void on_set_loginName_textChanged(QString /* unused */);
    void on_set_manualConfig_toggled(bool checked);
    void on_set_noProxy_toggled(bool checked);
    void on_set_portNumber_editingFinished();
    void on_set_portNumber_valueChanged(int /* unused */);
    void on_set_proxyApply_clicked();
    void on_set_proxyAuth_toggled(bool checked);
    void on_set_useStudio_toggled(bool checked);

    void on_set_clipboardSharing_toggled(bool checked);
    void on_set_disableMouseWheel_toggled(bool checked);

private:
    bool eventFilter(QObject* object, QEvent* event) override;

    void grayOutProxy();
    void disableProxyApply();
    void enableProxyApply();
    void initProxy();
    void proxyDtor();

    android::emulation::AdbInterface* mAdb;
    std::unique_ptr<Ui::SettingsPage> mUi;
    std::unique_ptr<PerfStatsPage> mPerfStatsPage;
    bool    mDisableANGLE = false;
};

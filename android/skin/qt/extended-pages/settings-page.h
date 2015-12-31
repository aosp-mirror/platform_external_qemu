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

#pragma once

#include "android/settings-agent.h"
#include "ui_settings-page.h"
#include <QString>
#include <QWidget>
#include <memory>

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = 0);

signals:
    void themeChanged(SettingsTheme new_theme);

private slots:
    void on_set_themeBox_currentIndexChanged(int index);
    void on_set_sdkPathButton_clicked();
    void on_set_saveLocFolderButton_clicked();
    void on_set_saveLocBox_textEdited(const QString&);
    void on_set_sdkPathBox_textEdited(const QString&);
    void on_set_allowKeyboardGrab_toggled(bool checked);

private:
    bool eventFilter (QObject* object, QEvent* event) override;

    std::unique_ptr<Ui::SettingsPage> mUi;
};

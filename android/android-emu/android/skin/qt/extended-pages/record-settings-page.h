// Copyright (C) 2019 The Android Open Source Project
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

#include <qobjectdefs.h>              // for Q_OBJECT, signals
#include <QString>                    // for QString
#include <QWidget>                    // for QWidget
#include <memory>                     // for unique_ptr

#include "ui_record-settings-page.h"  // for RecordSettingsPage

class QObject;
class QWidget;

class RecordSettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit RecordSettingsPage(QWidget *parent = 0);

signals:
    void on_toggleMacros_toggled(bool value);

private:
    std::unique_ptr<Ui::RecordSettingsPage> mUi;
};

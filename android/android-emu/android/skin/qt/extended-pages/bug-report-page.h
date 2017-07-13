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

#pragma once

#include "android/bugreport.h"
#include "android/skin/qt/extended-pages/common.h"

#include "ui_bug-report-page.h"

#include <QMessageBox>
#include <QWidget>

#include <memory>
#include <string>

struct QBugreportAgent;

class BugreportPage : public QWidget {
    Q_OBJECT

public:
    explicit BugreportPage(QWidget* parent = 0);
    void showEvent(QShowEvent* event);
    void updateTheme();
    void setBugreportAgent(const QBugreportAgent* agent);
    static void bugreportDataCallback(const BugreportData* data,
                                      BugreportDataType type,
                                      void* context);

private slots:
    void on_bug_saveButton_clicked();
    void on_bug_sendToGoogle_clicked();
private:
    bool eventFilter(QObject* object, QEvent* event) override;
    void saveReport();
    void onReceiveAdbLogcat(const BugreportData* data);
    void onReceiveAdbBugreport(const BugreportData* data);
    void onReceiveScreenshot(const BugreportData* data);
    void onReceiveAvdDetails(const BugreportData* data);
    const QBugreportAgent* mBugreportAgent;
    QMessageBox mDeviceDetailsDialog;
    bool mBugreportSavedLocally = false;
    std::unique_ptr<Ui::BugreportPage> mUi;
    SettingsTheme mTheme;

    std::string mSaveDirectory;
};

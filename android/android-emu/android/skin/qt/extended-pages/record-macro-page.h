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

#include "ui_record-macro-page.h"

#include <QWidget>
#include <memory>

struct QAndroidAutomationAgent;

class RecordMacroPage : public QWidget {
    Q_OBJECT

public:
    explicit RecordMacroPage(QWidget* parent = 0);

    static void setAutomationAgent(const QAndroidAutomationAgent* agent);

private slots:
    void on_playButton_clicked();
    void on_macroList_itemClicked(QListWidgetItem* item);

private:
    void loadUi();
    std::string getMacrosDirectory();

    std::unique_ptr<Ui::RecordMacroPage> mUi;

    static const QAndroidAutomationAgent* sAutomationAgent;
};

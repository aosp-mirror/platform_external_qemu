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

#include "ui_record-macro-saved-item.h"

#include <QWidget>
#include <memory>

namespace Ui {
class RecordMacroSavedItem;
}

class RecordMacroSavedItem : public QWidget {
    Q_OBJECT

public:
    explicit RecordMacroSavedItem(QWidget* parent = 0);

    void setName(QString name);
    std::string getName() const;
    void setDisplayInfo(QString displayInfo);
    void setDisplayTime(QString displayTime);
    void macroSelected(bool state);
    bool getIsPreset();
    void setIsPreset(bool isPreset);
    void editEnabled(bool enable);

private slots:
    void on_editButton_clicked();

signals:
    void editButtonClickedSignal(RecordMacroSavedItem* macroItem);

private:
    void setDisplayInfoOpacity(double opacity);
    void loadUi();

    bool mIsPreset = false;
    std::unique_ptr<Ui::RecordMacroSavedItem> mUi;
};

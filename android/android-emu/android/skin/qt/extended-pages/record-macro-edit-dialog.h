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

#include <qobjectdefs.h>                  // for Q_OBJECT, slots
#include <QDialog>                        // for QDialog
#include <QString>                        // for QString
#include <memory>                         // for unique_ptr

#include "ui_record-macro-edit-dialog.h"  // for RecordMacroEditDialog

class QObject;
class QString;
class QWidget;

enum class ButtonClicked : int { Delete = 2 };

class RecordMacroEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit RecordMacroEditDialog(QWidget *parent = 0);

    void setCurrentName(QString name);
    QString getNewName();

private slots:
    void on_deleteButton_clicked();

private:
    std::unique_ptr<Ui::RecordMacroEditDialog> mUi;
};

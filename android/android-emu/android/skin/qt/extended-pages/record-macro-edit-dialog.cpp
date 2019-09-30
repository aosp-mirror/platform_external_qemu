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

#include "record-macro-edit-dialog.h"

#include <QLineEdit>                                // for QLineEdit
#include <QPushButton>                              // for QPushButton

#include "android/skin/qt/extended-pages/common.h"  // for getIconForCurrent...

class QWidget;

RecordMacroEditDialog::RecordMacroEditDialog(QWidget *parent) :
    QDialog(parent),
    mUi(new Ui::RecordMacroEditDialog()) {
    mUi->setupUi(this);

    connect(mUi->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mUi->saveButton, SIGNAL(clicked()), this, SLOT(accept()));

    mUi->deleteButton->setIcon(getIconForCurrentTheme("trash"));
}

void RecordMacroEditDialog::setCurrentName(QString name) {
    mUi->name->setText(name);
    mUi->name->selectAll();
}

QString RecordMacroEditDialog::getNewName() { 
    return mUi->name->text();
}

void RecordMacroEditDialog::on_deleteButton_clicked() {
    QDialog::done((int)ButtonClicked::Delete);
}

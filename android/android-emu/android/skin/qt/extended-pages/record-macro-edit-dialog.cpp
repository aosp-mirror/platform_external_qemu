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

#include "android/skin/qt/extended-pages/common.h"

#include <QSignalMapper>

RecordMacroEditDialog::RecordMacroEditDialog(QWidget *parent) :
    QDialog(parent),
    mUi(new Ui::RecordMacroEditDialog()) {
    mUi->setupUi(this);

    QSignalMapper* signalMapper = new QSignalMapper(this);
    signalMapper->setMapping(mUi->deleteButton, ButtonClicked::Delete);

    connect(mUi->cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(mUi->saveButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(done(int)));
    connect(mUi->deleteButton, SIGNAL(clicked()), signalMapper, SLOT(map()));

    mUi->deleteButton->setIcon(getIconForCurrentTheme("trash"));
}

void RecordMacroEditDialog::setCurrentName(QString name) {
    mUi->name->setText(name);
    mUi->name->selectAll();
}

QString RecordMacroEditDialog::getNewName() { 
    return mUi->name->text();
}
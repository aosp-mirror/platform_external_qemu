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

#include "record-macro-saved-item.h"

RecordMacroSavedItem::RecordMacroSavedItem(QWidget *parent)
    : QWidget(parent), mUi(new Ui::RecordMacroSavedItem()) {
    mUi->setupUi(this);
}

void RecordMacroSavedItem::setName(std::string name) {
    mUi->name->setText(QString(name.c_str()));
}

std::string RecordMacroSavedItem::getName() {
    return mUi->name->text().toUtf8().constData();
}

void RecordMacroSavedItem::setDisplayInfo(std::string displayInfo) {
    mUi->displayInfo->setText(QString(displayInfo.c_str()));
}

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
#include "vhal-item.h"

#include "ui_vhal-item.h"

VhalItem::VhalItem(QWidget* parent, QString name, QString proID)
    : QWidget(parent), ui(new Ui::VhalItem) {
    ui->setupUi(this);
    ui->name->setText(name);
    ui->proid->setText(proID);
}

void VhalItem::vhalItemSelected(bool state) {
    if (state) {
        ui->name->setStyleSheet("color: white");
        ui->proid->setStyleSheet("color: white");
    } else {
        ui->name->setStyleSheet("");
        ui->proid->setStyleSheet("");
    }
}

VhalItem::~VhalItem() {
    delete ui;
}

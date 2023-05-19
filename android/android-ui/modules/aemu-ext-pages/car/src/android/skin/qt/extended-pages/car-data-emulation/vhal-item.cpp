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

#include <QLabel>          // for QLabel

#include "ui_vhal-item.h"  // for VhalItem

class QWidget;

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

void VhalItem::setValues(int property_id, int area_id, QString key) {
    this->property_id = property_id;
    this->area_id = area_id;
    this->key = key;
}

int VhalItem::getPropertyId() {
    return this->property_id;
}
int VhalItem::getAreaId() {
    return this->area_id;
}
QString VhalItem::getKey() {
    return this->key;
}

// Copyright (C) 2020 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#include "posture-selection-dialog.h"
#include "ui_postureselectiondialog.h"

#include "android/hw-sensors.h"

PostureSelectionDialog::PostureSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PostureSelectionDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Popup);
}

void PostureSelectionDialog::on_btn_closed_clicked() {
    emit newPostureRequested(1);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_flipped_clicked() {
    emit newPostureRequested(4);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_halfopen_clicked() {
    emit newPostureRequested(2);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_open_clicked() {
    emit newPostureRequested(3);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_tent_clicked() {
    accept();   // hides dialog
}

PostureSelectionDialog::~PostureSelectionDialog()
{
    delete ui;
}

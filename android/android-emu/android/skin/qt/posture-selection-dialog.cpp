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
    emit newPostureRequested(POSTURE_CLOSED);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_flipped_clicked() {
    emit newPostureRequested(POSTURE_FLIPPED);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_halfopen_clicked() {
    emit newPostureRequested(POSTURE_HALF_OPENED);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_open_clicked() {
    emit newPostureRequested(POSTURE_OPENED);
    accept();   // hides dialog
}

void PostureSelectionDialog::on_btn_tent_clicked() {
    emit newPostureRequested(POSTURE_TENT);
    accept();   // hides dialog
}

void PostureSelectionDialog::showEvent(QShowEvent* event) {
    QDialog::showEvent(event);
    if (!mShown) {
        hideUnsupportedPostureButtons();
    }
    mShown = true;
}

void PostureSelectionDialog::hideUnsupportedPostureButtons() {
    struct FoldableState foldableState;
    android_foldable_get_state(&foldableState);
    bool *supportedFoldablePostures = foldableState.config.supportedFoldablePostures;

    if (!supportedFoldablePostures[POSTURE_OPENED]) {
        ui->btn_open->setHidden(true);
    }
    if (!supportedFoldablePostures[POSTURE_CLOSED]) {
        ui->btn_closed->setHidden(true);
    }
    if (!supportedFoldablePostures[POSTURE_FLIPPED]) {
        ui->btn_flipped->setHidden(true);
    }
    if (!supportedFoldablePostures[POSTURE_HALF_OPENED]) {
        ui->btn_halfopen->setHidden(true);
    }
    if (!supportedFoldablePostures[POSTURE_TENT]) {
        ui->btn_tent->setHidden(true);
    }
}

PostureSelectionDialog::~PostureSelectionDialog()
{
    delete ui;
}

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

#include "android/skin/qt/extended-pages/common.h"

#include <QGraphicsOpacityEffect>

const double kDisplayInfoOpacity = 0.66f;

RecordMacroSavedItem::RecordMacroSavedItem(QWidget* parent)
    : QWidget(parent), mUi(new Ui::RecordMacroSavedItem()) {
    mUi->setupUi(this);

    loadUi();
}

void RecordMacroSavedItem::setName(QString name) {
    mUi->name->setText(name);
}

std::string RecordMacroSavedItem::getName() const {
    return mUi->name->text().toUtf8().constData();
}

void RecordMacroSavedItem::setDisplayInfo(QString displayInfo) {
    mUi->displayInfo->setText(displayInfo);
}

void RecordMacroSavedItem::setDisplayTime(QString displayTime) {
    mUi->displayTime->setText(displayTime);
}

void RecordMacroSavedItem::macroSelected(bool state) {
    if (state) {
        mUi->name->setStyleSheet("color: white");
        mUi->displayInfo->setStyleSheet("color: white");
        mUi->displayTime->setStyleSheet("color: white");
    } else {
        mUi->name->setStyleSheet("");
        mUi->displayInfo->setStyleSheet("");
        mUi->displayTime->setStyleSheet("");
    }
}

void RecordMacroSavedItem::setDisplayInfoOpacity(double opacity) {
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(mUi->displayInfo);
    effect->setOpacity(opacity);
    mUi->displayInfo->setGraphicsEffect(effect);
}

bool RecordMacroSavedItem::getIsPreset() {
    return mIsPreset;
}

void RecordMacroSavedItem::setIsPreset(bool isPreset) {
    mIsPreset = isPreset;
}

void RecordMacroSavedItem::loadUi() {
    setDisplayInfoOpacity(kDisplayInfoOpacity);

    mUi->editButton->setIcon(getIconForCurrentTheme("edit"));
}

void RecordMacroSavedItem::on_editButton_clicked() {
    emit editButtonClickedSignal(this);
}

void RecordMacroSavedItem::editEnabled(bool enable) {
    if (enable) {
        mUi->editButton->show();
    } else {
        mUi->editButton->hide();
    }
}

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

#include "cc-list-item.h"

#include "android/skin/qt/extended-pages/common.h"

#include <QGraphicsOpacityEffect>

const double kDisplayInfoOpacity = 0.66f;
const int kNameLabelWidth = 200;

CCListItem::CCListItem(QWidget* parent)
    : QWidget(parent), mUi(new Ui::CCListItem()) {
    mUi->setupUi(this);

    setSubtitleOpacity(kDisplayInfoOpacity);
    mUi->editButton->setIcon(getIconForCurrentTheme("more_vert"));
}

void CCListItem::setTitle(QString title) {
    QFontMetrics metrix(mUi->title->font());
    QString clippedTitle = 
            metrix.elidedText(title, Qt::ElideRight, kNameLabelWidth);
    mUi->title->setText(clippedTitle);
}

std::string CCListItem::getTitle() const {
    return mUi->title->text().toUtf8().constData();
}

void CCListItem::setSubtitle(QString subtitle) {
    mUi->subtitle->setText(subtitle);
}

void CCListItem::setExtraLabelText(QString extraText) {
    mUi->extraLabel->setText(extraText);
}

void CCListItem::setExtraLabelPixmap(QPixmap pixmap) {
    mUi->extraLabel->setPixmap(pixmap);
}

void CCListItem::setSubtitleOpacity(double opacity) {
    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(mUi->subtitle);
    effect->setOpacity(opacity);
    mUi->subtitle->setGraphicsEffect(effect);
}

void CCListItem::on_editButton_clicked() {
    emit editButtonClickedSignal(this);
}

void CCListItem::setEditButtonEnabled(bool enable) {
    if (enable) {
        mUi->editButton->show();
    } else {
        mUi->editButton->hide();
    }
}

void CCListItem::setSelected(bool selected) {
    if (selected) {
        mUi->title->setStyleSheet("color: white");
        mUi->subtitle->setStyleSheet("color: white");
        mUi->extraLabel->setStyleSheet("color: white");
    } else {
        mUi->title->setStyleSheet("");
        mUi->subtitle->setStyleSheet("");
        mUi->extraLabel->setStyleSheet("");
    }
}

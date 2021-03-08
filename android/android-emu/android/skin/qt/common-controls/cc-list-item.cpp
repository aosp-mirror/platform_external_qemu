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

#include <qnamespace.h>                             // for ElideRight
#include <QByteArray>                               // for QByteArray
#include <QFontMetrics>                             // for QFontMetrics
#include <QGraphicsOpacityEffect>                   // for QGraphicsOpacityE...
#include <QLabel>                                   // for QLabel
#include <QPixmap>                                  // for QPixmap
#include <QPushButton>                              // for QPushButton

#include "android/settings-agent.h"                 // for SETTINGS_THEME_DARK
#include "android/skin/qt/extended-pages/common.h"  // for getIconForCurrent...

class QGraphicsOpacityEffect;
class QWidget;

const double kDisplayInfoOpacity = 0.66f;
const int kNameLabelWidth = 170;

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

void CCListItem::setLabelText(QString extraText, CCLayoutDirection direction) {
    switch (direction) {
    case CCLayoutDirection::Left:
        mUi->leftLabel->setText(extraText);
        break;
    case CCLayoutDirection::Right:
        mUi->rightLabel->setText(extraText);
        break;
    }
}

void CCListItem::setLabelPixmap(QPixmap pixmap, CCLayoutDirection direction) {
    switch (direction) {
    case CCLayoutDirection::Left:
        mUi->leftLabel->setPixmap(pixmap);
        break;
    case CCLayoutDirection::Right:
        mUi->rightLabel->setPixmap(pixmap);
        break;
    }
}

void CCListItem::setLabelSize(int width, int height, CCLayoutDirection direction) {
    switch (direction) {
    case CCLayoutDirection::Left:
        mUi->leftLabel->setFixedSize(width, height);
        break;
    case CCLayoutDirection::Right:
        mUi->rightLabel->setFixedSize(width, height);
        break;
    }
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
    mSelected = selected;
    if (selected) {
        mUi->title->setStyleSheet("color: white");
        mUi->subtitle->setStyleSheet("color: white");
        mUi->leftLabel->setStyleSheet("color: white");
        mUi->rightLabel->setStyleSheet("color: white");
    } else {
        mUi->title->setStyleSheet("");
        mUi->subtitle->setStyleSheet("");
        mUi->leftLabel->setStyleSheet("");
        mUi->rightLabel->setStyleSheet("");
    }
    updateTheme();
}

void CCListItem::updateTheme() {
    if (mSelected) {
        mUi->editButton->setIcon(getIconForTheme("more_vert", SettingsTheme::SETTINGS_THEME_DARK));
    } else {
        mUi->editButton->setIcon(getIconForCurrentTheme("more_vert"));
    }
}

bool CCListItem::isSelected() const {
    return mSelected;
}

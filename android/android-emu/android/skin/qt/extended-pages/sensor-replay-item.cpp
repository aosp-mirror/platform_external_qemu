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

#include "sensor-replay-item.h"

class QWidget;

SensorReplayItem::SensorReplayItem(QListWidget* const listWidget, const QString& name, const QString& path, QWidget* parent)
    : CCListItem(parent), mListWidget(listWidget) {

    mListItem = new QListWidgetItem(listWidget);
    mListItem->setSizeHint(QSize(0, 50));

    setTitle(name);
    setSubtitle(path);

    listWidget->addItem(mListItem);
    listWidget->setItemWidget(mListItem, this);
    setEditButtonEnabled(true);
}

std::string SensorReplayItem::getName() const {
    return getTitle();
}

void SensorReplayItem::removeFromListWidget() {
    auto* item = mListWidget->takeItem(mListWidget->row(mListItem));
    if (item != nullptr) {
        delete item;
        mListItem = nullptr;
    }
}

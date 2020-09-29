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
#include "android/utils/debug.h"

class QWidget;

#define D(...) VERBOSE_PRINT(car, __VA_ARGS__)


SensorReplayItem::SensorReplayItem(QListWidget* const listWidget, const QString& name, const QString& path, QWidget* parent)
    : CCListItem(parent), mListWidget(listWidget) {

    D("Add an Item to the list");
    mListItem = new QListWidgetItem(listWidget);
    mListItem->setSizeHint(QSize(0, 50));

    D("get elided name");
    QString elidedFileName = QString(name);
    if(elidedFileName.size() > 15) {
        elidedFileName.resize(15);
        elidedFileName.append("...");
    }

    D("Add an Item to the list");
    setTitle(elidedFileName);
    setSubtitle(path);

    D("add item to the list");
    listWidget->addItem(mListItem);
    D("setitem widget");
    listWidget->setItemWidget(mListItem, this);
    D("set current item");
    listWidget->setCurrentItem(mListItem);

    D("setedit button ");
    setEditButtonEnabled(true);
    mFileName = QString(name);
    mPath = QString(path);

}

void SensorReplayItem::setSensorSessionPlayback(SensorSessionPlayback* sensorSessionPlayback) {
    // Set playback session
    mSensorSessionPlayback = sensorSessionPlayback;
}

QString SensorReplayItem::getPath() const {
    return mPath;
}

SensorSessionPlayback* SensorReplayItem::getSensorSessionPlayback() {
    return mSensorSessionPlayback;
}

void SensorReplayItem::removeFromListWidget() {
    auto* item = mListWidget->takeItem(mListWidget->row(mListItem));
    if (item != nullptr) {
        delete item;
        mListItem = nullptr;
    }
}

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

#include "sensor-replay-item.h"

class QWidget;

using android::sensorsessionplayback::SensorSessionPlayback;

static constexpr int32_t FILENAME_MAX_WIDTH = 15;

SensorReplayItem::SensorReplayItem(
        QListWidget* const listWidget,
        const QString& name,
        const QString& path,
        android::sensorsessionplayback::SensorSessionPlayback*
                sensorSessionPlayback,
        QWidget* parent)
    : CCListItem(parent), mListWidget(listWidget) {
    mListItem = new QListWidgetItem(listWidget);
    mListItem->setSizeHint(QSize(0, 50));

    // Formatting filename
    QString elidedFileName = QString(name);
    if (elidedFileName.size() > FILENAME_MAX_WIDTH) {
        elidedFileName.resize(FILENAME_MAX_WIDTH);
        elidedFileName.append("...");
    }

    setTitle(elidedFileName);
    setSubtitle(path);

    listWidget->addItem(mListItem);
    listWidget->setItemWidget(mListItem, this);
    listWidget->setCurrentItem(mListItem);

    setEditButtonEnabled(true);
    mPath = QString(path);

    mSensorSessionPlayback = sensorSessionPlayback;
}

SensorReplayItem::~SensorReplayItem() {
    delete mSensorSessionPlayback;
    mSensorSessionPlayback = nullptr;
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

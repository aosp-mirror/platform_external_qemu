// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/image-well.h"

#include "android/metrics/proto/studio_stats.pb.h"

#include <QFileDialog>

ImageWell::ImageWell(QWidget* parent)
    : QWidget(parent), mUi(new Ui::ImageWell()) {
    mUi->setupUi(this);
}

void ImageWell::setPath(QString path) {
    // Ignore the return value and don't emit pathChanged.
    (void)setPathInternal(path);
}

void ImageWell::on_filePicker_clicked() {
    // Use dialog to get file name
    QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Image File"), ".",
            tr("Image files (*.png *.jpg *.jpeg)"));

    emit interaction();

    // Keep the previous image if the file picker dialog is canceled.
    if (fileName.isNull()) {
        return;
    }

    if (setPathInternal(fileName)) {
        emit pathChanged(mPath);
    }
}

bool ImageWell::setPathInternal(QString path) {
    if (mPath != path) {
        mPath = path;

        if (path.isNull()) {
            mUi->image->clear();
        } else {
            QPixmap image(mPath);
            const int height = mUi->image->height();
            const int width = mUi->image->width();
            mUi->image->setPixmap(
                    image.scaled(width, height, Qt::KeepAspectRatio));
        }

        return true;
    } else {
        return false;
    }
}

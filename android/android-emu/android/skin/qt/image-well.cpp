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

#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QMimeData>

ImageWell::ImageWell(QWidget* parent)
    : QWidget(parent), mUi(new Ui::ImageWell()) {
    mUi->setupUi(this);
    setAcceptDrops(true);
}

void ImageWell::setPath(QString path) {
    // Ignore the return value and don't emit pathChanged.
    (void)setPathInternal(path);
}

void ImageWell::setStartingDirectory(QString startingDirectory) {
    mStartingDirectory = startingDirectory;
}

void ImageWell::dragEnterEvent(QDragEnterEvent* event) {
    if (getPathIfValidDrop(event->mimeData()) != QString()) {
        event->acceptProposedAction();
    }
}

void ImageWell::dropEvent(QDropEvent* event) {
    // Modal dialogs don't prevent drag-and-drop! Manually check for a modal
    // dialog, and if so, reject the event.
    if (QApplication::activeModalWidget() != nullptr) {
        event->ignore();
        return;
    }

    QString path = getPathIfValidDrop(event->mimeData());
    if (path.isEmpty()) {
        event->ignore();
        return;
    }

    if (setPathInternal(path)) {
        emit pathChanged(mPath);
    }
}

void ImageWell::on_filePicker_clicked() {
    // Use dialog to get file name.
    QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open Image"), mStartingDirectory,
            tr("Images (*.png *.jpg *.jpeg)"));

    emit interaction();

    // Keep the previous image if the file picker dialog is canceled.
    if (fileName.isEmpty()) {
        return;
    }

    if (setPathInternal(fileName)) {
        emit pathChanged(mPath);
    }
}

bool ImageWell::setPathInternal(const QString& path) {
    if (mPath == path) {
        return false;  // No change.
    }

    if (path.isNull()) {
        mUi->image->clear();
        mPath = path;
        return true;
    }

    QPixmap image(path);
    if (image.isNull()) {
        // Failed to load, reset back to an empty path.
        qWarning() << "Image could not be loaded: " << path;
        mUi->image->clear();
        const bool pathChanged = !mPath.isNull();
        mPath = QString();

        return pathChanged;
    }

    mPath = path;
    const int height = mUi->image->height();
    const int width = mUi->image->width();
    mUi->image->setPixmap(image.scaled(width, height, Qt::KeepAspectRatio));
    return true;
}

QString ImageWell::getPathIfValidDrop(const QMimeData* mimeData) const {
    // Require exactly one URL to a local file.
    if (mimeData && mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        if (urls.length() == 1 && urls[0].isLocalFile()) {
            QFileInfo info(urls[0].toLocalFile());
            QString extension = info.suffix();

            // Support PNG and JPEG files.
            if (extension == "png" || extension == "jpg" ||
                extension == "jpeg") {
                return info.absoluteFilePath();
            }
        }
    }

    return QString();
}

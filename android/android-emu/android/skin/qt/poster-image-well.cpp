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

#include "android/skin/qt/poster-image-well.h"

#include <qloggingcategory.h>                        // for qCWarning
#include <qnamespace.h>                              // for KeepAspectRatio
#include <qstring.h>                                 // for operator!=, oper...
#include <QApplication>                              // for QApplication
#include <QDebug>                                    // for QDebug
#include <QDragEnterEvent>                           // for QDragEnterEvent
#include <QDropEvent>                                // for QDropEvent
#include <QFileDialog>                               // for QFileDialog
#include <QFileInfo>                                 // for QFileInfo
#include <QLabel>                                    // for QLabel
#include <QLineEdit>                                 // for QLineEdit
#include <QList>                                     // for QList
#include <QMimeData>                                 // for QMimeData
#include <QPixmap>                                   // for QPixmap
#include <QStackedWidget>                            // for QStackedWidget
#include <QUrl>                                      // for QUrl

#include "android/skin/qt/editable-slider-widget.h"  // for EditableSliderWi...
#include "android/skin/qt/logging-category.h"        // for emu

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMimeData;
class QUrl;
class QWidget;

static constexpr int kPageNoImage = 0;
static constexpr int kPageImage = 1;

PosterImageWell::PosterImageWell(QWidget* parent)
    : QWidget(parent), mUi(new Ui::PosterImageWell()), mOverlayWidget(this) {
    mUi->setupUi(this);
    setAcceptDrops(true);
    mOverlayWidget.setObjectName("DropTarget");
    mOverlayWidget.hide();
    mOverlayWidget.setAttribute(Qt::WA_TransparentForMouseEvents);

    mUi->stackedWidget->setCurrentIndex(kPageNoImage);
    mUi->sizeSlider->setRange(0.0, mSliderValueScale, false);
    mUi->sizeSlider->setValue(mSliderValueScale, false);
}

void PosterImageWell::setPath(QString path) {
    // Ignore the return value and don't emit pathChanged.
    (void)setPathInternal(path);
}

float PosterImageWell::getScale() const {
    return static_cast<float>(mUi->sizeSlider->getValue()) / mSliderValueScale;
}

void PosterImageWell::setScale(float value) {
    mUi->sizeSlider->setValue(value * mSliderValueScale, false);
}

void PosterImageWell::setMinMaxSize(float minSize, float maxSize) {
    const float previousScale = getScale();

    mSliderValueScale = maxSize;
    mUi->sizeSlider->setRange(minSize, mSliderValueScale, false);
    setScale(previousScale);
}

void PosterImageWell::setStartingDirectory(QString startingDirectory) {
    mStartingDirectory = startingDirectory;
}

void PosterImageWell::dragEnterEvent(QDragEnterEvent* event) {
    if (getPathIfValidDrop(event->mimeData()) != QString()) {
        mOverlayWidget.setGeometry(rect());
        mOverlayWidget.show();
        mOverlayWidget.raise();

        event->acceptProposedAction();
        update();
    }
}

void PosterImageWell::dragLeaveEvent(QDragLeaveEvent* event) {
    mOverlayWidget.hide();
}

void PosterImageWell::dropEvent(QDropEvent* event) {
    mOverlayWidget.hide();

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

void PosterImageWell::on_filePicker_clicked() {
    openFilePicker();
}

void PosterImageWell::on_pickFileButton_clicked() {
    openFilePicker();
}

void PosterImageWell::on_removeButton_clicked() {
    emit interaction();

    if (setPathInternal(QString())) {
        emit pathChanged(mPath);
    }
}

void PosterImageWell::on_sizeSlider_valueChanged(double value) {
    const float scaledValue = static_cast<float>(value) / mSliderValueScale;
    emit interaction();
    emit scaleChanged(scaledValue);
}

void PosterImageWell::openFilePicker() {
    // Use dialog to get file name.
    QString fileName = QFileDialog::getOpenFileName(
            this, tr("Open image"), mStartingDirectory,
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

void PosterImageWell::removeImage() {
    mUi->stackedWidget->setCurrentIndex(kPageNoImage);

    mUi->image->setText(tr("No image applied"));
    mUi->fileName->clear();
    mPath = QString();
}

bool PosterImageWell::setPathInternal(const QString& path) {
    if (mPath == path) {
        return false;  // No change.
    }

    if (path.isNull()) {
        removeImage();
        return true;
    }

    QPixmap image(path);
    if (image.isNull()) {
        // Failed to load, reset back to an empty path.
        qCWarning(emu) << tr("Can't load image: ") << path;

        const bool pathChanged = !mPath.isNull();
        removeImage();
        return pathChanged;
    }

    mUi->stackedWidget->setCurrentIndex(kPageImage);

    mPath = path;
    const int height = mUi->image->height();
    const int width = mUi->image->width();
    mUi->image->setPixmap(image.scaled(width, height, Qt::KeepAspectRatio));
    mUi->fileName->setText(QFileInfo(path).fileName());
    return true;
}

QString PosterImageWell::getPathIfValidDrop(const QMimeData* mimeData) const {
    // Require exactly one URL to a local file.
    if (mimeData && mimeData->hasUrls()) {
        QList<QUrl> urls = mimeData->urls();
        if (urls.length() == 1 && urls[0].isLocalFile()) {
            QFileInfo info(urls[0].toLocalFile());
            QString extension = info.suffix().toLower();

            // Support PNG and JPEG files.
            if (extension == "png" || extension == "jpg" ||
                extension == "jpeg") {
                return info.absoluteFilePath();
            }
        }
    }

    return QString();
}

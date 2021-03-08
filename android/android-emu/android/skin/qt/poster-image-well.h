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
#pragma once

#include <qobjectdefs.h>           // for Q_PROPERTY, Q_OBJECT, signals, slots
#include <QString>                 // for QString
#include <QWidget>                 // for QWidget
#include <memory>                  // for unique_ptr

#include "ui_poster-image-well.h"  // for PosterImageWell

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QMimeData;
class QObject;
class QString;
class QWidget;

// This widget provides an image viewer and file picker, and exposes the path
// of the file it is referencing.
class PosterImageWell : public QWidget {
    Q_OBJECT

    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged
                       USER true);
    Q_PROPERTY(float scale READ getScale WRITE setScale NOTIFY scaleChanged
                       USER true);

public:
    explicit PosterImageWell(QWidget* parent = 0);

    // Get the current image path of the widget.  Returns a null QString if no
    // image is set.
    QString getPath() const { return mPath; }

    // Set the current image path of the widget.
    //
    // Does not invoke pathChanged.
    //
    // |path| - File path, or an empty string to clear the value. The filename
    //          is assumed to point to a valid file.
    void setPath(QString path);

    // Get the current poster scale.
    float getScale() const;

    // Set the current poster scale.
    //
    // |value| - Poster scale, between 0 and 1.
    void setScale(float value);

    // Set minimum and maximum size of the poster.
    //
    // |minSize| - Minimum size, in meters.
    // |maxSize| - Maximum size, in meters.
    void setMinMaxSize(float minSize, float maxSize);

    // Set the starting directory for the file dialog.  Defaults to the current
    // working directory, ".".  Does not have to match the path's base
    // directory.
    //
    // |startingDirectory| - The starting directory to show when the file dialog
    //                       loads.
    void setStartingDirectory(QString startingDirectory);

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

signals:
    // Emitted when the path stored by the widget changes.
    void pathChanged(QString path);

    // Emitted when the widget scale changes.
    void scaleChanged(float value);

    // Emitted when the user interacts with the control, even if the action
    // is canceled.
    void interaction();

private slots:
    // No image state.
    void on_filePicker_clicked();

    // With image state.
    void on_pickFileButton_clicked();
    void on_removeButton_clicked();
    void on_sizeSlider_valueChanged(double value);

private:
    void openFilePicker();

    // Removes any image and resets back to default state.
    void removeImage();

    // Returns true if the path was changed.
    bool setPathInternal(const QString& path);

    // Returns the path of the file from a QDropEvent or QDragEnterEvent's
    // mimeData if the mime data is supported.
    //
    // To be supported, the drop must be for a single file on the local system
    // with a PNG or JPEG extension.
    //
    // |mimeData| - QMimeData object from QDrag*Event::mimeData() or
    //              QDropEvent::mimeData().
    QString getPathIfValidDrop(const QMimeData* mimeData) const;

    std::unique_ptr<Ui::PosterImageWell> mUi;

    QString mStartingDirectory;
    QString mPath;
    QWidget mOverlayWidget;
    float mSliderValueScale = 1.0f;
};

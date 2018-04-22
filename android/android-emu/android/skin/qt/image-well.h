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

#include "ui_image-well.h"

#include <QWidget>
#include <memory>

// This widget provides an image viewer and file picker, and exposes the path
// of the file it is referencing.
class ImageWell : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathChanged
                       USER true);

public:
    explicit ImageWell(QWidget* parent = 0);

    QString getPath() const { return mPath; }

    // Set the current image path of the widget.
    //
    // Does not invoke pathChanged.
    //
    // |path| - File path, or an empty string to clear the value. The filename
    //          is assumed to point to a valid file.
    void setPath(QString path);

    // Set the starting directory for the file dialog.  Defaults to the current
    // working directory, ".".  Does not have to match the path's base
    // directory.
    //
    // |startingDirectory| - The starting directory to show when the file dialog
    //                       loads.
    void setStartingDirectory(QString startingDirectory);

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

signals:
    // Emitted when the path stored by the widget changes.
    void pathChanged(QString path);

    // Emitted when the user interacts with the control, even if the action
    // is canceled.
    void interaction();

private slots:
    void on_filePicker_clicked();

private:
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

    std::unique_ptr<Ui::ImageWell> mUi;

    QString mStartingDirectory;
    QString mPath;
};

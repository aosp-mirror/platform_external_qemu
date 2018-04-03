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
    bool setPathInternal(QString path);

    std::unique_ptr<Ui::ImageWell> mUi;

    QString mPath;
};

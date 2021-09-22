// Copyright (C) 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#ifndef RESIZABLEDIALOG_H
#define RESIZABLEDIALOG_H

#include <QDialog>
#include "android/resizable_display_config.h"

namespace Ui {
class ResizableDialog;
}

class ResizableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResizableDialog(QWidget *parent = nullptr);
    ~ResizableDialog();
    void showEvent(QShowEvent* event) override;
    PresetEmulatorSizeType getSize();

signals:
    void newResizableRequested(PresetEmulatorSizeType newSize);

private slots:
    void on_size_phone_clicked();
    void on_size_unfolded_clicked();
    void on_size_tablet_clicked();
    void on_size_desktop_clicked();

private:
    Ui::ResizableDialog *ui;
    void saveSize(PresetEmulatorSizeType size);
};

#endif // RESIZABLEDIALOG_H

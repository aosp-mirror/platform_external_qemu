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
#ifndef POSTURESELECTIONDIALOG_H
#define POSTURESELECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class PostureSelectionDialog;
}

class PostureSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PostureSelectionDialog(QWidget *parent = nullptr);
    ~PostureSelectionDialog();
    void showEvent(QShowEvent* event) override;

signals:
    void newPostureRequested(int newPosture);

private slots:
    void on_btn_closed_clicked();
    void on_btn_flipped_clicked();
    void on_btn_halfopen_clicked();
    void on_btn_open_clicked();
    void on_btn_tent_clicked();

private:
    Ui::PostureSelectionDialog *ui;

    // Hide buttons which refer to unsupported foldable postures.
    void hideUnsupportedPostureButtons();
    bool  mShown = false;
};

#endif // POSTURESELECTIONDIALOG_H

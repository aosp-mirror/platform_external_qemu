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

#pragma once

#include "ui_foldable-page.h"

#include "android/skin/qt/emulator-qt-window.h"

class EmulatorQtWindow;

struct QAndroidFoldableAgent;
class FoldablePage : public QWidget
{
    Q_OBJECT

public:
    explicit FoldablePage(QWidget *parent = 0);

    static void earlyInitialization();

private slots:
    void on_isOpen_toggled(bool checked);
    void on_configurationButtonGroup_buttonClicked(int idx);
    void on_densityButtonGroup_buttonClicked(int idx);

private:
    std::unique_ptr<Ui::FoldablePage> mUi;
};

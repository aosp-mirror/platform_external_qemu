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

#include "android/utils/compiler.h"

#include "ui_car-cluster-window.h"

#include <QFrame>
#include <QWidget>

#include <memory>

class EmulatorQtWindow;
class ExtendedWindow;

// This window is used to show car cluster window. It shows when
// android auto emulator start. Car cluster shows car sensors,
// including current gear, turn-by-turn navigation, speed
class CarClusterWindow : public QFrame {
    Q_OBJECT

public:
    CarClusterWindow(EmulatorQtWindow* window, QWidget* parent);
    ~CarClusterWindow();

    void show();
    void hide();

    void hideEvent(QHideEvent* event);
    void showEvent(QShowEvent* event);
    bool isDismissed();

private:
    bool mIsDismissed;
    EmulatorQtWindow* mEmulatorWindow;
    std::unique_ptr<Ui::CarClusterWindow> mCarClusterWindowUi;
};

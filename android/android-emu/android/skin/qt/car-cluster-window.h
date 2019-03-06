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

class EmulatorQtWindow;
class ExtendedWindow;

//This windowis used to show car cluster window It show's when
//android auto emualtor start. Car cluster shows the car sensors,
//including current gear, tryn-by-turn navigation, speed
//TODO: Where should I show the window? Above/below/left/right of the main window?
class CarClusterWindow : public QFrame
{
    Q_OBJECT

public:
    CarClusterWindow(EmulatorQtWindow* window,
                            QWidget* parent);
    ~CarClusterWindow();

    void show();
    void hide();

private:
    EmulatorQtWindow* mEmulatorWindow;
    std::unique_ptr<Ui::CarClusterWindow> mCarClusterWindowUi;
};


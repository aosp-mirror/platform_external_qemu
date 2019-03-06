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

#include "ui_instrumentclusterwindow.h"


#include <QFrame>
#include <QWidget>

class EmulatorQtWindow;
class ExtendedWindow;


class InstrumentClusterWindow : public QFrame
{
    Q_OBJECT

public:
    InstrumentClusterWindow(EmulatorQtWindow* window,
                            QWidget* parent);
    ~InstrumentClusterWindow();

    void show();
    void hide();
    void dockMainWindow();

private:
    EmulatorQtWindow* mEmulatorWindow;
    std::unique_ptr<Ui::InstrumentClusterWindow> mInstrumentClusterWindowUi;
};


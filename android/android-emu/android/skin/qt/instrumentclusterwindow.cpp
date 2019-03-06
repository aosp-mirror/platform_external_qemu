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
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/instrumentclusterwindow.h"
#include "android/emulation/proto/VehicleHalProto.pb.h"

InstrumentClusterWindow::InstrumentClusterWindow(EmulatorQtWindow *window,
                       QWidget* parent) :
    QFrame(nullptr),
    mInstrumentClusterWindowUi(new Ui::InstrumentClusterWindow),
    mEmulatorWindow(window)
{
    mInstrumentClusterWindowUi->setupUi(this);
}

InstrumentClusterWindow::~InstrumentClusterWindow()
{
}

void InstrumentClusterWindow::show(){
    QFrame::show();
}

void InstrumentClusterWindow::hide(){
    QFrame::hide();
}

void InstrumentClusterWindow::dockMainWindow() {
    // Align horizontally relative to the main window's frame.
    // Align vertically to its contents.
    // If we're frameless, adjust for a transparent border
    // around the skin.
    //TODO
    // move(parentWidget()->frameGeometry().left(),
    //      parentWidget()->geometry().bottom());
}
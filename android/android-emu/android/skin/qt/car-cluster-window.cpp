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
#include "android/skin/qt/car-cluster-window.h"
#include "android/skin/qt/emulator-qt-window.h"

static const int kCarClusterWindowWidth = 650;
static const int kCarClusterWindowHeight = 360;
CarClusterWindow::CarClusterWindow(EmulatorQtWindow* window, QWidget* parent)
    : QFrame(nullptr),
      mCarClusterWindowUi(new Ui::CarClusterWindow),
      mEmulatorWindow(window) {
    mCarClusterWindowUi->setupUi(this);
    this->setFixedSize(kCarClusterWindowWidth, kCarClusterWindowHeight);
    mIsDismissed = false;
}

CarClusterWindow::~CarClusterWindow() {}

void CarClusterWindow::show() {
    QFrame::show();
}

void CarClusterWindow::hide() {
    QFrame::hide();
}

void CarClusterWindow::hideEvent(QHideEvent* event) {
    mIsDismissed = true;
}

void CarClusterWindow::showEvent(QShowEvent* event) {
    mIsDismissed = false;
}

bool CarClusterWindow::isDismissed() {
    return mIsDismissed;
}
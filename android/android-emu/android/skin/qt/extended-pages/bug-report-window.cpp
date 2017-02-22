// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/extended-pages/bug-report-window.h"

#include "ui_bug-report-window.h"
#include "android/globals.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"

#include <QSettings>

BugReportWindow::BugReportWindow(QWidget* parent)
    : QFrame(parent),
      mUi(new Ui::BugReportWindow) {
// "Tool" type windows live in another layer on top of everything in OSX, which
// is undesirable because it means the extended window must be on top of the
// emulator window. However, on Windows and Linux, "Tool" type windows are the
// only way to make a window that does not have its own taskbar item.
#ifdef __APPLE__
    Qt::WindowFlags flag = Qt::Dialog;
#else
    Qt::WindowFlags flag = Qt::Tool;
#endif

    setWindowFlags(flag | Qt::WindowMinimizeButtonHint |
                   Qt::WindowCloseButtonHint);

    QSettings settings;
    bool onTop = settings.value(Ui::Settings::ALWAYS_ON_TOP, false).toBool();
    setFrameOnTop(this, onTop);
    mUi->setupUi(this);
}




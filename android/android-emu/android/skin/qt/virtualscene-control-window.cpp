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

#include "android/skin/qt/virtualscene-control-window.h"

#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/tool-window.h"

#include <QDesktopWidget>
#include <QPainter>
#include <QScreen>

static constexpr int kWindowHeight = 50;

VirtualSceneControlWindow::VirtualSceneControlWindow(ToolWindow* toolWindow,
                                                     QWidget* parent)
    : QFrame(parent), mToolWindow(toolWindow), mSizeTweaker(this) {
    setObjectName("VirtualSceneControlWindow");

    Qt::WindowFlags flags =
            Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint;

#ifdef __APPLE__
    // "Tool" type windows live in another layer on top of everything in OSX,
    // which is undesirable because it means the extended window must be on top
    // of the emulator window. However, on Windows and Linux, "Tool" type
    // windows are the only way to make a window that does not have its own
    // taskbar item.
    flags |= Qt::Dialog;
#else   // __APPLE__
    flags |= Qt::Tool;
#endif  // !__APPLE__

#ifdef __linux__
    // Without the hint below, X11 window systems will prevent this window from
    // being moved into a position where it is not fully visible. It is 
    // required so that when the emulator container is moved partially
    // offscreen, this window is "permitted" to follow it offscreen.
    flags |= Qt::X11BypassWindowManagerHint;
#endif  // __linux__

    setWindowFlags(flags);

    setWidth(parent->frameGeometry().width());
}

VirtualSceneControlWindow::~VirtualSceneControlWindow() = default;

void VirtualSceneControlWindow::setWidth(int width) {
    resize(QSize(width, kWindowHeight));
}

void VirtualSceneControlWindow::keyPressEvent(QKeyEvent* event) {
    mToolWindow->handleQtKeyEvent(event);
}

void VirtualSceneControlWindow::keyReleaseEvent(QKeyEvent* event) {
    mToolWindow->handleQtKeyEvent(event);
}

void VirtualSceneControlWindow::paintEvent(QPaintEvent*) {
    QPainter p;
    QPen pen(Qt::SolidLine);
    pen.setColor(Qt::black);
    pen.setWidth(1);
    p.begin(this);
    p.setPen(pen);

    double dpr = 1.0;
    int primary_screen_idx = qApp->desktop()->screenNumber(this);
    if (primary_screen_idx < 0) {
        primary_screen_idx = qApp->desktop()->primaryScreen();
    }
    const auto screens = QApplication::screens();
    if (primary_screen_idx >= 0 && primary_screen_idx < screens.size()) {
        const QScreen* const primary_screen = screens.at(primary_screen_idx);
        if (primary_screen) {
            dpr = primary_screen->devicePixelRatio();
        }
    }

    if (dpr > 1.0) {
        // Normally you'd draw the border with a (0, 0, w-1, h-1) rectangle.
        // However, there's some weirdness going on with high-density displays
        // that makes a single-pixel "slack" appear at the left and bottom
        // of the border. This basically adds 1 to compensate for it.
        p.drawRect(contentsRect());
    } else {
        p.drawRect(QRect(0, 0, width() - 1, height() - 1));
    }
    p.end();
}

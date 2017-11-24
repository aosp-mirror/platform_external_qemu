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

static constexpr int kMousePollIntervalMilliseconds = 16;
static constexpr int kWindowHeight = 50;
static const QColor kHighlightColor = QColor(2, 136, 209);

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
    QApplication::instance()->installEventFilter(this);

    connect(&mMousePoller, SIGNAL(timeout()), this, SLOT(slot_mousePoller()));
}

VirtualSceneControlWindow::~VirtualSceneControlWindow() {
    QApplication::instance()->removeEventFilter(this);
}

bool VirtualSceneControlWindow::handleQtKeyEvent(QKeyEvent* event) {
    const bool down = event->type() == QEvent::KeyPress;

    // Trigger when the Alt key but no other modifiers is held.
    if (event->key() == Qt::Key_Alt) {
        if (down && !mCaptureMouse && event->modifiers() == Qt::AltModifier) {
            setCaptureMouse(true);
        } else if (!down && mCaptureMouse) {
            setCaptureMouse(false);
        }

        return true;
    }

    // Look out for other modifier presses and cancel the capture.
    if (mCaptureMouse && down && event->modifiers() != Qt::AltModifier) {
        setCaptureMouse(false);
    }

    return false;
}

void VirtualSceneControlWindow::setWidth(int width) {
    resize(QSize(width, kWindowHeight));
}

void VirtualSceneControlWindow::setCaptureMouse(bool capture) {
    if (capture) {
        mOriginalMousePosition = QCursor::pos();

        QCursor cursor(Qt::BlankCursor);
        parentWidget()->activateWindow();
        parentWidget()->grabMouse(cursor);

        // Move cursor to the center of the window.
        mPreviousMousePosition = getMouseCaptureCenter();
        QCursor::setPos(mPreviousMousePosition);

        // Qt only sends mouse move events when the mouse is in the application
        // bounds, set up a timer to poll for mouse position changes in case the
        // mouse escapes.
        mMousePoller.start(kMousePollIntervalMilliseconds);
    } else {
        mMousePoller.stop();

        QCursor::setPos(mOriginalMousePosition);
        parentWidget()->releaseMouse();
    }

    update();

    mCaptureMouse = capture;
}

void VirtualSceneControlWindow::hideEvent(QHideEvent* event) {
    QFrame::hide();

    if (mCaptureMouse) {
        setCaptureMouse(false);
    }
}

bool VirtualSceneControlWindow::eventFilter(QObject* target, QEvent* event) {
    if (mCaptureMouse) {
        if (event->type() == QEvent::MouseMove) {
            updateMouselook();
            return true;
        } else if (event->type() == QEvent::Wheel) {
            return true;
        }
    }

    return QObject::eventFilter(target, event);
}

void VirtualSceneControlWindow::keyPressEvent(QKeyEvent* event) {
    mToolWindow->handleQtKeyEvent(event);
}

void VirtualSceneControlWindow::keyReleaseEvent(QKeyEvent* event) {
    mToolWindow->handleQtKeyEvent(event);
}

void VirtualSceneControlWindow::paintEvent(QPaintEvent*) {
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

    QRect rect(0, 0, width() - 1, height() - 1);

    if (dpr > 1.0) {
        // Normally you'd draw the border with a (0, 0, w-1, h-1) rectangle.
        // However, there's some weirdness going on with high-density displays
        // that makes a single-pixel "slack" appear at the left and bottom
        // of the border. This basically adds 1 to compensate for it.
        rect = contentsRect();
    }

    QPainter p;
    p.begin(this);

    if (mCaptureMouse) {
        p.fillRect(rect, kHighlightColor);
    }

    QPen outlinePen(Qt::SolidLine);
    outlinePen.setColor(Qt::black);
    outlinePen.setWidth(1);
    p.setPen(outlinePen);
    p.drawRect(rect);
    p.end();
}

void VirtualSceneControlWindow::slot_mousePoller() {
    updateMouselook();
}

void VirtualSceneControlWindow::updateMouselook() {
    mPreviousMousePosition = getMouseCaptureCenter();
    QCursor::setPos(mPreviousMousePosition);
}

QPoint VirtualSceneControlWindow::getMouseCaptureCenter() {
    QWidget* container = parentWidget();
    return container->pos() +
           QPoint(container->width() / 2, container->height() / 2);
}

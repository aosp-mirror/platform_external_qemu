// Copyright (C) 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/emulator-overlay.h"

#include "android/skin/qt/emulator-container.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/tool-window.h"

EmulatorOverlay::EmulatorOverlay(EmulatorQtWindow* window,
                                 EmulatorContainer* container)
    : QFrame(container),
      mEmulatorWindow(window),
      mContainer(container),
      mRubberBand(QRubberBand::Rectangle, this),
      mCursor(":/cursor/zoom_cursor"),
      mMultitouchCenter(-1, -1),
      mPrimaryTouchPoint(-1, -1),
      mReleaseOnClose(false),
      mFlashValue(0),
      mMode(OverlayMode::Hidden) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

// Without the hint below, X11 window systems will prevent this window from
// being moved
// into a position where they are not fully visible. It is required so that when
// the
// emulator container is moved partially offscreen, this overlay is "permitted"
// to
// follow it offscreen.
#ifdef __linux__
    setWindowFlags(windowFlags() | Qt::X11BypassWindowManagerHint);
#endif

    mRubberBand.hide();

    // Load in higher-resolution images on higher resolution screens
    if (devicePixelRatio() > 1.5) {
        mCenterImage.load(":/multitouch/center_point_2x");
        mCenterImage.setDevicePixelRatio(devicePixelRatio());

        mTouchImage.load(":/multitouch/touch_point_2x");
        mTouchImage.setDevicePixelRatio(devicePixelRatio());
    } else {
        mCenterImage.load(":/multitouch/center_point");
        mTouchImage.load(":/multitouch/touch_point");
    }

    mCenterPointRadius = mCenterImage.width() / devicePixelRatio();
    mTouchPointRadius = mTouchImage.width() / devicePixelRatio();

    mFlashAnimation.setStartValue(250);
    mFlashAnimation.setEndValue(0);
    mFlashAnimation.setEasingCurve(QEasingCurve::Linear);
    QObject::connect(&mFlashAnimation, SIGNAL(finished()), this,
                     SLOT(slot_animationFinished()));
    QObject::connect(&mFlashAnimation, SIGNAL(valueChanged(QVariant)), this,
                     SLOT(slot_animationValueChanged(QVariant)));
}

EmulatorOverlay::~EmulatorOverlay() {}

void EmulatorOverlay::focusOutEvent(QFocusEvent* event) {
    if (mMode == OverlayMode::Multitouch) {
        hide();
    }
    QFrame::focusOutEvent(event);
}

void EmulatorOverlay::hideEvent(QHideEvent* event) {
    mRubberBand.hide();
}

void EmulatorOverlay::keyPressEvent(QKeyEvent* event) {
    mEmulatorWindow->keyPressEvent(event);
}

void EmulatorOverlay::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Alt && mMode == OverlayMode::Multitouch) {
        hide();
    } else {
        mEmulatorWindow->keyReleaseEvent(event);
    }
}

void EmulatorOverlay::mouseMoveEvent(QMouseEvent* event) {
    if (mMode == OverlayMode::Zoom) {
        mRubberBand.setGeometry(
                QRect(mRubberbandOrigin, event->pos())
                        .normalized()
                        .intersected(QRect(0, 0, width(), height())));
    } else if (mMode == OverlayMode::Multitouch) {
        mPrimaryTouchPoint = event->pos();
        update();

        if (event->buttons() & Qt::LeftButton) {
            mEmulatorWindow->handleMouseEvent(
                    kEventMouseMotion, kMouseButtonLeft, mPrimaryTouchPoint);
            mEmulatorWindow->handleMouseEvent(kEventMouseMotion,
                                              kMouseButtonSecondaryTouch,
                                              getSecondaryTouchPoint());
        } else if (event->buttons() & Qt::RightButton) {
            updateMultitouchCenter(event->pos());
        }
    }
}

void EmulatorOverlay::mousePressEvent(QMouseEvent* event) {
    if (mMode == OverlayMode::Zoom) {
        mRubberbandOrigin = event->pos();
        mRubberBand.setGeometry(QRect(mRubberbandOrigin, QSize()));
        mRubberBand.show();
    } else if (mMode == OverlayMode::Multitouch) {
        if (!androidHwConfig_isScreenMultiTouch(android_hw)) {
            showErrorDialog(tr("Your virtual device is not configured for "
                               "multi-touch input."),
                            tr("Multi-touch"));
        } else if (event->button() == Qt::LeftButton) {
            mPrimaryTouchPoint = event->pos();
            mReleaseOnClose = true;
            update();

            mEmulatorWindow->handleMouseEvent(kEventMouseButtonDown,
                                              kMouseButtonLeft,
                                              mPrimaryTouchPoint);
            mEmulatorWindow->handleMouseEvent(kEventMouseButtonDown,
                                              kMouseButtonSecondaryTouch,
                                              getSecondaryTouchPoint());
        } else if (event->button() == Qt::RightButton) {
            updateMultitouchCenter(event->pos());
        }
    }
}

void EmulatorOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (mMode == OverlayMode::Zoom) {
        QRect geom = mRubberBand.geometry();
        QPoint localPoint =
                mEmulatorWindow->mapFromGlobal(mapToGlobal(geom.center()));

        // Assume that very, very small dragged rectangles were actually just
        // clicks that
        // slipped a bit
        int areaSq =
                geom.width() * geom.width() + geom.height() * geom.height();

        // Left click zooms in
        if (event->button() == Qt::LeftButton) {
            // Click events (with no drag) keep the mouse focused on the same
            // pixel
            if (areaSq < 20) {
                mEmulatorWindow->zoomIn(
                        localPoint, mContainer->mapFromGlobal(QCursor::pos()));

                // Dragged rectangles will center said rectangle and zoom in as
                // much as possible
            } else {
                mEmulatorWindow->zoomTo(localPoint, geom.size());
            }

            // Right click zooms out
        } else if (event->button() == Qt::RightButton) {
            // Click events (with no drag) keep the mouse focused on the same
            // pixel
            if (areaSq < 20) {
                mEmulatorWindow->zoomOut(
                        localPoint, mContainer->mapFromGlobal(QCursor::pos()));

                // Dragged rectangles will reset zoom to 1
            } else {
                mEmulatorWindow->zoomReset();
            }
        }
        mRubberBand.hide();
    } else if (mMode == OverlayMode::Multitouch) {
        if (event->button() == Qt::LeftButton) {
            mPrimaryTouchPoint = event->pos();
            mReleaseOnClose = false;
            mEmulatorWindow->handleMouseEvent(
                    kEventMouseButtonUp, kMouseButtonLeft, mPrimaryTouchPoint);
            mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp,
                                              kMouseButtonSecondaryTouch,
                                              getSecondaryTouchPoint());
            update();
        }
    }
}

void EmulatorOverlay::paintEvent(QPaintEvent* e) {
    // A frameless and translucent window (AKA a totally invisible one like
    // this) will
    // actually not appear at all on some systems. To circumvent this, we draw a
    // window-sized quad that is basically invisible, forcing the window to be
    // drawn.
    // Because this is not strange enough, the alpha value of said quad *must*
    // be above a
    // certain threshold else the window will simply not appear.
    int alpha = mFlashValue * 255;

// On OSX, this threshold is 12, so make alpha 13.
// On Windows, this threshold is 0, so make alpha 1.
// On Linux, this threshold is 0.
#if __APPLE__
    if (alpha < 13)
        alpha = 13;
#elif _WIN32
    if (alpha < 1)
        alpha = 1;
#endif

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QRect bg(QPoint(0, 0), size());
    painter.fillRect(bg, QColor(255, 255, 255, alpha));

    if (mMode == OverlayMode::Multitouch) {
        painter.translate(-mCenterPointRadius / 2, -mCenterPointRadius / 2);
        painter.drawImage(mMultitouchCenter, mCenterImage);
        painter.resetTransform();

        painter.translate(-mTouchPointRadius / 2, -mTouchPointRadius / 2);
        painter.drawImage(mPrimaryTouchPoint, mTouchImage);
        painter.drawImage(getSecondaryTouchPoint(), mTouchImage);
        painter.resetTransform();

        painter.setOpacity(.67);
        painter.setPen(QPen(QColor("#00BEA4")));

        QLineF lineToPrimary =
                QLineF(QPoint(), mPrimaryTouchPoint - mMultitouchCenter);
        if (lineToPrimary.length() > mTouchPointRadius) {
            QPointF delta =
                    (lineToPrimary.unitVector().p2() * (mTouchPointRadius / 2));
            painter.drawLine(QLineF(mMultitouchCenter + delta,
                                    mPrimaryTouchPoint - delta));
            painter.drawLine(QLineF(mMultitouchCenter - delta,
                                    getSecondaryTouchPoint() + delta));
        }
    }
}

void EmulatorOverlay::resizeEvent(QResizeEvent* event) {
    mMultitouchCenter = QPoint(width() / 2, height() / 2);
}

void EmulatorOverlay::showEvent(QShowEvent* event) {
    setFocus();
    activateWindow();
}

void EmulatorOverlay::showAsFlash() {
    if (mMode == OverlayMode::Hidden) {
        mMode = OverlayMode::Flash;
        mFlashAnimation.start();
        show();
    }
}

void EmulatorOverlay::hideForFlash() {
    if (mMode == OverlayMode::Flash) {
        hide();
    }
}

void EmulatorOverlay::showForMultitouch() {
    if (mMode != OverlayMode::Hidden)
        return;

    // Show and render the frame once before the mode is changed.
    // This ensures that the first frame of the overlay that is rendered *does
    // not* show
    // the center point, as this has adverse effects on OSX (it seems to corrupt
    // the alpha
    // portion of the color buffer, leaving the original location of the point
    // with a
    // different alpha, resulting in a shadow).
    show();
    update();

    mMode = OverlayMode::Multitouch;
    setCursor(Qt::ArrowCursor);
    setMouseTracking(true);
    mPrimaryTouchPoint = mapFromGlobal(QCursor::pos());
}

void EmulatorOverlay::showForZoom() {
    if (mMode != OverlayMode::Hidden)
        return;

    mMode = OverlayMode::Zoom;
    setCursor(QCursor(mCursor));
    show();
}

void EmulatorOverlay::hide() {
    QFrame::hide();
    setMouseTracking(false);
    mMode = OverlayMode::Hidden;
    mContainer->setFocus();
    mContainer->activateWindow();

    if (mReleaseOnClose) {
        mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp, kMouseButtonLeft,
                                          mPrimaryTouchPoint);

        mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp,
                                          kMouseButtonSecondaryTouch,
                                          getSecondaryTouchPoint());
        mReleaseOnClose = false;
    }
}

void EmulatorOverlay::slot_animationFinished() {
    hideForFlash();
}

void EmulatorOverlay::slot_animationValueChanged(const QVariant& value) {
    mFlashValue = value.toDouble() / mFlashAnimation.startValue().toDouble();
    repaint();
}

QPoint EmulatorOverlay::getSecondaryTouchPoint() const {
    return mPrimaryTouchPoint + 2 * (mMultitouchCenter - mPrimaryTouchPoint);
}

void EmulatorOverlay::updateMultitouchCenter(const QPoint& pos) {
    if (QRect(QPoint(), this->size()).contains(pos)) {
        mMultitouchCenter = pos;
        update();
    }
}

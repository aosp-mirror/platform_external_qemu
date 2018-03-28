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

#include <QDesktopWidget>

// The RGB values for the "resize" rectangle
#define RESIZE_RGB 0x01, 0xBE, 0xA4

EmulatorOverlay::MultitouchResources::MultitouchResources(
    const QString& centerImgPath,
    const QString& touchImgPath,
    float dpr) {

    centerImage.load(centerImgPath);
    touchImage.load(touchImgPath);

    if (dpr > 1.5f) {
        centerImage.setDevicePixelRatio(dpr);
        touchImage.setDevicePixelRatio(dpr);
    }

    centerImageRadius = centerImage.width() / dpr;
    touchImageRadius = touchImage.width() / dpr;
}

EmulatorOverlay::EmulatorOverlay(EmulatorQtWindow* window,
                                 EmulatorContainer* container)
    : QFrame(container),
      mEmulatorWindow(window),
      mContainer(container),
      mRubberBand(QRubberBand::Rectangle, this),
      mZoomCursor(":/cursor/zoom_cursor"),
      mMultitouchResources([this] {
          float dpr = devicePixelRatioF();
          if (dpr >= 1.5f) {
            return std::make_tuple(":/multitouch/center_point_2x",
                                   ":/multitouch/touch_point_2x", dpr);
          } else {
            return std::make_tuple(":/multitouch/center_point",
                                   ":/multitouch/touch_point", dpr);
          }
      }),
      mMultitouchCenter(-1, -1),
      mPrimaryTouchPoint(-1, -1),
      mSecondaryTouchPoint(-1, -1),
      mReleaseOnClose(false),
      mLerpValue(1),
      mFlashValue(0),
      mMode(OverlayMode::Hidden) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

// Without the hint below, X11 window systems will prevent this window from
// being moved into a position where it is not fully visible. It is required
// so that when the emulator container is moved partially offscreen, this
// overlay is "permitted" to follow it offscreen.
#ifdef __linux__
    setWindowFlags(windowFlags() | Qt::X11BypassWindowManagerHint);
#endif

    mFlashAnimation.setStartValue(250);
    mFlashAnimation.setEndValue(0);
    mFlashAnimation.setEasingCurve(QEasingCurve::Linear);
    QObject::connect(&mFlashAnimation, SIGNAL(finished()), this,
                     SLOT(slot_flashAnimationFinished()));
    QObject::connect(&mFlashAnimation, SIGNAL(valueChanged(QVariant)), this,
                     SLOT(slot_flashAnimationValueChanged(QVariant)));

    mTouchPointAnimation.setStartValue(0);
    mTouchPointAnimation.setEndValue(250);
    mTouchPointAnimation.setEasingCurve(QEasingCurve::OutQuint);
    QObject::connect(&mTouchPointAnimation, SIGNAL(valueChanged(QVariant)),
                     this, SLOT(slot_animationValueChanged(QVariant)));
}

EmulatorOverlay::~EmulatorOverlay() {}

void EmulatorOverlay::focusOutEvent(QFocusEvent* event) {
    if (mMode == OverlayMode::Multitouch) {
        hideAndFocusContainer();
    } else if (mMode == OverlayMode::Zoom) {
        hide();
    }
    QFrame::focusOutEvent(event);
}

void EmulatorOverlay::hideEvent(QHideEvent* event) {
    mRubberBand.ifExists([&] { mRubberBand->hide(); });
}

void EmulatorOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control && mMode == OverlayMode::Zoom) {
        hideAndFocusContainer();
        mMode = OverlayMode::UserHiddenZoom;
    } else {
        mEmulatorWindow->keyPressEvent(event);
    }
}

void EmulatorOverlay::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Control && (mMode == OverlayMode::Multitouch ||
                                            mMode == OverlayMode::Resize       ))
    {
        hideAndFocusContainer();
    } else {
        mEmulatorWindow->keyReleaseEvent(event);
    }
}

void EmulatorOverlay::mouseMoveEvent(QMouseEvent* event) {
    if (mMode == OverlayMode::Zoom) {
        mRubberBand->setGeometry(
                QRect(mRubberbandOrigin, event->pos())
                        .normalized()
                        .intersected(QRect(0, 0, width(), height())));
    } else if (mMode == OverlayMode::Multitouch ||
               mMode == OverlayMode::Resize)
    {
        mLastMousePos = event->pos();
        updateTouchPoints(event);
        update();
        generateTouchEvents(event);
    }
}

void EmulatorOverlay::mousePressEvent(QMouseEvent* event) {
    if (mMode == OverlayMode::Zoom) {
        mRubberbandOrigin = event->pos();
        mRubberBand->setGeometry(QRect(mRubberbandOrigin, QSize()));
        mRubberBand->show();
    } else if (mMode == OverlayMode::Multitouch) {
        if (!androidHwConfig_isScreenMultiTouch(android_hw)) {
            showErrorDialog(tr("Your virtual device is not configured for "
                               "multi-touch input."),
                            tr("Multi-touch"));
        } else {
            mReleaseOnClose = true;
            generateTouchEvents(event);
        }
    }
}

void EmulatorOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (mMode == OverlayMode::Zoom) {
        QRect geom = mRubberBand->geometry();
        QPoint localPoint =
                mEmulatorWindow->mapFromGlobal(mapToGlobal(geom.center()));

        // Assume that very, very small dragged rectangles were actually just
        // clicks that slipped a bit
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
        mRubberBand->hide();
    } else if (mMode == OverlayMode::Multitouch ||
               mMode == OverlayMode::Resize)
    {
        generateTouchEvents(event);
    }
}

void EmulatorOverlay::moveEvent(QMoveEvent* event) {
    if (mMode == OverlayMode::Multitouch ||
        mMode == OverlayMode::Resize)
    {
        hideAndFocusContainer();
    }
}

void EmulatorOverlay::paintEvent(QPaintEvent* e) {
    // A frameless and translucent window (AKA a totally invisible one like
    // this) will actually not appear at all on some systems. To circumvent
    // this, we draw a window-sized quad that is basically invisible, forcing
    // the window to be drawn. Because this is not strange enough, the alpha
    // value of said quad *must* be above a certain threshold else the window
    // will simply not appear.
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

    if (mMode == OverlayMode::Resize) {
        // Draw the dashed-line box to show how big the resized window will be
        drawResizeBox(&painter, alpha);
        return;
    }

    QRect bg(QPoint(0, 0), size());
    painter.fillRect(bg, QColor(255, 255, 255, alpha));

    if (mMode == OverlayMode::Multitouch) {
        const auto& mtRes = mMultitouchResources.get();

        double lerpValue = mIsSwipeGesture ? 1.0 - mLerpValue : mLerpValue;
        QPoint primaryPoint = lerpValue * primaryPinchPoint() +
                              (1.0 - lerpValue) * primarySwipePoint();
        QPoint secondaryPoint = lerpValue * secondaryPinchPoint() +
                                (1.0 - lerpValue) * secondaryTouchPoint();

        painter.translate(-mtRes.touchImageRadius / 2, -mtRes.touchImageRadius / 2);
        painter.drawImage(primaryPoint, mtRes.touchImage);
        painter.drawImage(secondaryPoint, mtRes.touchImage);
        painter.resetTransform();

        painter.setOpacity(lerpValue);
        painter.translate(-mtRes.centerImageRadius / 2, -mtRes.centerImageRadius / 2);
        painter.drawImage(mMultitouchCenter, mtRes.centerImage);
        painter.resetTransform();

        painter.setOpacity(.67 * lerpValue);
        painter.setPen(QPen(QColor("#00BEA4")));

        QLineF lineToOne = QLineF(QPoint(), primaryPoint - mMultitouchCenter);
        if (lineToOne.length() > mtRes.touchImageRadius) {
            QPointF delta =
                    (lineToOne.unitVector().p2() * (mtRes.touchImageRadius / 2));
            painter.drawLine(
                    QLineF(mMultitouchCenter + delta, primaryPoint - delta));
        }

        QLineF lineToTwo = QLineF(QPoint(), secondaryPoint - mMultitouchCenter);
        if (lineToTwo.length() > mtRes.touchImageRadius) {
            QPointF delta =
                    (lineToTwo.unitVector().p2() * (mtRes.touchImageRadius / 2));
            painter.drawLine(
                    QLineF(mMultitouchCenter + delta, secondaryPoint - delta));
        }
    }
}

void EmulatorOverlay::showEvent(QShowEvent* event) {
    setFocus();
    activateWindow();
}

void EmulatorOverlay::showAsFlash() {
    if (mMode == OverlayMode::Hidden) {
        mMode = OverlayMode::Flash;
    }
    mFlashAnimation.start();
    show();
}

void EmulatorOverlay::hideForFlash() {
    if (mMode == OverlayMode::Flash) {
        hideAndFocusContainer();
    }
}

void EmulatorOverlay::showForMultitouch(bool centerTouches) {
    if (mMode != OverlayMode::Hidden)
        return;

    if (!geometry().contains(QCursor::pos()))
        return;

    // Show and render the frame once before the mode is changed.
    // This ensures that the first frame of the overlay that is rendered *does
    // not* show the center point, as this has adverse effects on OSX (it seems
    // to corrupt the alpha portion of the color buffer, leaving the original
    // location of the point with a different alpha, resulting in a shadow).
    show();
    update();

    mMode = OverlayMode::Multitouch;
    setCursor(Qt::ArrowCursor);
    setMouseTracking(true);

    QPoint mousePosition = mapFromGlobal(QCursor::pos());
    mPrimaryTouchPoint = mousePosition;
    mMultitouchCenter = centerTouches ? mEmulatorWindow->deviceGeometry().center()
                                      : mousePosition;
    mSecondaryTouchPoint = secondaryPinchPoint();
    mLastMousePos = mousePosition;
    mIsSwipeGesture = false;
}

void EmulatorOverlay::showForResize(int whichCorner) {
    QRect screenGeo;
    QDesktopWidget* desktop = ((QApplication*)QApplication::instance())->desktop();
    int screenNum = desktop->screenNumber(mContainer);  // Screen holding the app

    if (screenNum < 0) {
        // No active screen!
        return;
    }
    screenGeo = desktop->screenGeometry(screenNum);

    // Set this widget to the full screen
    setGeometry(screenGeo);

    // Show and render the frame once before the mode is changed.
    // This ensures that the first frame of the overlay that is rendered *does
    // not* show the center point, as this has adverse effects on OSX (it seems
    // to corrupt the alpha portion of the color buffer, leaving the original
    // location of the point with a different alpha, resulting in a shadow).
    show();
    update();

    mMode = OverlayMode::Resize;

    QPoint mousePosition  = QCursor::pos();

    mResizeCorner = whichCorner;
    mLastMousePos = mousePosition;
    mIsSwipeGesture = false;
}

// Draw a dashed-line box to indicate how big the resized
// window will be. The box retains the aspect ratio that
// the AVD has.
void EmulatorOverlay::drawResizeBox(QPainter* painter, int alpha) {
    QSize overlaySize = size();
    QRect bg(QPoint(0, 0), overlaySize);
    painter->fillRect(bg, QColor(255, 255, 255, alpha));

    QPoint mousePosition = QCursor::pos();
    QRect deviceGeo = mContainer->geometry();

    // Get the (x,y) and (height x width) of the box.
    int boxX, boxW;
    if (mResizeCorner == 0 || mResizeCorner == 3) { // Dragging the left
        boxW = deviceGeo.width()  - (mousePosition.x() - deviceGeo.x()) - mEmulatorWindow->getRightTransparency();
    } else { // Dragging the right
        boxW = mousePosition.x() - deviceGeo.x() - mEmulatorWindow->getLeftTransparency();
    }
    if (boxW < 256) boxW = 256;

    int boxY, boxH;
    if (mResizeCorner == 0 || mResizeCorner == 1) { // Dragging the top
        boxH = deviceGeo.height() - (mousePosition.y() - deviceGeo.y()) - mEmulatorWindow->getBottomTransparency();
    } else { // Dragging the bottom
        boxH = mousePosition.y() - deviceGeo.y() - mEmulatorWindow->getTopTransparency();
    }
    if (boxH < 256) boxH = 256;

    // Get the container size
    int containerW = deviceGeo.width() -
                              mEmulatorWindow->getLeftTransparency() - mEmulatorWindow->getRightTransparency();
    int containerH = deviceGeo.height() -
                              mEmulatorWindow->getTopTransparency() - mEmulatorWindow->getBottomTransparency();

    // Adjust the box size to match the container's aspect ratio
    QSize boxSize(containerW, containerH);
    boxSize.scale(boxW, boxH, Qt::KeepAspectRatio);
    boxW = boxSize.width();
    boxH = boxSize.height();

    // Compute the box location using this adjusted size
    if (mResizeCorner == 0 || mResizeCorner == 3) { // Dragging the left
        boxX = deviceGeo.x() + deviceGeo.width() - mEmulatorWindow->getRightTransparency() - boxW;
    } else { // Dragging the right
        boxX = deviceGeo.x() + mEmulatorWindow->getLeftTransparency();
    }
    if (mResizeCorner == 0 || mResizeCorner == 1) { // Dragging the top
        boxY = deviceGeo.y() + deviceGeo.height() - mEmulatorWindow->getBottomTransparency() - boxH;
    } else { // Dragging the bottom
        boxY = deviceGeo.y() + mEmulatorWindow->getTopTransparency();
    }

    // Need to subtract off the screen origin
    boxX -= x();
    boxY -= y();

    // Fill the entire rectangle with a see-through color
    painter->fillRect(boxX, boxY, boxW, boxH, QBrush(QColor(RESIZE_RGB, 64)));

    // Outline the rectangle with the same color, but opaque
    QPen pen = painter->pen();
    pen.setWidth(2);
    pen.setColor(QColor(RESIZE_RGB, 255));
    pen.setStyle(Qt::SolidLine);
    painter->setPen(pen);
    painter->drawRect(boxX, boxY, boxW, boxH);
}

void EmulatorOverlay::paintForResize(int mouseX, int mouseY){
    mMode = OverlayMode::Resize;
    // Trigger a paint event
    repaint();
}

void EmulatorOverlay::showForZoom() {
    if (mMode != OverlayMode::Hidden)
        return;

    mMode = OverlayMode::Zoom;
    setCursor(QCursor(mZoomCursor.get()));
    show();
}

void EmulatorOverlay::showForZoomUserHidden() {
    if (mMode != OverlayMode::UserHiddenZoom)
        return;

    mMode = OverlayMode::Hidden;
    showForZoom();
}

bool EmulatorOverlay::wasZoomUserHidden() const {
    return mMode == OverlayMode::UserHiddenZoom;
}

void EmulatorOverlay::hide() {
    QFrame::hide();
    setMouseTracking(false);

    if (mMode == OverlayMode::Resize) {
        // Resize mode enlarged the overlay. Put it back to
        // the container size.
        setGeometry(mContainer->geometry());
        mContainer->activateWindow();
    }

    mMode = OverlayMode::Hidden;

    if (mReleaseOnClose) {
        mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp, kMouseButtonLeft,
                                          mPrimaryTouchPoint, QPoint(0, 0));

        mEmulatorWindow->handleMouseEvent(kEventMouseButtonUp,
                                          kMouseButtonSecondaryTouch,
                                          mSecondaryTouchPoint, QPoint(0, 0));
        mReleaseOnClose = false;
    }
}

void EmulatorOverlay::slot_flashAnimationFinished() {
    hideForFlash();
}

void EmulatorOverlay::slot_flashAnimationValueChanged(const QVariant& value) {
    mFlashValue = value.toDouble() / mFlashAnimation.startValue().toDouble();
    repaint();
}

void EmulatorOverlay::slot_animationValueChanged(const QVariant& value) {
    mLerpValue = value.toDouble() / mTouchPointAnimation.endValue().toDouble();
    repaint();
}

void EmulatorOverlay::hideAndFocusContainer() {
    hide();
    mContainer->setFocus();
    mContainer->activateWindow();
}

void EmulatorOverlay::generateTouchEvents(QMouseEvent* event) {
    SkinEventType eventType = (SkinEventType)0;

    if (event->type() == QMouseEvent::MouseButtonPress) {
        if (event->button() == Qt::RightButton) {
            mLerpValue = 0.0;
            mIsSwipeGesture = true;
            mTouchPointAnimation.start();
            updateTouchPoints(event);
        }
        eventType = kEventMouseButtonDown;
    } else if (event->type() == QMouseEvent::MouseButtonRelease) {
        if (event->button() == Qt::RightButton) {
            mLerpValue = 0.0;
            mIsSwipeGesture = false;
            mTouchPointAnimation.start();
            updateTouchPoints(event);
        }
        eventType = kEventMouseButtonUp;
    } else if (event->type() == QMouseEvent::MouseMove) {
        eventType = kEventMouseMotion;
    }

    if (eventType) {
        mEmulatorWindow->handleMouseEvent(eventType, kMouseButtonLeft,
                                          mPrimaryTouchPoint, QPoint(0, 0), true);
        mEmulatorWindow->handleMouseEvent(eventType, kMouseButtonSecondaryTouch,
                                          mSecondaryTouchPoint, QPoint(0, 0));
    }
}

void EmulatorOverlay::updateTouchPoints(QMouseEvent* event) {
    if (!mIsSwipeGesture) {
        mPrimaryTouchPoint = primaryPinchPoint();
        mSecondaryTouchPoint = secondaryPinchPoint();
    } else {
        mPrimaryTouchPoint = primarySwipePoint();
        mSecondaryTouchPoint = secondaryTouchPoint();
    }
}

QPoint EmulatorOverlay::primaryPinchPoint() const {
    return mLastMousePos;
}

QPoint EmulatorOverlay::secondaryPinchPoint() const {
    return mLastMousePos + 2 * (mMultitouchCenter - mLastMousePos);
}

QPoint EmulatorOverlay::primarySwipePoint() const {
    return mLastMousePos + QPoint(width() * .1, 0);
}

QPoint EmulatorOverlay::secondaryTouchPoint() const {
    return mLastMousePos - QPoint(width() * .1, 0);
}

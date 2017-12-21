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
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/tool-window.h"

#include <QDesktopWidget>
#include <QPainter>
#include <QScreen>

#ifdef __APPLE__
#include <Carbon/Carbon.h>  // For kVK_ANSI_E
#endif

#include <glm/gtx/euler_angles.hpp>

static constexpr int kMousePollIntervalMilliseconds = 16;
static constexpr int kWindowHeight = 50;
static const QColor kHighlightColor = QColor(2, 136, 209);

static const float kPixelsToRotationRadians = 0.2f * static_cast<float>(M_PI / 180.0f);
static const float kMovementVelocityMetersPerSecond = 1.0f;
static const float kMinVerticalRotationDegrees = -80.0f;
static const float kMaxVerticalRotationDegrees = 80.0f;

static const char* kTextStyleFormatString =
        "font-size:%1;background-color:transparent;";
static const char* kHighlightTextStyle = "color:#eee";

VirtualSceneControlWindow::VirtualSceneControlWindow(ToolWindow* toolWindow,
                                                     QWidget* parent)
    : QFrame(parent), mToolWindow(toolWindow), mSizeTweaker(this),
      mControlsUi(new Ui::VirtualSceneControls()) {
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

    mControlsUi->setupUi(this);
    updateHighlightStyle();

    setWidth(parent->frameGeometry().width());
    QApplication::instance()->installEventFilter(this);

    connect(&mMousePoller, SIGNAL(timeout()), this, SLOT(slot_mousePoller()));
}

VirtualSceneControlWindow::~VirtualSceneControlWindow() {
    QApplication::instance()->removeEventFilter(this);
}

bool VirtualSceneControlWindow::handleQtKeyEvent(QKeyEvent* event,
                                                 QtKeyEventSource source) {
    const bool down = event->type() == QEvent::KeyPress;

    // Trigger when the Alt key but no other modifiers is held.
    if (event->key() == Qt::Key_Alt &&
        source != QtKeyEventSource::ExtendedWindow) {
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

void VirtualSceneControlWindow::updateTheme(const QString& styleSheet) {
    setStyleSheet(styleSheet);
    updateHighlightStyle();
}

void VirtualSceneControlWindow::setAgent(const UiEmuAgent* agentPtr) {
    mSensorsAgent = agentPtr->sensors;
}

void VirtualSceneControlWindow::setWidth(int width) {
    resize(QSize(width, kWindowHeight));
}

void VirtualSceneControlWindow::setCaptureMouse(bool capture) {
    if (capture) {
        mOriginalMousePosition = QCursor::pos();

        // Get the starting rotation.
        if (mSensorsAgent) {
            glm::vec3 eulerDegrees;
            mSensorsAgent->getPhysicalParameter(
                    PHYSICAL_PARAMETER_ROTATION, &eulerDegrees.x,
                    &eulerDegrees.y, &eulerDegrees.z,
                    PARAMETER_VALUE_TYPE_TARGET);

            // The physical model represents rotation in the X Y Z order, but
            // for mouselook we need the Y X Z order. Convert the rotation order
            // via a quaternion and decomposing the rotations.
            const glm::quat rotation = glm::eulerAngleXYZ(
                    glm::radians(eulerDegrees.x), glm::radians(eulerDegrees.y),
                    glm::radians(eulerDegrees.z));

            mEulerRotationRadians.y = atan2(rotation.y, rotation.w) * 2.0f;

            const glm::quat xzRotation =
                    glm::angleAxis(-mEulerRotationRadians.y,
                                   glm::vec3(0.0f, 1.0f, 0.0f)) *
                    rotation;
            mEulerRotationRadians.x = glm::pitch(xzRotation);
            mEulerRotationRadians.z = glm::roll(xzRotation);

            if (mEulerRotationRadians.x > M_PI) {
                mEulerRotationRadians.x -= static_cast<float>(2 * M_PI);
            }

            mSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_AMBIENT_MOTION, 0.005f, 0.f, 0.f,
                    PHYSICAL_INTERPOLATION_SMOOTH);
        }

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

        // Unset velocity.
        for (bool& pressed : mKeysHeld) {
            pressed = false;
        }

        if (mSensorsAgent) {
            mSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_AMBIENT_MOTION, 0.f,
                    0.f, 0.f,
                    PHYSICAL_INTERPOLATION_SMOOTH);
        }

        updateVelocity();
    }

    emit(virtualSceneControlsEngaged(capture));

    mCaptureMouse = capture;

    updateHighlightStyle();
    update();  // Queues a repaint call.
}

void VirtualSceneControlWindow::hideEvent(QHideEvent* event) {
    QFrame::hide();

    if (mCaptureMouse) {
        setCaptureMouse(false);
    }
}

bool VirtualSceneControlWindow::eventFilter(QObject* target, QEvent* event) {
    // TODO(jwmcglynn): Touch the center of the screen when the mouse is
    // captured. See EmulatorQtWindow::handleMouseEvent
    if (mCaptureMouse) {
        if (event->type() == QEvent::MouseMove) {
            updateMouselook();
            return true;
        } else if (event->type() == QEvent::Wheel) {
            return true;
        } else if (event->type() == QEvent::KeyPress ||
                   event->type() == QEvent::KeyRelease) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (handleKeyEvent(keyEvent)) {
                updateVelocity();
                return true;
            }
        }
    }

    return QObject::eventFilter(target, event);
}

void VirtualSceneControlWindow::keyPressEvent(QKeyEvent* event) {
    mToolWindow->handleQtKeyEvent(event,
                                  QtKeyEventSource::VirtualSceneControlWindow);
}

void VirtualSceneControlWindow::keyReleaseEvent(QKeyEvent* event) {
    mToolWindow->handleQtKeyEvent(event,
                                  QtKeyEventSource::VirtualSceneControlWindow);
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

void VirtualSceneControlWindow::setActive(bool active) {
    mIsActive = active;
    if (active) {
        show();
    } else {
        hide();
    }
}

bool VirtualSceneControlWindow::isActive() {
    return mIsActive;
}

void VirtualSceneControlWindow::slot_mousePoller() {
    updateMouselook();
}

void VirtualSceneControlWindow::updateMouselook() {
    QPoint offset = QCursor::pos() - mPreviousMousePosition;
    mPreviousMousePosition = getMouseCaptureCenter();
    QCursor::setPos(mPreviousMousePosition);

    if (offset.x() != 0 || offset.y() != 0) {
        mEulerRotationRadians.x -= offset.y() * kPixelsToRotationRadians;
        mEulerRotationRadians.y -= offset.x() * kPixelsToRotationRadians;

        // Clamp up/down rotation to -80 and +80 degrees, like a FPS camera.
        if (mEulerRotationRadians.x <
            glm::radians(kMinVerticalRotationDegrees)) {
            mEulerRotationRadians.x = glm::radians(kMinVerticalRotationDegrees);
        } else if (mEulerRotationRadians.x >
                   glm::radians(kMaxVerticalRotationDegrees)) {
            mEulerRotationRadians.x = glm::radians(kMaxVerticalRotationDegrees);
        }

        if (mSensorsAgent) {
            // Rotations applied in the Y X Z order, but we need to convert to X
            // Y Z order for the physical model.
            glm::vec3 rotationRadians;
            glm::extractEulerAngleXYZ(
                    glm::eulerAngleYXZ(mEulerRotationRadians.y,
                                       mEulerRotationRadians.x,
                                       mEulerRotationRadians.z),
                    rotationRadians.x, rotationRadians.y, rotationRadians.z);
            const glm::vec3 rotationDegrees = glm::degrees(rotationRadians);

            mSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_ROTATION, rotationDegrees.x,
                    rotationDegrees.y, rotationDegrees.z,
                    PHYSICAL_INTERPOLATION_SMOOTH);
        }

        updateVelocity();
    }
}

void VirtualSceneControlWindow::updateHighlightStyle() {
    mControlsUi->instructions->setText(getInfoText());

    QString textStyle =
            QString(kTextStyleFormatString)
                    .arg(Ui::stylesheetFontSize(Ui::FontSize::Large));
    if (mCaptureMouse) {
        mControlsUi->instructions->setStyleSheet(textStyle +
                                                 kHighlightTextStyle);
    } else {
        mControlsUi->instructions->setStyleSheet(textStyle);
    }
}

QString VirtualSceneControlWindow::getInfoText() {
    if (mCaptureMouse) {
        return tr("Control view with mouse + WASDQE.");
    } else {
#if __APPLE__
        return tr("Press %1 Option to move camera.").arg(QChar(0x2325));
#else   // __APPLE__
        return tr("Press Alt to move camera.");
#endif  // !__APPLE__
    }
}

bool VirtualSceneControlWindow::handleKeyEvent(QKeyEvent* event) {
    const bool pressed = event->type() == QEvent::KeyPress;

    switch (event->key()) {
        case Qt::Key_W:
            mKeysHeld[Held_W] = pressed;
            break;
        case Qt::Key_A:
            mKeysHeld[Held_A] = pressed;
            break;
        case Qt::Key_S:
            mKeysHeld[Held_S] = pressed;
            break;
        case Qt::Key_D:
            mKeysHeld[Held_D] = pressed;
            break;
        case Qt::Key_Q:
            mKeysHeld[Held_Q] = pressed;
            break;
        case Qt::Key_E:
            mKeysHeld[Held_E] = pressed;
            break;
#ifdef __APPLE__
        case Qt::Key_unknown:
            // On OS X, when the Alt key is held Qt can't recognize the E key.
            // Compare the nativeVirtualKey code to see if this is the E key.
            if (event->nativeVirtualKey() == kVK_ANSI_E) {
                mKeysHeld[Held_E] = pressed;
                break;
            }

// Otherwise fall through.
#endif
        default:
            // Not handled.
            return false;
    }

    return true;
}

void VirtualSceneControlWindow::updateVelocity() {
    // Moving in an additional direction should not slow down other axes, so
    // this is intentionally not normalized. As a result, moving along
    // additional axes increases the overall velocity.
    const glm::vec3 lookDirectionBaseVelocity(
            (mKeysHeld[Held_A] ? -1.0f : 0.0f) +
                    (mKeysHeld[Held_D] ? 1.0f : 0.0f),
            (mKeysHeld[Held_Q] ? -1.0f : 0.0f) +
                    (mKeysHeld[Held_E] ? 1.0f : 0.0f),
            (mKeysHeld[Held_W] ? -1.0f : 0.0f) +
                    (mKeysHeld[Held_S] ? 1.0f : 0.0f));

    const glm::vec3 velocity = glm::angleAxis(mEulerRotationRadians.y,
                                              glm::vec3(0.0f, 1.0f, 0.0f)) *
                               lookDirectionBaseVelocity *
                               kMovementVelocityMetersPerSecond;

    if (velocity != mVelocity) {
        mVelocity = velocity;

        if (mSensorsAgent) {
            mSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_VELOCITY, velocity.x, velocity.y,
                    velocity.z, PHYSICAL_INTERPOLATION_SMOOTH);
        }
    }
}

QPoint VirtualSceneControlWindow::getMouseCaptureCenter() {
    QWidget* container = parentWidget();
    return container->pos() +
           QPoint(container->width() / 2, container->height() / 2);
}

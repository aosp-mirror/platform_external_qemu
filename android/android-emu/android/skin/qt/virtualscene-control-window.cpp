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

#include "android/base/system/System.h"
#include "android/featurecontrol/feature_control.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/tool-window.h"
#include "android/utils/debug.h"

#include <QDesktopWidget>
#include <QPainter>
#include <QScreen>
#include <QStyle>
#include <QTextStream>

#ifdef __APPLE__
#include <Carbon/Carbon.h>  // For kVK_ANSI_E
#include "android/skin/qt/mac-native-window.h"
#endif

#include <glm/gtx/euler_angles.hpp>

#define W(...) dwarning(__VA_ARGS__)

// Allow at most 5 reports every 60 seconds.
static constexpr uint64_t kReportWindowDurationUs = 1000 * 1000 * 60;
static constexpr uint32_t kMaxReportsPerWindow = 5;

static constexpr int kMousePollIntervalMilliseconds = 16;
static constexpr int kWindowHeight = 50;
static const QColor kHighlightColor = QColor(2, 136, 209);

static constexpr int kMetricsAggregateIntervalMilliseconds = 1000;
static constexpr uint64_t kTapAfterHotkeyThresholdMilliseconds = 2000;

static const float kPixelsToRotationRadians = 0.2f * static_cast<float>(M_PI / 180.0f);
static const float kMovementVelocityMetersPerSecond = 1.0f;
static const float kMinVerticalRotationDegrees = -80.0f;
static const float kMaxVerticalRotationDegrees = 80.0f;

static const char* kTextStyleFormatString =
        "* { font-size: %1; background-color: transparent; }"
        "*[ColorGroup=\"Highlight\"] { color: #eee; }";

const QAndroidSensorsAgent* VirtualSceneControlWindow::sSensorsAgent = nullptr;

VirtualSceneControlWindow::VirtualSceneControlWindow(
        EmulatorQtWindow* emulatorWindow,
        ToolWindow* toolWindow)
    : QFrame(emulatorWindow->containerWindow()),
      mEmulatorWindow(emulatorWindow),
      mToolWindow(toolWindow),
      mSizeTweaker(this),
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

    setWindowFlags(flags);

    mControlsUi->setupUi(this);
    dockMainWindow();

    SettingsTheme theme = getSelectedTheme();
    updateTheme(Ui::stylesheetForTheme(theme));

    QString textStyle =
            QString(kTextStyleFormatString)
                    .arg(Ui::stylesheetFontSize(Ui::FontSize::Large));
    mControlsUi->instructions->setStyleSheet(textStyle);

    QApplication::instance()->installEventFilter(this);

    connect(&mMousePoller, SIGNAL(timeout()), this, SLOT(slot_mousePoller()));
    connect(&mMetricsAggregateTimer, SIGNAL(timeout()), this,
            SLOT(slot_metricsAggregator()));

    QSettings settings;
    mShouldShowInfoDialog =
            settings.value(Ui::Settings::SHOW_VIRTUALSCENE_INFO, true).toBool();
}

VirtualSceneControlWindow::~VirtualSceneControlWindow() {
    QApplication::instance()->removeEventFilter(this);
}

void VirtualSceneControlWindow::dockMainWindow() {
    // Align vertically relative to the main window's frame.
    // Align horizontally relative to its contents.
    // If we're frameless, adjust for a transparent border
    // around the skin.
    const int parentWidgetWidth = parentWidget()->frameGeometry().width() -
                                  mEmulatorWindow->getLeftTransparency() -
                                  mEmulatorWindow->getRightTransparency();
    const int virtualSceneWidgetWidth =
            std::min(parentWidgetWidth, kVirtualSceneControlWindowMaxWidth);

    setWidth(virtualSceneWidgetWidth);
    move(parentWidget()->frameGeometry().left() +
                 mEmulatorWindow->getLeftTransparency() +
                 (parentWidgetWidth - virtualSceneWidgetWidth) / 2,
         parentWidget()->geometry().bottom() -
                 mEmulatorWindow->getBottomTransparency() +
                 kVirtualSceneControlWindowOffset);
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
    updateHighlightAndFocusStyle();
}

// static
void VirtualSceneControlWindow::setAgent(const UiEmuAgent* agentPtr) {
    sSensorsAgent = agentPtr->sensors;
}

void VirtualSceneControlWindow::setWidth(int width) {
    resize(QSize(width, kWindowHeight));
}

void VirtualSceneControlWindow::setCaptureMouse(bool capture) {
    if (capture) {
        mOriginalMousePosition = QCursor::pos();
        ++mVirtualSceneMetrics.hotkeyInvokeCount;
        mMouseCaptureElapsed.start();

        // Get the starting rotation.
        if (sSensorsAgent) {
            glm::vec3 eulerDegrees;
            sSensorsAgent->getPhysicalParameter(
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

            sSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_AMBIENT_MOTION, 0.005f, 0.f, 0.f,
                    PHYSICAL_INTERPOLATION_SMOOTH);
        }

        mEmulatorWindow->containerWindow()->hideVirtualSceneInfoDialog();

        QCursor cursor(Qt::BlankCursor);
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

        if (sSensorsAgent) {
            sSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_AMBIENT_MOTION, 0.f,
                    0.f, 0.f,
                    PHYSICAL_INTERPOLATION_SMOOTH);
        }

        updateVelocity();

        mVirtualSceneMetrics.hotkeyDurationMs =
                mMouseCaptureElapsed.isValid() ? mMouseCaptureElapsed.elapsed()
                                               : 0;
        mMouseCaptureElapsed.invalidate();
        mLastHotkeyReleaseElapsed.start();
    }

    emit(virtualSceneControlsEngaged(capture));

    mCaptureMouse = capture;

    updateHighlightAndFocusStyle();
    update();  // Queues a repaint call.
}

void VirtualSceneControlWindow::showEvent(QShowEvent* event) {
#ifdef __APPLE__
    // See EmulatorContainer::showEvent() for explanation on why this is needed
    WId parentWid = parentWidget()->effectiveWinId();
    parentWid = (WId)getNSWindow((void*)parentWid);

    WId wid = effectiveWinId();
    Q_ASSERT(wid && parentWid);
    wid = (WId)getNSWindow((void*)wid);
    nsWindowAdopt((void*)parentWid, (void*)wid);
#endif

    mVirtualSceneMetrics = VirtualSceneMetrics();
    mOverallDuration.restart();
    mMetricsAggregateTimer.start(kMetricsAggregateIntervalMilliseconds);
    aggregateMovementMetrics(true);
}

void VirtualSceneControlWindow::hideEvent(QHideEvent* event) {
    QFrame::hide();

    if (mCaptureMouse) {
        setCaptureMouse(false);
    }

    mMetricsAggregateTimer.stop();
    slot_metricsAggregator();  // Perform final metrics aggregation.

    const uint64_t now = android::base::System::get()->getHighResTimeUs();

    // Reset the metrics reporting limiter if enough time has passed.
    if (mReportWindowStartUs + kReportWindowDurationUs < now) {
        mReportWindowStartUs = now;
        mReportWindowCount = 0;
    }

    if (mReportWindowCount > kMaxReportsPerWindow) {
        W("%s: Dropping metrics, too many recent reports.", __FUNCTION__);
        return;
    }

    ++mReportWindowCount;

    android_studio::EmulatorVirtualSceneSession metrics;
    metrics.set_duration_ms(mOverallDuration.elapsed());
    if (mVirtualSceneMetrics.minSensorDelayMs >= 0 &&
        mVirtualSceneMetrics.minSensorDelayMs != INT_MAX) {
        metrics.set_min_sensor_delay_ms(
                static_cast<uint32_t>(mVirtualSceneMetrics.minSensorDelayMs));
    }
    metrics.set_tap_count(mVirtualSceneMetrics.tapCount);
    metrics.set_orientation_change_count(
            mVirtualSceneMetrics.orientationChangeCount);
    metrics.set_virtual_sensors_visible(
            mVirtualSceneMetrics.virtualSensorsVisible);
    metrics.set_virtual_sensors_interaction_count(
            mVirtualSceneMetrics.virtualSensorsInteractionCount);
    metrics.set_hotkey_invoke_count(mVirtualSceneMetrics.hotkeyInvokeCount);
    metrics.set_hotkey_duration_ms(mVirtualSceneMetrics.hotkeyDurationMs);
    metrics.set_taps_after_hotkey_invoke(
            mVirtualSceneMetrics.tapsAfterHotkeyInvoke);
    metrics.set_total_rotation_radians(
            mVirtualSceneMetrics.totalRotationRadians);
    metrics.set_total_translation_meters(
            mVirtualSceneMetrics.totalTranslationMeters);

    android::metrics::MetricsReporter::get().report(
            [metrics](android_studio::AndroidStudioEvent* event) {
                event->mutable_emulator_details()
                        ->mutable_virtual_scene()
                        ->CopyFrom(metrics);
            });
}

bool VirtualSceneControlWindow::eventFilter(QObject* target, QEvent* event) {
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
        } else if (event->type() == QEvent::WindowDeactivate &&
                   target == parentWidget()) {
            setCaptureMouse(false);
        }
    }

    if (event->type() == QEvent::WindowActivate ||
        event->type() == QEvent::WindowDeactivate) {
        const bool hotkeyAvailable = !mToolWindow->isExtendedWindowFocused();
        if (hotkeyAvailable != mIsHotkeyAvailable) {
            mIsHotkeyAvailable = hotkeyAvailable;
            updateHighlightAndFocusStyle();
            update();  // Queue a repaint event.
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
    if (mIsHotkeyAvailable) {
        outlinePen.setColor(Qt::black);
    } else {
        outlinePen.setColor(QColor(127, 127, 127));
    }
    outlinePen.setWidth(1);
    p.setPen(outlinePen);
    p.drawRect(rect);
    p.end();
}

void VirtualSceneControlWindow::setActive(bool active) {
    mIsActive = active;
    if (active) {
        show();

        if (mShouldShowInfoDialog) {
            mEmulatorWindow->containerWindow()->showVirtualSceneInfoDialog();

            connect(mEmulatorWindow->containerWindow(),
                    SIGNAL(hideVirtualSceneInfoDialog()), this,
                    SLOT(slot_virtualSceneInfoDialogHasBeenSeen()));
        }
    } else {
        hide();

        // The camera session has ended.  If the info dialog is still open, we
        // want to show it again next time in case the camera crashed and the
        // user did not read the dialog.  To accomplish this, disconnect the
        // hidden event before hiding the window so that we don't unset the
        // mShouldShowInfoDialog flag when the window hides.
        disconnect(mEmulatorWindow->containerWindow(),
                   SIGNAL(hideVirtualSceneInfoDialog()),
                   this, SLOT(slot_virtualSceneInfoDialogHasBeenSeen()));

        mEmulatorWindow->containerWindow()->hideVirtualSceneInfoDialog();
    }
}

bool VirtualSceneControlWindow::isActive() {
    return mIsActive;
}

void VirtualSceneControlWindow::reportMouseButtonDown() {
    ++mVirtualSceneMetrics.tapCount;
    if (mLastHotkeyReleaseElapsed.isValid() &&
        mLastHotkeyReleaseElapsed.elapsed() <
                kTapAfterHotkeyThresholdMilliseconds) {
        ++mVirtualSceneMetrics.tapsAfterHotkeyInvoke;
    }
}

void VirtualSceneControlWindow::orientationChanged(SkinRotation) {
    ++mVirtualSceneMetrics.orientationChangeCount;
}

void VirtualSceneControlWindow::virtualSensorsPageVisible() {
    // We receive an event when the virtual sensors page is opened while the
    // camera is running. If it is opened before the session has started, only
    // count it if there is an interaction on the page.
    mVirtualSceneMetrics.virtualSensorsVisible = true;
}

void VirtualSceneControlWindow::virtualSensorsInteraction() {
    mVirtualSceneMetrics.virtualSensorsVisible = true;
    ++mVirtualSceneMetrics.virtualSensorsInteractionCount;
}

void VirtualSceneControlWindow::addShortcutKeysToKeyStore(
        ShortcutKeyStore<QtUICommand>& keystore) {
    if (!feature_is_enabled(kFeature_VirtualScene)) {
        return;
    }

    QString shortcuts =
            "Alt+W VIRTUAL_SCENE_MOVE_FORWARD\n"
            "Alt+A VIRTUAL_SCENE_MOVE_LEFT\n"
            "Alt+S VIRTUAL_SCENE_MOVE_BACKWARD\n"
            "Alt+D VIRTUAL_SCENE_MOVE_RIGHT\n"
            "Alt+Q VIRTUAL_SCENE_MOVE_DOWN\n"
            "Alt+E VIRTUAL_SCENE_MOVE_UP\n"
            "Alt VIRTUAL_SCENE_CONTROL\n";

    QTextStream stream(&shortcuts);
    keystore.populateFromTextStream(stream, parseQtUICommand, true);
}

void VirtualSceneControlWindow::slot_mousePoller() {
    updateMouselook();
}

void VirtualSceneControlWindow::slot_metricsAggregator() {
    if (sSensorsAgent) {
        int delayMs = static_cast<uint32_t>(sSensorsAgent->getDelayMs());
        if (delayMs < mVirtualSceneMetrics.minSensorDelayMs) {
            mVirtualSceneMetrics.minSensorDelayMs = delayMs;
        }

        aggregateMovementMetrics();
    }
}

void VirtualSceneControlWindow::slot_virtualSceneInfoDialogHasBeenSeen() {
    // If the window is explicitly hidden, prevent it from being shown again
    // this session.
    mShouldShowInfoDialog = false;
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

        if (sSensorsAgent) {
            // Rotations applied in the Y X Z order, but we need to convert to X
            // Y Z order for the physical model.
            glm::vec3 rotationRadians;
            glm::extractEulerAngleXYZ(
                    glm::eulerAngleYXZ(mEulerRotationRadians.y,
                                       mEulerRotationRadians.x,
                                       mEulerRotationRadians.z),
                    rotationRadians.x, rotationRadians.y, rotationRadians.z);
            const glm::vec3 rotationDegrees = glm::degrees(rotationRadians);

            sSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_ROTATION, rotationDegrees.x,
                    rotationDegrees.y, rotationDegrees.z,
                    PHYSICAL_INTERPOLATION_SMOOTH);
        }

        updateVelocity();
    }
}

void VirtualSceneControlWindow::updateHighlightAndFocusStyle() {
    mControlsUi->instructions->setText(getInfoText());

    if (mCaptureMouse) {
        mControlsUi->instructions->setProperty("ColorGroup", "Highlight");
    } else if (!mIsHotkeyAvailable) {
        mControlsUi->instructions->setProperty("ColorGroup", "Inactive");
    } else {
        mControlsUi->instructions->setProperty("ColorGroup", "");
    }

    mControlsUi->instructions->style()->unpolish(mControlsUi->instructions);
    mControlsUi->instructions->style()->polish(mControlsUi->instructions);
}

QString VirtualSceneControlWindow::getInfoText() {
    if (mCaptureMouse) {
        return tr("Control view with mouse + WASDQE.");
    } else {
#ifdef Q_OS_MAC
        return tr("Press \u2325 Option to move camera.");
#else   // Q_OS_MAC
        return tr("Press Alt to move camera.");
#endif  // Q_OS_MAC
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

        if (sSensorsAgent) {
            sSensorsAgent->setPhysicalParameterTarget(
                    PHYSICAL_PARAMETER_VELOCITY, velocity.x, velocity.y,
                    velocity.z, PHYSICAL_INTERPOLATION_SMOOTH);
        }
    }
}

void VirtualSceneControlWindow::aggregateMovementMetrics(bool reset) {
    if (reset) {
        mVirtualSceneMetrics.totalTranslationMeters = 0.0;
        mVirtualSceneMetrics.totalRotationRadians = 0.0;
    }

    if (!sSensorsAgent) {
        return;
    }

    glm::vec3 eulerDegrees;
    glm::vec3 position;
    sSensorsAgent->getPhysicalParameter(
            PHYSICAL_PARAMETER_ROTATION, &eulerDegrees.x, &eulerDegrees.y,
            &eulerDegrees.z, PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);
    sSensorsAgent->getPhysicalParameter(
            PHYSICAL_PARAMETER_POSITION, &position.x, &position.y, &position.z,
            PARAMETER_VALUE_TYPE_CURRENT_NO_AMBIENT_MOTION);

    const glm::quat rotation = glm::eulerAngleXYZ(glm::radians(eulerDegrees.x),
                                                  glm::radians(eulerDegrees.y),
                                                  glm::radians(eulerDegrees.z));

    if (!reset) {
        mVirtualSceneMetrics.totalTranslationMeters +=
                glm::distance(mLastReportedPosition, position);
        const double angle =
                glm::angle(glm::inverse(mLastReportedRotation) * rotation);
        if (!std::isnan(angle)) {
            mVirtualSceneMetrics.totalRotationRadians += angle;
        }
    }

    mLastReportedPosition = position;
    mLastReportedRotation = rotation;
}

QPoint VirtualSceneControlWindow::getMouseCaptureCenter() {
    QWidget* container = parentWidget();
    return container->pos() +
           QPoint(container->width() / 2, container->height() / 2);
}

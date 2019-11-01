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

#include <QtCore/qglobal.h>
#include <qapplication.h>
#include <qcoreevent.h>
#include <qglobal.h>
#include <qnamespace.h>
#include <qpoint.h>
#include <qwindowdefs.h>
#include <QApplication>
#include <QColor>
#include <QCoreApplication>
#include <QCursor>
#include <QDesktopWidget>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QRect>
#include <QScreen>
#include <QSettings>
#include <QSize>
#include <QStyle>
#include <QTextStream>
#include <QVariant>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <functional>

#include "android/base/Log.h"
#include "android/base/system/System.h"
#include "android/featurecontrol/feature_control.h"
#include "android/hw-sensors.h"
#include "android/metrics/MetricsReporter.h"
#include "android/physics/GlmHelpers.h"
#include "android/physics/Physics.h"
#include "android/settings-agent.h"
#include "android/skin/keycode.h"
#include "android/skin/qt/emulator-container.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/qt-settings.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/skin/qt/shortcut-key-store.h"
#include "android/skin/qt/stylesheet.h"
#include "android/skin/qt/tool-window.h"
#include "android/virtualscene/WASDInputHandler.h"
// #include "glm/gtc/../detail/func_geometric.inl"
// #include "glm/gtc/../detail/func_trigonometric.inl"
#include "glm/gtc/quaternion.hpp"
#include "studio_stats.pb.h"
#include "ui_virtualscene-controls.h"

class QHideEvent;
class QKeyEvent;
class QObject;
class QPaintEvent;
class QScreen;
class QShowEvent;
class QString;
class QWidget;

#ifdef __APPLE__
#import <Carbon/Carbon.h>  // For kVK_ANSI_E
#include "android/skin/qt/mac-native-window.h"
#endif

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

static QPoint setMousePosition(QPoint position) {
#ifdef __APPLE__
    CGPoint pos;
    pos.x = position.x();
    pos.y = position.y();

    // On OS X 10.14, QCursor::setPos requires accessibility permissions and may
    // silently fail if access is denied (without ever reprompting). Instead of
    // setPos, use CGWarpMouseCursorPosition, which is intended to recenter the
    // mouse position for mouselook in games.
    if (CGWarpMouseCursorPosition(pos) == kCGErrorSuccess) {
        CGAssociateMouseAndMouseCursorPosition(true);
        return position;
    } else {
        // If this fails, return the current mouse position, which allows
        // limited control but is bounded by screen edges.
        return QCursor::pos();
    }
#else
    QCursor::setPos(position);
    return position;
#endif
}

VirtualSceneControlWindow::VirtualSceneControlWindow(
        EmulatorQtWindow* emulatorWindow,
        ToolWindow* toolWindow)
    : QFrame(emulatorWindow->containerWindow()),
      mEmulatorWindow(emulatorWindow),
      mToolWindow(toolWindow),
      mSizeTweaker(this),
      mControlsUi(new Ui::VirtualSceneControls()) {
    setObjectName("VirtualSceneControlWindow");

    if (sSensorsAgent) {
        mInputHandler = android::virtualscene::WASDInputHandler(sSensorsAgent);
    } else {
        // If this happens, the user won't be able to control the virtual scene,
        // so we want it to be visible.
        LOG(ERROR) << "Failed to initialize WASD Input Handler";
    }

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

        if (mInputHandler) {
            mInputHandler->enable();
        }

        mEmulatorWindow->containerWindow()->hideVirtualSceneInfoDialog();

        QCursor cursor(Qt::BlankCursor);
        parentWidget()->grabMouse(cursor);

        // Move cursor to the center of the window.
        mPreviousMousePosition = setMousePosition(getMouseCaptureCenter());

        // Qt only sends mouse move events when the mouse is in the application
        // bounds, set up a timer to poll for mouse position changes in case the
        // mouse escapes.
        mMousePoller.start(kMousePollIntervalMilliseconds);
    } else {
        mMousePoller.stop();

        setMousePosition(mOriginalMousePosition);
        parentWidget()->releaseMouse();

        if (mInputHandler) {
            mInputHandler->disable();
        }

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
        LOG(INFO) << "Dropping metrics, too many recent reports.";
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
        if ((event->type() == QEvent::WindowDeactivate &&
                       target == parentWidget())
#ifdef __APPLE__
            || !isOptionKeyHeld()
#endif
        ) {
            setCaptureMouse(false);
        }
        else if (event->type() == QEvent::MouseMove) {
            updateMouselook();
            return true;
        } else if (event->type() == QEvent::Wheel) {
            return true;
        } else if (event->type() == QEvent::KeyPress ||
                   event->type() == QEvent::KeyRelease) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (handleKeyEvent(keyEvent)) {
                return true;
            }
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

void VirtualSceneControlWindow::setActiveForCamera(bool active) {
    mIsActiveCamera = active;
    if (active) {
        show();

        if (mShouldShowInfoDialog) {
            mEmulatorWindow->containerWindow()->showVirtualSceneInfoDialog();

            connect(mEmulatorWindow->containerWindow(),
                    SIGNAL(hideVirtualSceneInfoDialog()), this,
                    SLOT(slot_virtualSceneInfoDialogHasBeenSeen()));
        }
    } else {
        if (!mIsActiveRecording) {
            hide();
        }

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

void VirtualSceneControlWindow::setActiveForRecording(bool active) {
    mIsActiveRecording = active;
    if (mIsActiveCamera) {
        return;
    }
    if (active) {
        show();
    } else {
        hide();
    }
}

bool VirtualSceneControlWindow::isActive() {
    return mIsActiveCamera || mIsActiveRecording;
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
    mPreviousMousePosition = setMousePosition(getMouseCaptureCenter());

    if (mInputHandler) {
        mInputHandler->mouseMove(offset.x(), offset.y());
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

static android::base::Optional<android::virtualscene::ControlKey> toControlKey(
        QKeyEvent* event) {
    using android::virtualscene::ControlKey;
#ifdef __APPLE__
    int nativeCode = event->nativeVirtualKey();
#else
    int nativeCode = event->nativeScanCode();
#endif
    int linuxKeyCode = skin_native_scancode_to_linux(nativeCode);
    switch (linuxKeyCode) {
        case LINUX_KEY_W:
            return ControlKey::W;
        case LINUX_KEY_A:
            return ControlKey::A;
        case LINUX_KEY_S:
            return ControlKey::S;
        case LINUX_KEY_D:
            return ControlKey::D;
        case LINUX_KEY_Q:
            return ControlKey::Q;
        case LINUX_KEY_E:
            return ControlKey::E;
        default:
            return {};
    }
}

bool VirtualSceneControlWindow::handleKeyEvent(QKeyEvent* event) {
    if (!mInputHandler) {
        return false;
    }

    const auto controlKey = toControlKey(event);
    if (!controlKey) {
        // Not a key that is handled.
        return false;
    }

    if (event->type() == QEvent::KeyPress) {
        mInputHandler->keyDown(controlKey.value());
    } else {
        mInputHandler->keyUp(controlKey.value());
    }

    return true;
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

    const glm::quat rotation = fromEulerAnglesXYZ(glm::radians(eulerDegrees));

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

void VirtualSceneControlWindow::setRecordingState(bool state) {
    setActiveForRecording(state);
    if (state) {
        mControlsUi->recOngoingButton->setIcon(
                getIconForCurrentTheme("recordCircle"));
        mControlsUi->recOngoingButton->setIconSize(QSize(30, 20));
        mControlsUi->recOngoingButton->setText(tr("RECORDING ONGOING "));
        mControlsUi->recOngoingButton->show();
    } else {
        mControlsUi->recOngoingButton->hide();
    }
}

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

#pragma once

#include "android/base/Optional.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/skin/qt/shortcut-key-store.h"
#include "android/skin/qt/size-tweaker.h"
#include "android/skin/rect.h"
#include "android/ui-emu-agent.h"
#include "android/virtualscene/WASDInputHandler.h"

#include "ui_virtualscene-controls.h"

#include <QElapsedTimer>
#include <QFrame>
#include <QObject>
#include <QWidget>
#include <QtCore>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <memory>

class EmulatorQtWindow;
class ToolWindow;

// Design requested a max width of 700 dp, and offset of 16 from the emulator
// window.  This is defined here.
static constexpr int kVirtualSceneControlWindowMaxWidth = 700;
static constexpr int kVirtualSceneControlWindowOffset = 16;

class VirtualSceneControlWindow : public QFrame {
    Q_OBJECT

public:
    explicit VirtualSceneControlWindow(EmulatorQtWindow* emulatorWindow,
                                       ToolWindow* toolWindow);
    virtual ~VirtualSceneControlWindow();

    void dockMainWindow();
    bool handleQtKeyEvent(QKeyEvent* event, QtKeyEventSource source);
    void updateTheme(const QString& styleSheet);

    void setWidth(int width);
    void setCaptureMouse(bool capture);

    bool eventFilter(QObject* target, QEvent* event) override;

    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent*) override;

    void setActive(bool active);
    bool isActive();

    void reportMouseButtonDown();

    static void setAgent(const UiEmuAgent* agentPtr);
    static void addShortcutKeysToKeyStore(
            ShortcutKeyStore<QtUICommand>& keystore);
signals:
    void virtualSceneControlsEngaged(bool engaged);

public slots:
    void orientationChanged(SkinRotation);
    void virtualSensorsPageVisible();
    void virtualSensorsInteraction();

private slots:
    void slot_mousePoller();
    void slot_metricsAggregator();
    void slot_virtualSceneInfoDialogHasBeenSeen();

private:
    void updateMouselook();
    void updateHighlightAndFocusStyle();
    QString getInfoText();

    // Returns true if the event was handled.
    bool handleKeyEvent(QKeyEvent* event);
    void aggregateMovementMetrics(bool reset = false);

    QPoint getMouseCaptureCenter();

    static const QAndroidSensorsAgent* sSensorsAgent;

    EmulatorQtWindow* mEmulatorWindow = nullptr;
    ToolWindow* mToolWindow = nullptr;
    SizeTweaker mSizeTweaker;
    std::unique_ptr<Ui::VirtualSceneControls> mControlsUi;
    android::base::Optional<android::virtualscene::WASDInputHandler>
            mInputHandler;

    bool mCaptureMouse = false;
    QTimer mMousePoller;
    QPoint mOriginalMousePosition;
    QPoint mPreviousMousePosition;

    bool mIsActive = false;
    bool mShouldShowInfoDialog = true;
    bool mIsHotkeyAvailable = true;

    uint64_t mReportWindowStartUs = 0;
    uint32_t mReportWindowCount = 0;

    // Aggregate metrics to determine how the virtual scene window is used.
    // Metrics are collected while the window is visible, and reported when the
    // session ends.
    QTimer mMetricsAggregateTimer;
    QElapsedTimer mOverallDuration;
    QElapsedTimer mMouseCaptureElapsed;
    QElapsedTimer mLastHotkeyReleaseElapsed;
    glm::quat mLastReportedRotation = glm::quat();
    glm::vec3 mLastReportedPosition = glm::vec3();

    struct VirtualSceneMetrics {
        int minSensorDelayMs = INT_MAX;
        uint32_t tapCount = 0;
        uint32_t orientationChangeCount = 0;
        bool virtualSensorsVisible = false;
        uint32_t virtualSensorsInteractionCount = 0;
        uint32_t hotkeyInvokeCount = 0;
        uint64_t hotkeyDurationMs = 0;
        uint32_t tapsAfterHotkeyInvoke = 0;
        double totalRotationRadians = 0.0;
        double totalTranslationMeters = 0.0;
    };

    VirtualSceneMetrics mVirtualSceneMetrics;
};

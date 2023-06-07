/* Copyright (C) 2015-2016 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include <qobjectdefs.h>                             // for Q_OBJECT, slots
#include <QButtonGroup>                              // for QButtonGroup
#include <QFrame>                                    // for QFrame
#include <QString>                                   // for QString
#include <map>                                       // for map
#include <memory>                                    // for shared_ptr, uniq...
#include <mutex>
#include <condition_variable>

#include "android/settings-agent.h"                  // for SettingsTheme
#include "android/skin/qt/extended-window-styles.h"  // for ExtendedWindowPane
#include "android/skin/qt/qt-ui-commands.h"          // for QtUICommand
#include "android/skin/qt/size-tweaker.h"            // for SizeTweaker
#include "android/ui-emu-agent.h"                    // for UiEmuAgent
#include "host-common/qt_ui_defs.h"


class EmulatorQtWindow;
class QCloseEvent;
class QHideEvent;
class QKeyEvent;
class QObject;
class QPushButton;
class QShowEvent;
class ToolWindow;
class VirtualSceneControlWindow;
class VirtualSensorsPage;
template <class CommandType> class ShortcutKeyStore;

namespace android {
namespace metrics {
 class UiEventTracker;
}
}

using android::metrics::UiEventTracker;

namespace Ui {
    class ExtendedControls;
}

class ExtendedWindow : public QFrame
{
    Q_OBJECT

public:
    ExtendedWindow(EmulatorQtWindow* eW, ToolWindow* tW);

    ~ExtendedWindow();

    static void setAgent(const UiEmuAgent* agentPtr);
    // Some pages on the extended window perform actions even if the UI
    // is not created.
    // In this case the dtor is not called when the Emulator exits, so
    // someone needs to call 'shutdown' to let us know we should stop
    // those actions.
    static void shutDown();
    void sendMetricsOnShutDown();

    void show();
    void showPane(ExtendedWindowPane pane);

    // Wait until this component has reached the visibility
    // state.
    void waitForVisibility(bool visible);

    void connectVirtualSceneWindow(
            VirtualSceneControlWindow* virtualSceneWindow);

    VirtualSensorsPage* getVirtualSensorsPage();

    void overrideUiObjects();

private slots:
    void switchFrameAlways(bool showFrame);
    void switchOnTop(bool isOntop);
    void switchToTheme(SettingsTheme theme);
    void disableMouseWheel(bool disabled);
    void disablePinchToZoom(bool disabled);
    void pauseAvdWhenMinimized(bool pause);
    void showMacroRecordPage();
    void hideRotationButtons();

    // Master tabs
    void on_batteryButton_clicked();
    void on_cameraButton_clicked();
    void on_bugreportButton_clicked();
    void on_cellularButton_clicked();
    void on_dpadButton_clicked();
    void on_displaysButton_clicked();
    void on_fingerButton_clicked();
    void on_googlePlayButton_clicked();
    void on_helpButton_clicked();
    void on_carDataButton_clicked();
    void on_carRotaryButton_clicked();
    void on_sensorReplayButton_clicked();
    void on_locationButton_clicked();
    void on_microphoneButton_clicked();
    void on_recordButton_clicked();
    void on_rotaryInputButton_clicked();
    void on_settingsButton_clicked();
    void on_snapshotButton_clicked();
    void on_telephoneButton_clicked();
    void on_virtSensorsButton_clicked();

private:
    void closeEvent(QCloseEvent* ce) override;
    void keyPressEvent(QKeyEvent* e) override;
    void adjustTabs(ExtendedWindowPane thisIndex);
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

    EmulatorQtWindow* mEmulatorWindow;
    ToolWindow*  mToolWindow;
    std::map<ExtendedWindowPane, QPushButton*> mPaneButtonMap;
    std::shared_ptr<UiEventTracker> mPaneInvocationTracker;
    const ShortcutKeyStore<QtUICommand>* mQtUIShortcuts;
    std::unique_ptr<Ui::ExtendedControls> mExtendedUi;
    bool mFirstShowEvent = true;
    SizeTweaker mSizeTweaker;
    QButtonGroup mSidebarButtons;
    bool mExtendedWindowWasShown = false;
    int mHAnchor = 0;
    int mVAnchor = 0;
    std::condition_variable mCvVisible;
    std::mutex mMutexVisible;
    bool mVisible{false};
    bool mExtendedInitialized{false};
};
